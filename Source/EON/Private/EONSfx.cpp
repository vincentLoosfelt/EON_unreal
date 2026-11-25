#include "EONSfx.h"
#include "EON.h"
#include "EONEnemy.h"
#include "EONStatics.h"
#include "EONTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	TMap<FName, TWeakObjectPtr<USoundBase>> GCache;
	TMap<FName, double> GLast;

	/** Groupes de volume (menu F6, rubrique Son). */
	enum class EGrp : uint8 { Jira, Impacts, Ennemis, Monde };

	/** /Game/EON/Audio/OldSchoolShonen/<Famille>/SFX_Shonen_<Famille>_NN, /Game/EON/Audio/Freesound/SFX_FS_*,
	 *  sinon /Game/EON/Audio/Placeholders/<Name>. */
	FString AssetPath(const FString& Name)
	{
		static const FString Shonen(TEXT("SFX_Shonen_"));
		if (Name.StartsWith(Shonen))
		{
			FString Fam = Name.Mid(Shonen.Len());
			int32 Us = INDEX_NONE;
			if (Fam.FindLastChar(TEXT('_'), Us)) { Fam.LeftInline(Us); }
			return FString::Printf(TEXT("/Game/EON/Audio/OldSchoolShonen/%s/%s.%s"), *Fam, *Name, *Name);
		}
		if (Name.StartsWith(TEXT("SFX_FS_")))
		{
			return FString::Printf(TEXT("/Game/EON/Audio/Freesound/%s.%s"), *Name, *Name);
		}
		return FString::Printf(TEXT("/Game/EON/Audio/Placeholders/%s.%s"), *Name, *Name);
	}

	USoundBase* Find(const FString& Name)
	{
		const FName K(*Name);
		if (TWeakObjectPtr<USoundBase>* W = GCache.Find(K); W && W->IsValid()) { return W->Get(); }
		USoundBase* S = LoadObject<USoundBase>(nullptr, *AssetPath(Name));
		if (!S) { UE_LOG(LogEON, Warning, TEXT("Son introuvable : %s"), *Name); }
		GCache.Add(K, S);
		return S;
	}

	/** Écart minimal entre deux lectures d'une même clé. Faux = trop tôt, on ne joue pas. */
	bool GapOk(const UWorld* World, const FName Key, float MinGap)
	{
		if (MinGap <= 0.f) { return true; }
		const double Now = World->GetRealTimeSeconds();
		if (const double* L = GLast.Find(Key); L && Now - *L >= 0.0 && Now - *L < MinGap) { return false; }
		GLast.Add(Key, Now);
		return true;
	}

	/** Groupe d'un son joué par son nom (appels EONSfx::Play directs). */
	EGrp GroupOfName(const TCHAR* Name)
	{
		const FString N(Name);
		if (N.StartsWith(TEXT("SFX_Jira"))) { return EGrp::Jira; }
		if (N.StartsWith(TEXT("SFX_Ennemi")) || N == TEXT("SFX_Proto_roar")) { return EGrp::Ennemis; }
		return EGrp::Monde;
	}

	float GroupVol(const FEONGameFeelRow& F, EGrp G)
	{
		switch (G)
		{
		case EGrp::Jira:    return F.VolJira;
		case EGrp::Impacts: return F.VolImpacts;
		case EGrp::Ennemis: return F.VolEnnemis;
		default:            return F.VolMonde;
		}
	}

	/** Atténuation des sons placés : pas de baisse sur tout l'écran (16 tuiles), seulement gauche / droite autour de Jira. */
	USoundAttenuation* SpatialAtt()
	{
		static TStrongObjectPtr<USoundAttenuation> Att;
		if (!Att.IsValid())
		{
			USoundAttenuation* A = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("EON_SfxSpatial"));
			FSoundAttenuationSettings& S = A->Attenuation;
			S.bAttenuate = true;
			S.bSpatialize = true;
			S.AttenuationShape = EAttenuationShape::Sphere;
			S.AttenuationShapeExtents = FVector(1500.f, 0.f, 0.f);   // 15 m sans baisse de volume
			S.FalloffDistance = 2000.f;                               // puis silence à 35 m
			S.NonSpatializedRadiusStart = 150.f;                      // tout près de Jira : au centre
			S.NonSpatializedRadiusEnd = 100.f;
			Att.Reset(A);
		}
		return Att.Get();
	}

	/** L'oreille est sur Jira, tournée comme la caméra : un ennemi à droite de l'écran s'entend à droite. */
	void PlaceListener(const UWorld* World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		const APawn* P = PC ? PC->GetPawn() : nullptr;
		if (!P) { return; }
		const float Yaw = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Yaw : 0.f;
		PC->SetAudioListenerOverride(nullptr, P->GetActorLocation(), FRotator(0.f, Yaw, 0.f));
	}

	void PlayIn(const UObject* WorldContext, const UWorld* World, const TCHAR* Name, float Volume, int32 Variants, float PitchVar, EGrp Grp)
	{
		const FEONGameFeelRow F = UEONStatics::GetGameFeel(WorldContext);
		if (!F.Sound) { return; }
		const float Vol = Volume * GroupVol(F, Grp) * F.Vol / 0.5f;   // Vol 0,5 = volume de référence
		if (Vol <= 0.001f) { return; }
		const FString Full = Variants > 0 ? FString::Printf(TEXT("%s_%02d"), Name, FMath::RandRange(1, Variants)) : FString(Name);
		USoundBase* S = Find(Full);
		if (!S) { return; }
		const float Pitch = 1.f + FMath::FRandRange(-PitchVar, PitchVar);
		// Sons des ennemis : placés à leur position (gauche / droite). Le reste : à plat, au centre.
		const AEONEnemy* E = Cast<AEONEnemy>(WorldContext);
		if (E && F.SonSpatial)
		{
			PlaceListener(World);
			UGameplayStatics::PlaySoundAtLocation(WorldContext, S, E->GetActorLocation(), FRotator::ZeroRotator, Vol, Pitch, 0.f, SpatialAtt());
		}
		else
		{
			UGameplayStatics::PlaySound2D(WorldContext, S, Vol, Pitch);
		}
	}

	/** Un évènement du jeu (clé du proto) et le son qui le porte. */
	struct FSfxEvent
	{
		const TCHAR* Key;
		const TCHAR* Asset;   // nom sans le suffixe _NN quand Variants > 0
		int32 Variants;
		float Volume;
		float PitchVar;
		float MinGap;
		EGrp Grp;
	};

	// Table des évènements (25/11 : premier choix, à valider à l'écoute par Vincent :
	// Audio_Sources/OldSchoolShonen/Planche_Ecoute.html reprend cette table).
	const FSfxEvent GEvents[] =
	{
		// --- Combat
		{ TEXT("hit"),     TEXT("SFX_Shonen_Melee"),          11, 0.35f, 0.08f, 0.03f, EGrp::Impacts },   // balle ou coup qui touche un ennemi, armure qui bloque
		{ TEXT("kill"),    TEXT("SFX_Shonen_MeleeGore"),       6, 0.60f, 0.06f, 0.04f, EGrp::Impacts },   // mort d'un ennemi
		{ TEXT("hurt"),    TEXT("SFX_Shonen_Land"),           11, 0.80f, 0.05f, 0.10f, EGrp::Jira },      // Jira touchée
		{ TEXT("dash"),    TEXT("SFX_Shonen_Dodge"),           9, 0.55f, 0.05f, 0.05f, EGrp::Jira },      // dash de Jira
		{ TEXT("run"),     TEXT("SFX_Shonen_Dodge"),           9, 0.30f, 0.08f, 0.20f, EGrp::Jira },      // départ de la course (sfx('dash') du proto, plus doux)
		{ TEXT("fall"),    TEXT("SFX_Shonen_FallWhistle"),     5, 0.70f, 0.03f, 0.30f, EGrp::Jira },      // Jira tombe dans le vide
		{ TEXT("efall"),   TEXT("SFX_Shonen_FallWhistle"),     5, 0.40f, 0.10f, 0.10f, EGrp::Ennemis },   // un ennemi tombe dans le vide
		{ TEXT("explode"), TEXT("SFX_Shonen_Explosion"),      11, 0.70f, 0.06f, 0.05f, EGrp::Impacts },   // explosions (Onibi, Aoe, Faille)
		// --- Pouvoirs
		{ TEXT("slash"),   TEXT("SFX_Shonen_Swing"),           5, 0.55f, 0.06f, 0.05f, EGrp::Jira },      // coup de Lame
		{ TEXT("charge"),  TEXT("SFX_Shonen_AbilityCharge"),  19, 0.45f, 0.04f, 0.25f, EGrp::Jira },      // début de charge (Rafale, Laser), onde maintenue, QTE de l'ultime
		{ TEXT("ready"),   TEXT("SFX_Shonen_Spark"),           4, 0.45f, 0.05f, 0.20f, EGrp::Jira },      // charge pleine
		{ TEXT("spell"),   TEXT("SFX_Shonen_AbilityRelease"), 15, 0.70f, 0.05f, 0.10f, EGrp::Jira },      // onde de choc
		{ TEXT("pulse"),   TEXT("SFX_Shonen_Swing"),           5, 0.25f, 0.10f, 0.15f, EGrp::Jira },      // pulsations de l'onde maintenue (sfx('slash') du proto)
		{ TEXT("mut"),     TEXT("SFX_Shonen_AbilityReady"),    8, 0.80f, 0.03f, 0.50f, EGrp::Jira },      // passage d'un stade de mutation
		{ TEXT("grow"),    TEXT("SFX_Shonen_Land"),           11, 0.70f, 0.03f, 0.50f, EGrp::Jira },      // l'armure pousse
		{ TEXT("holo"),    TEXT("SFX_Shonen_Fly"),             5, 0.55f, 0.05f, 0.30f, EGrp::Jira },      // hologramme leurre
		{ TEXT("ult"),     TEXT("SFX_Shonen_AbilityRelease"), 15, 1.00f, 0.02f, 0.50f, EGrp::Jira },      // ultime (cut-in)
		// --- Freesound CC0 retenus par Vincent (25/11), Audio_Sources/Freesound_CC0/CREDITS.txt
		{ TEXT("alarm"),   TEXT("SFX_FS_Alarm"),               1, 0.70f, 0.02f, 0.50f, EGrp::Monde },     // klaxon de la Faille
		{ TEXT("hiss"),    TEXT("SFX_FS_Hiss"),                2, 0.70f, 0.05f, 0.20f, EGrp::Monde },     // jet d'air
		{ TEXT("hisslong"),TEXT("SFX_FS_HissLong"),            1, 0.60f, 0.03f, 0.50f, EGrp::Monde },     // vidange de la cuve de réveil
		{ TEXT("lift"),    TEXT("SFX_FS_Lift"),                1, 0.55f, 0.03f, 0.50f, EGrp::Monde },     // bouffée de sortie de cuve
		{ TEXT("khurt"),   TEXT("SFX_FS_KidHurt"),             1, 0.70f, 0.04f, 0.30f, EGrp::Monde },     // le Cartographe prend un coup
		{ TEXT("item"),    TEXT("SFX_FS_Item"),                1, 0.70f, 0.00f, 0.20f, EGrp::Monde },     // objet débloqué (glockenspiel)
		{ TEXT("pickup"),  TEXT("SFX_FS_Pickup"),              1, 0.45f, 0.10f, 0.04f, EGrp::Monde },     // ramassage (énergie, cœur, données, scrap)
		{ TEXT("whip"),    TEXT("SFX_FS_Whip"),                3, 0.60f, 0.06f, 0.08f, EGrp::Ennemis },   // tentacule du boss, élan du Kama
		{ TEXT("glass"),   TEXT("SFX_FS_Glass"),               1, 0.90f, 0.02f, 0.50f, EGrp::Monde },     // la cuve du boss éclate
		{ TEXT("step"),    TEXT("SFX_FS_Step"),                7, 0.25f, 0.06f, 0.12f, EGrp::Jira },      // pas de Jira (EONSfx::Steps)
		// --- 2e série Freesound CC0 (25/11)
		{ TEXT("door"),    TEXT("SFX_FS_Door"),                1, 0.65f, 0.05f, 0.15f, EGrp::Monde },     // porte qui s'ouvre (aérosols)
		{ TEXT("card"),    TEXT("SFX_FS_Card"),                1, 0.70f, 0.00f, 0.20f, EGrp::Monde },     // carte de niveau (barres de carillon)
		{ TEXT("deny"),    TEXT("SFX_FS_Deny"),                1, 0.55f, 0.00f, 0.40f, EGrp::Monde },     // refusé : infestation, porte rouge, QTE raté
		{ TEXT("mortar"),  TEXT("SFX_FS_Mortar"),              1, 0.55f, 0.08f, 0.06f, EGrp::Ennemis },   // départ d'un obus de mortier (pop + M203)
		// --- Tirs taillés dans Old-School Shonen (choix de Vincent le 25/11)
		{ TEXT("shoot"),   TEXT("SFX_Shonen_Shot"),            3, 0.45f, 0.07f, 0.03f, EGrp::Jira },      // tir standard (Throw 1-3, attaque)
		{ TEXT("burst"),   TEXT("SFX_Shonen_Burst"),           4, 0.40f, 0.08f, 0.02f, EGrp::Jira },      // coup de Rafale (Throw 1-4, 0,15 s)
		{ TEXT("eshot"),   TEXT("SFX_Shonen_EShot"),           4, 0.40f, 0.08f, 0.08f, EGrp::Ennemis },   // tir organique ennemi (Melee Gore 2-5)
		{ TEXT("tele"),    TEXT("SFX_Shonen_Tele"),            4, 0.50f, 0.00f, 0.15f, EGrp::Ennemis },   // annonce d'attaque, vague, montée de niveau (Ability Ready 2-5)
		// --- Terminal du C.O.M (joués en 2D, donc aussi pendant la pause)
		{ TEXT("menu_move"), TEXT("SFX_FS_MenuMove"),          1, 0.50f, 0.06f, 0.04f, EGrp::Monde },     // onglet ou ligne (touche mécanique)
		{ TEXT("menu_open"), TEXT("SFX_FS_MenuOpen"),          3, 0.55f, 0.00f, 0.30f, EGrp::Monde },     // ouverture (télé et moniteur cathodiques)
	};

	/** Distance parcourue depuis le dernier pas, par acteur. */
	TMap<TWeakObjectPtr<const AActor>, float> GStepAcc;
}

void EONSfx::Play(const UObject* WorldContext, const TCHAR* Name, float Volume, int32 Variants, float PitchVar, float MinGap)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld()) { return; }
	if (!GapOk(World, FName(Name), MinGap)) { return; }
	PlayIn(WorldContext, World, Name, Volume, Variants, PitchVar, GroupOfName(Name));
}

void EONSfx::Event(const UObject* WorldContext, const TCHAR* Key, float VolMul)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld()) { return; }
	for (const FSfxEvent& E : GEvents)
	{
		if (FCString::Strcmp(E.Key, Key) == 0)
		{
			// L'écart minimal suit la clé de l'évènement (« dash » et « run » partagent le même son sans se bloquer).
			if (GapOk(World, FName(*FString::Printf(TEXT("ev:%s"), Key)), E.MinGap))
			{
				PlayIn(WorldContext, World, E.Asset, E.Volume * VolMul, E.Variants, E.PitchVar, E.Grp);
			}
			return;
		}
	}
	UE_LOG(LogEON, Warning, TEXT("Évènement sonore inconnu : %s"), Key);
}

void EONSfx::Steps(const AActor* Actor, float Dt, bool bOnGround, bool bRunning)
{
	if (!Actor) { return; }
	float& Acc = GStepAcc.FindOrAdd(Actor);
	const float Speed = Actor->GetVelocity().Size2D();
	if (!bOnGround || Speed < 30.f) { Acc = 0.f; return; }
	// Un pas tous les 110 cm en marchant (2,8 m/s : 2,5 pas/s), 150 cm en courant ; le premier pas part tout de suite.
	const float Stride = bRunning ? 150.f : 110.f;
	if (Acc <= 0.f || Acc >= Stride)
	{
		Acc = FMath::Max(0.f, Acc - Stride) + 0.01f;
		Event(Actor, TEXT("step"), bRunning ? 1.5f : 1.f);
	}
	Acc += Speed * Dt;
}
