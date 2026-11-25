// EON — l'éditeur de salles dans un onglet d'Unreal (menu EON Salles › Éditeur de salles, ou Fenêtre › Outils).
//
// L'onglet affiche Tools/EditeurSalles/EditeurSalles.html dans le navigateur intégré d'Unreal. Un petit serveur HTTP
// local (localhost:8790, jamais ouvert au réseau) sert la page et fait pour elle ce que Chrome fait avec ses dossiers :
//   GET  /eon/editeur            la page de l'éditeur
//   GET  /eon/api/info           dossiers utilisés
//   GET  /eon/api/salles         les .eonsalle du dossier Salles : [{name, id, t, txt}]
//   POST /eon/api/salle?nom=&connu=&force=   écrit un .eonsalle (corps = texte) ; {conflit} si changé depuis « connu »
//   GET  /eon/api/biblio         la bibliothèque d'assets : chemins relatifs à SourceAssets
//   GET  /eon/biblio?p=chemin    un fichier de la bibliothèque (.eonasset, .eonapercu, .eontheme, .png, .webp)
// La page reconnaît ce mode à son adresse (/eon/...) : aucun dossier à choisir.
// Dossier des salles : même règle que Content/Python/eon_salles.py (le Drive s'il est monté, sinon Salles du projet).
#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "IWebBrowserDialog.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "SWebBrowser.h"
#include "WebBrowserModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogEONSalles, Log, All);

namespace EONSallesPanel
{
	static const FName TabId(TEXT("EONEditeurSalles"));
	static const uint32 Port = 8790;
	static TSharedPtr<IHttpRouter> Router;
	static TArray<FHttpRouteHandle> Routes;

	static FString ProjetDir() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); }
	static const TCHAR* Drive = TEXT("G:/My Drive/EON_EditeurSalles");
	static FString SallesDir()
	{
		return IFileManager::Get().DirectoryExists(*FPaths::GetPath(Drive)) ? FString(Drive) / TEXT("Salles") : ProjetDir() / TEXT("Salles");
	}
	static FString BiblioDir() { return ProjetDir() / TEXT("SourceAssets"); }
	static FString Url() { return FString::Printf(TEXT("http://localhost:%u/eon/editeur"), Port); }
	static FText Titre() { return FText::FromString(TEXT("Éditeur de salles")); }

	static FString Stamp(const FString& Path)
	{
		return FString::Printf(TEXT("%lld"), IFileManager::Get().GetTimeStamp(*Path).GetTicks());
	}

	static void Send(const FHttpResultCallback& Done, TUniquePtr<FHttpServerResponse> R, EHttpServerResponseCodes Code = EHttpServerResponseCodes::Ok)
	{
		R->Code = Code;
		R->Headers.Add(TEXT("Cache-Control"), { TEXT("no-store") });
		Done(MoveTemp(R));
	}
	static bool Text(const FHttpResultCallback& Done, const FString& S, const FString& Type, EHttpServerResponseCodes Code = EHttpServerResponseCodes::Ok)
	{
		Send(Done, FHttpServerResponse::Create(S, Type), Code);
		return true;
	}
	static bool Json(const FHttpResultCallback& Done, const TSharedRef<FJsonObject>& O)
	{
		FString S; FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&S));
		return Text(Done, S, TEXT("application/json; charset=utf-8"));
	}
	static bool Error(const FHttpResultCallback& Done, const FString& Msg, EHttpServerResponseCodes Code = EHttpServerResponseCodes::BadRequest)
	{
		UE_LOG(LogEONSalles, Warning, TEXT("Éditeur de salles : %s"), *Msg);
		return Text(Done, Msg, TEXT("text/plain; charset=utf-8"), Code);
	}
	static bool File(const FHttpResultCallback& Done, const FString& Path, const FString& Type)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path)) return Error(Done, TEXT("Fichier introuvable : ") + Path, EHttpServerResponseCodes::NotFound);
		Send(Done, FHttpServerResponse::Create(MoveTemp(Bytes), Type));
		return true;
	}

	// Un nom de salle : un fichier .eonsalle directement dans le dossier Salles, rien d'autre.
	static bool NomOk(const FString& N)
	{
		return N.EndsWith(TEXT(".eonsalle")) && N.Len() < 128 && !N.Contains(TEXT("/")) && !N.Contains(TEXT("\\"))
			&& !N.Contains(TEXT("..")) && !N.Contains(TEXT(":"));
	}
	static const TCHAR* BiblioExts[] = { TEXT("eonasset"), TEXT("eonapercu"), TEXT("eontheme"), TEXT("png"), TEXT("webp") };
	static bool BiblioExtOk(const FString& P)
	{
		const FString E = FPaths::GetExtension(P).ToLower();
		for (const TCHAR* X : BiblioExts) if (E == X) return true;
		return false;
	}

	static bool Page(const FHttpServerRequest&, const FHttpResultCallback& Done)
	{
		return File(Done, ProjetDir() / TEXT("Tools/EditeurSalles/EditeurSalles.html"), TEXT("text/html; charset=utf-8"));
	}

	static bool Info(const FHttpServerRequest&, const FHttpResultCallback& Done)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("salles"), SallesDir());
		O->SetStringField(TEXT("biblio"), BiblioDir());
		return Json(Done, O);
	}

	static bool ListeSalles(const FHttpServerRequest&, const FHttpResultCallback& Done)
	{
		const FString Dir = SallesDir();
		TArray<FString> Noms;
		IFileManager::Get().FindFiles(Noms, *(Dir / TEXT("*.eonsalle")), true, false);
		TArray<TSharedPtr<FJsonValue>> L;
		for (const FString& N : Noms)
		{
			FString Txt;
			if (!FFileHelper::LoadFileToString(Txt, *(Dir / N))) continue;
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("name"), N);
			O->SetStringField(TEXT("id"), N);
			O->SetStringField(TEXT("t"), Stamp(Dir / N));
			O->SetStringField(TEXT("txt"), Txt);
			L.Add(MakeShared<FJsonValueObject>(O));
		}
		FString S; FJsonSerializer::Serialize(L, TJsonWriterFactory<>::Create(&S));
		return Text(Done, S, TEXT("application/json; charset=utf-8"));
	}

	static bool EcrireSalle(const FHttpServerRequest& Req, const FHttpResultCallback& Done)
	{
		const FString* Nom = Req.QueryParams.Find(TEXT("nom"));
		if (!Nom || !NomOk(*Nom)) return Error(Done, TEXT("Nom de salle refusé"));
		const FString* Connu = Req.QueryParams.Find(TEXT("connu"));
		const FString* Force = Req.QueryParams.Find(TEXT("force"));
		const FString Dir = SallesDir(), Path = Dir / *Nom;
		const bool bExiste = IFileManager::Get().FileExists(*Path);
		if (!(Force && *Force == TEXT("1")))
		{
			const bool bConflit = (!Connu || Connu->IsEmpty()) ? bExiste : (bExiste && Stamp(Path) != *Connu);
			if (bConflit)
			{
				TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
				O->SetBoolField(TEXT("conflit"), true);
				return Json(Done, O);
			}
		}
		IFileManager::Get().MakeDirectory(*Dir, true);
		if (!FFileHelper::SaveArrayToFile(Req.Body, *Path)) return Error(Done, TEXT("Écriture impossible : ") + Path, EHttpServerResponseCodes::ServerError);
		UE_LOG(LogEONSalles, Log, TEXT("Éditeur de salles : %s enregistrée"), **Nom);
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("id"), *Nom);
		O->SetStringField(TEXT("t"), Stamp(Path));
		return Json(Done, O);
	}

	static bool ListeBiblio(const FHttpServerRequest&, const FHttpResultCallback& Done)
	{
		const FString Root = BiblioDir();
		TArray<FString> Tous;
		IFileManager::Get().FindFilesRecursive(Tous, *Root, TEXT("*.*"), true, false);
		TArray<TSharedPtr<FJsonValue>> L;
		for (FString P : Tous)
		{
			if (!BiblioExtOk(P)) continue;
			FPaths::MakePathRelativeTo(P, *(Root + TEXT("/")));
			L.Add(MakeShared<FJsonValueString>(P.Replace(TEXT("\\"), TEXT("/"))));
		}
		FString S; FJsonSerializer::Serialize(L, TJsonWriterFactory<>::Create(&S));
		return Text(Done, S, TEXT("application/json; charset=utf-8"));
	}

	static bool FichierBiblio(const FHttpServerRequest& Req, const FHttpResultCallback& Done)
	{
		const FString* P = Req.QueryParams.Find(TEXT("p"));
		if (!P || P->Contains(TEXT("..")) || P->Contains(TEXT(":")) || P->StartsWith(TEXT("/")) || !BiblioExtOk(*P))
			return Error(Done, TEXT("Chemin refusé"));
		const FString E = FPaths::GetExtension(*P).ToLower();
		const FString Type = E == TEXT("png") ? TEXT("image/png") : E == TEXT("webp") ? TEXT("image/webp") : TEXT("application/json; charset=utf-8");
		return File(Done, BiblioDir() / *P, Type);
	}

	static bool Demarrer()
	{
		if (Router) return true;
		FHttpServerModule& M = FHttpServerModule::Get();
		Router = M.GetHttpRouter(Port, true);
		if (!Router)
		{
			UE_LOG(LogEONSalles, Error, TEXT("Éditeur de salles : le port %u est déjà pris (un autre Unreal EON ouvert ?)"), Port);
			return false;
		}
		auto Bind = [](const TCHAR* Path, EHttpServerRequestVerbs Verb, bool (*Fn)(const FHttpServerRequest&, const FHttpResultCallback&))
		{
			Routes.Add(Router->BindRoute(FHttpPath(Path), Verb, FHttpRequestHandler::CreateStatic(Fn)));
		};
		Bind(TEXT("/eon/editeur"), EHttpServerRequestVerbs::VERB_GET, &Page);
		Bind(TEXT("/eon/api/info"), EHttpServerRequestVerbs::VERB_GET, &Info);
		Bind(TEXT("/eon/api/salles"), EHttpServerRequestVerbs::VERB_GET, &ListeSalles);
		Bind(TEXT("/eon/api/salle"), EHttpServerRequestVerbs::VERB_POST, &EcrireSalle);
		Bind(TEXT("/eon/api/biblio"), EHttpServerRequestVerbs::VERB_GET, &ListeBiblio);
		Bind(TEXT("/eon/biblio"), EHttpServerRequestVerbs::VERB_GET, &FichierBiblio);
		M.StartAllListeners();
		UE_LOG(LogEONSalles, Log, TEXT("Éditeur de salles : %s (salles : %s)"), *Url(), *SallesDir());
		return true;
	}

	// Saisie d'un texte (prompt() de la page) : petite fenêtre modale.
	static bool Demander(const FText& Question, const FText& Defaut, FString& Out)
	{
		bool bOk = false;
		TSharedPtr<SEditableTextBox> Box;
		TSharedRef<SWindow> Win = SNew(SWindow).Title(Titre()).SizingRule(ESizingRule::Autosized)
			.SupportsMinimize(false).SupportsMaximize(false);
		TWeakPtr<SWindow> WW = Win;
		auto Fermer = [WW, &bOk](bool bAccepte) { bOk = bAccepte; if (TSharedPtr<SWindow> W = WW.Pin()) W->RequestDestroyWindow(); };
		Win->SetContent(SNew(SBox).Padding(12).MinDesiredWidth(440)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Question).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SAssignNew(Box, SEditableTextBox).Text(Defaut)
				.OnTextCommitted_Lambda([Fermer](const FText&, ETextCommit::Type T) { if (T == ETextCommit::OnEnter) Fermer(true); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[SNew(SButton).Text(FText::FromString(TEXT("OK"))).OnClicked_Lambda([Fermer]() { Fermer(true); return FReply::Handled(); })]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SButton).Text(FText::FromString(TEXT("Annuler"))).OnClicked_Lambda([Fermer]() { Fermer(false); return FReply::Handled(); })]
			]
		]);
		Win->SetWidgetToFocusOnActivate(Box);
		FSlateApplication::Get().AddModalWindow(Win, FGlobalTabmanager::Get()->GetRootWindow());
		if (bOk) Out = Box->GetText().ToString();
		return bOk;
	}

	// alert(), confirm() et prompt() de la page : le navigateur intégré ne les affiche pas seul.
	// Montrés au tick suivant, hors de l'appel du navigateur, qui attend Continue().
	static EWebBrowserDialogEventResponse Dialogue(const TWeakPtr<IWebBrowserDialog>& W)
	{
		TSharedPtr<IWebBrowserDialog> D = W.Pin();
		if (!D) return EWebBrowserDialogEventResponse::Unhandled;
		const EWebBrowserDialogType Type = D->GetType();
		if (Type != EWebBrowserDialogType::Alert && Type != EWebBrowserDialogType::Confirm && Type != EWebBrowserDialogType::Prompt)
			return EWebBrowserDialogEventResponse::Continue;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([D, Type](float)
		{
			if (Type == EWebBrowserDialogType::Alert)
			{
				FMessageDialog::Open(EAppMsgType::Ok, D->GetMessageText(), Titre());
				D->Continue(true);
			}
			else if (Type == EWebBrowserDialogType::Confirm)
			{
				D->Continue(FMessageDialog::Open(EAppMsgType::OkCancel, D->GetMessageText(), Titre()) == EAppReturnType::Ok);
			}
			else
			{
				FString Rep;
				const bool bOk = Demander(D->GetMessageText(), D->GetDefaultPrompt(), Rep);
				D->Continue(bOk, FText::FromString(Rep));
			}
			return false;
		}));
		return EWebBrowserDialogEventResponse::Handled;
	}

	static TSharedRef<SDockTab> Spawn(const FSpawnTabArgs&)
	{
		const bool bServeur = Demarrer();
		TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab).Label(Titre());
		if (!bServeur)
		{
			Tab->SetContent(SNew(SBox).Padding(20)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(FString::Printf(
				TEXT("Le port %u est déjà pris : un autre Unreal EON a sans doute l'éditeur de salles ouvert. Ferme-le, puis rouvre cet onglet."), Port)))]);
			return Tab;
		}
		// SWebBrowser ne charge pas le module web (CEF) lui-même : sans cet appel, l'onglet reste sur le sablier.
		IWebBrowserModule::Get();
		Tab->SetContent(SNew(SWebBrowser)
			.InitialURL(Url())
			.ShowControls(false)
			.ShowAddressBar(false)
			.ShowErrorMessage(true)
			.BackgroundColor(FColor(16, 18, 20))
			.OnShowDialog_Static(&Dialogue));
		return Tab;
	}

	static FDelayedAutoRegisterHelper GEnregistrement(EDelayedRegisterRunPhase::EndOfEngineInit, []()
	{
		if (!GIsEditor || IsRunningCommandlet()) return;
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateStatic(&Spawn))
			.SetDisplayName(Titre())
			.SetTooltipText(FText::FromString(TEXT("Dessiner les salles d'EON (.eonsalle) sans quitter Unreal")))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
	});

	static FAutoConsoleCommand GCmdPanneau(TEXT("eon.salles.panneau"), TEXT("Ouvre l'éditeur de salles dans un onglet d'Unreal"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			if (GIsEditor) FGlobalTabmanager::Get()->TryInvokeTab(TabId);
		}));
}
#endif
