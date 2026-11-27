#include "EONFx.h"
#include "EONTypes.h"
#include "EONStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"

namespace
{
	/** Dilatation propre aux effets : 1, ou 0,15 / 0,0001 pendant un arrêt sur image (le gel global vaut 0,0001). */
	float GFxDilation = 1.f;
	/** Arrêt sur image en cours (SetHitstop). La dilatation des effets est recalculée à chaque image d'après le gel
	 *  RÉELLEMENT appliqué au monde (FxPreTick) : le gel global ne prend effet qu'à l'image suivante ; poser tout de
	 *  suite 0,15 / 0,0001 faisait vieillir les effets de plusieurs secondes d'un coup (fumée morte à 2,6 s au lieu de 4). */
	bool GHitstop = false;
	void FxPreTick(UWorld* W, ELevelTick, float Dt);
	FDelegateHandle GFxHook;
	void EnsureHook() { if (!GFxHook.IsValid()) { GFxHook = FWorldDelegates::OnWorldPreActorTick.AddStatic(&FxPreTick); } }
	TArray<TWeakObjectPtr<UNiagaraComponent>> GLive;

	UNiagaraSystem* Sys(int32 Which)
	{
		static const TCHAR* Paths[] = {
			TEXT("/Game/EON/VFX/NS_EON_Gerbe.NS_EON_Gerbe"),
			TEXT("/Game/EON/VFX/NS_EON_GerbeDir.NS_EON_GerbeDir"),
			TEXT("/Game/EON/VFX/NS_EON_Anneau.NS_EON_Anneau"),
			TEXT("/Game/EON/VFX/NS_EON_Fumee.NS_EON_Fumee"),
			TEXT("/Game/EON/VFX/NS_EON_FumeeLobes.NS_EON_FumeeLobes"),
			TEXT("/Game/EON/VFX/NS_EON_Feu.NS_EON_Feu"),
			TEXT("/Game/EON/VFX/NS_EON_FeuLobes.NS_EON_FeuLobes"),
			TEXT("/Game/EON/VFX/NS_EON_FumeeExplo.NS_EON_FumeeExplo"),
			TEXT("/Game/EON/VFX/NS_EON_FumeeExploLobes.NS_EON_FumeeExploLobes"),
			TEXT("/Game/EON/VFX/NS_EON_Etincelles.NS_EON_Etincelles") };
		static TStrongObjectPtr<UNiagaraSystem> Cache[10];
		if (!Cache[Which].IsValid()) { Cache[Which].Reset(LoadObject<UNiagaraSystem>(nullptr, Paths[Which])); }
		return Cache[Which].Get();
	}

	UNiagaraComponent* Start(const UObject* Ctx, int32 Which, const FVector& At)
	{
		UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		UNiagaraSystem* S = Sys(Which);
		if (!W || !S || W->GetNetMode() == NM_DedicatedServer) { return nullptr; }
		UNiagaraComponent* C = UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, S, At, FRotator::ZeroRotator, FVector::OneVector, true, false);
		if (C)
		{
			// Solo : la dilatation propre ne marche qu'ainsi ; elle suit l'arrêt sur image (SetHitstop).
			C->SetForceSolo(true);
			C->SetCustomTimeDilation(GFxDilation);
			EnsureHook();
			GLive.RemoveAllSwap([](const TWeakObjectPtr<UNiagaraComponent>& P) { return !P.IsValid(); });
			GLive.Add(C);
		}
		return C;
	}

	void Pas12(UNiagaraComponent* C);
	void Common(UNiagaraComponent* C, int32 N, const FLinearColor& Col, float Life, float SizePx, float Drag)
	{
		C->SetVariableInt(TEXT("User.Count"), N);
		C->SetVariableLinearColor(TEXT("User.Color"), Col);
		C->SetVariableFloat(TEXT("User.Life"), Life);
		// Étoile à 4 branches (M_EON_Particule, Vincent, 26/11 : plus de carré pixel) : elle ne remplit qu'une partie
		// de son sprite, qui est donc 2,2 fois plus grand que l'ancien carré.
		const float Sz = EON::PxToCm(SizePx) * 2.2f;
		C->SetVariableVec2(TEXT("User.Size"), FVector2D(Sz, Sz));
		C->SetVariableFloat(TEXT("User.Drag"), Drag);
		C->Activate(true);
		Pas12(C);
	}
}

FLinearColor EONFx::Hex(const TCHAR* H)
{
	return FLinearColor(FColor::FromHex(H));
}

void EONFx::Gerbe(const UObject* Ctx, const FVector& At, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float SizePx, float Drag, float RadiusPx)
{
	if (N <= 0) { return; }
	if (UNiagaraComponent* C = Start(Ctx, 0, At))
	{
		C->SetVariableFloat(TEXT("User.Speed"), EON::PxToCm(SpeedPx));
		C->SetVariableFloat(TEXT("User.Radius"), EON::PxToCm(RadiusPx));
		Common(C, N, Col, Life, SizePx, Drag);
	}
}

void EONFx::GerbeDir(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float SizePx, float Drag)
{
	if (N <= 0) { return; }
	const FVector D = Dir.GetSafeNormal();
	if (D.IsNearlyZero()) { Gerbe(Ctx, At, N, Col, SpeedPx, Life, SizePx, Drag); return; }
	if (UNiagaraComponent* C = Start(Ctx, 1, At))
	{
		C->SetVariableFloat(TEXT("User.Speed"), EON::PxToCm(SpeedPx));
		C->SetVariableFloat(TEXT("User.Radius"), 0.f);
		// Un peu de hauteur : à plat, la moitié du cône rentrerait dans le sol.
		C->SetVariableVec3(TEXT("User.Dir"), (D + FVector(0.f, 0.f, 0.35f)).GetSafeNormal());
		Common(C, N, Col, Life, SizePx, Drag);
	}
}

void EONFx::Gerbe2(const UObject* Ctx, const FVector& At, int32 N, int32 Every, const FLinearColor& A, const FLinearColor& B, float SpeedPx, float Life, float SizePx, float Drag, float RadiusPx)
{
	const int32 Nb = Every > 0 ? (N + Every - 1) / Every : 0;
	Gerbe(Ctx, At, N - Nb, A, SpeedPx, Life, SizePx, Drag, RadiusPx);
	Gerbe(Ctx, At, Nb, B, SpeedPx, Life, SizePx, Drag, RadiusPx);
}

void EONFx::Etincelles(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float WidthPx, float LenPx, float Drag)
{
	const FVector D = Dir.GetSafeNormal();
	if (N <= 0 || D.IsNearlyZero()) { return; }
	if (UNiagaraComponent* C = Start(Ctx, 9, At))
	{
		C->SetVariableFloat(TEXT("User.Speed"), EON::PxToCm(SpeedPx));
		C->SetVariableFloat(TEXT("User.Radius"), 0.f);
		C->SetVariableVec3(TEXT("User.Dir"), D);
		C->SetVariableInt(TEXT("User.Count"), N);
		C->SetVariableLinearColor(TEXT("User.Color"), Col);
		C->SetVariableFloat(TEXT("User.Life"), Life);
		// Alignées sur la vitesse : X = largeur, Y = longueur (dans le sens du mouvement).
		C->SetVariableVec2(TEXT("User.Size"), FVector2D(EON::PxToCm(WidthPx), EON::PxToCm(LenPx)));
		C->SetVariableFloat(TEXT("User.Drag"), Drag);
		C->Activate(true);
		Pas12(C);
	}
}

void EONFx::GerbeDir2(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, int32 Every, const FLinearColor& A, const FLinearColor& B, float SpeedPx, float Life, float SizePx, float Drag)
{
	const int32 Nb = Every > 0 ? (N + Every - 1) / Every : 0;
	GerbeDir(Ctx, At, Dir, N - Nb, A, SpeedPx, Life, SizePx, Drag);
	GerbeDir(Ctx, At, Dir, Nb, B, SpeedPx, Life, SizePx, Drag);
}

// ---------------------------------------------------------------- Fumée toon à 12 i/s
namespace
{
	/** Fumées vivantes et temps accumulé depuis leur dernier bond. */
	struct FPas { TWeakObjectPtr<UNiagaraComponent> C; float Acc = 0.f; float Reste = 10.f; };
	TArray<FPas> GPas;
	/** Vincent, 26/11 : tout ce qui est animé est à 12 i/s. Gerbes, étincelles et anneaux Niagara avancent par
	 *  bonds de 1/12 s, comme la fumée (PasFumee). */
	void Pas12(UNiagaraComponent* C)
	{
		if (!C) { return; }
		FPas P; P.C = C; P.Acc = 1.f / 12.f; P.Reste = 6.f;
		GPas.Add(P);
	}
	TAutoConsoleVariable<int32> CVarFumee12(TEXT("eon.fx.fumee12"), 1, TEXT("Fumée toon animée à 12 i/s (1) ou fluide (0)."));

	/** Avant le tick des acteurs : une fumée ne bouge que lorsqu'elle a accumulé 1/12 s ; ce tick-là, sa dilatation
	 *  rattrape tout le temps accumulé, sinon elle est quasi nulle (image tenue). */
	void PasFumee(UWorld* W, float Dt)
	{
		if (Dt <= 0.f) { return; }
		for (int32 i = GPas.Num() - 1; i >= 0; --i)
		{
			UNiagaraComponent* C = GPas[i].C.Get();
			if (!C) { GPas.RemoveAtSwap(i); continue; }
			if (C->GetWorld() != W) { continue; }
			// Sécurité : une fumée qui dure bien plus que sa vie (système endormi) est supprimée, jamais laissée figée.
			GPas[i].Reste -= Dt;
			if (GPas[i].Reste <= 0.f) { UE_LOG(LogTemp, Display, TEXT("EON fumée figée supprimée")); C->DestroyComponent(); GPas.RemoveAtSwap(i); continue; }
			if (CVarFumee12.GetValueOnGameThread() == 0) { C->SetCustomTimeDilation(GFxDilation); continue; }
			GPas[i].Acc += Dt * GFxDilation;
			if (GPas[i].Acc >= 1.f / 12.f) { C->SetCustomTimeDilation(GPas[i].Acc / Dt); GPas[i].Acc = 0.f; }
			// Jamais 0 : à dilatation nulle, Niagara endort le système et ses sphères restent figées pour toujours.
			else { C->SetCustomTimeDilation(1e-4f); }
		}
	}
}

namespace
{
	/** Amas de sphères à 12 i/s : noyau (système Core, 1 sphère sur 6, au moins 2) + lobes soudés à sa surface (Lobes).
	 *  Fumée : NS_EON_Fumee/FumeeLobes ; feu des explosions : NS_EON_Feu/FeuLobes (même principe, Vincent, 24/11). */
	void Amas(const UObject* Ctx, const FVector& At, int32 N, const FLinearColor& Col, float Life, float Scale, int32 Core, int32 Lobes)
	{
	const int32 NNoyau = FMath::Max(2, N / 6);
	for (int32 Which : { Core, Lobes })
	{
		UNiagaraComponent* C = Start(Ctx, Which, At);
		if (!C) { continue; }
		C->SetWorldScale3D(FVector(FMath::Max(0.05f, Scale)));
		C->SetVariableInt(TEXT("User.Count"), Which == Core ? NNoyau : N);
		C->SetVariableLinearColor(TEXT("User.Color"), Col);
		// Centre de la masse (monde), pour courber les normales vers celles du volume d'ensemble (M_EON_FumeeToon).
		const FVector Centre = At + FVector(0.f, 0.f, 35.f * Scale);
		C->SetVariableFloat(TEXT("User.CX"), Centre.X);
		C->SetVariableFloat(TEXT("User.CY"), Centre.Y);
		C->SetVariableFloat(TEXT("User.CZ"), Centre.Z);
		// Le noyau vit un peu moins longtemps : il rétrécit le premier, la masse s'ouvre au centre.
		C->SetVariableFloat(TEXT("User.Life"), Which == Core ? Life * 0.85f : Life);
		// Stencil 3 : celluloïd détouré seulement au bord de la masse (lobes fusionnés, M_PP_EON_Trait).
		C->SetRenderCustomDepth(true);
		C->SetCustomDepthStencilValue(3);
		C->Activate(true);
		// Première image tout de suite, puis bonds de 1/12 s.
		FPas Pas; Pas.C = C; Pas.Acc = 1.f / 12.f; Pas.Reste = Life * 3.f + 1.f;
		GPas.Add(Pas);
	}
	}
}

void EONFx::Fumee(const UObject* Ctx, const FVector& At, int32 N, const FLinearColor& Col, float Life, float Scale)
{
	if (N <= 0) { return; }
	UE_LOG(LogTemp, Display, TEXT("EON fumée %s n=%d s=%.2f"), *At.ToString(), N, Scale);
	Amas(Ctx, At, N, Col, Life, Scale, 3, 4);
}

void EONFx::Anneau(const UObject* Ctx, const FVector& At, float RadiusPx, float Life, const FLinearColor& Col)
{
	if (UNiagaraComponent* C = Start(Ctx, 2, At + FVector(0.f, 0.f, 3.f)))
	{
		const float D = EON::PxToCm(RadiusPx) * 2.f;
		C->SetVariableLinearColor(TEXT("User.Color"), Col);
		C->SetVariableFloat(TEXT("User.Life"), Life);
		C->SetVariableVec2(TEXT("User.Size"), FVector2D(D, D));
		C->Activate(true);
		Pas12(C);
	}
}

// ---------------------------------------------------------------- Effets en maillage (24/11, effets « ABS »)
#include "EONFxPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	AEONFxPiece* SpawnPiece(const UObject* Ctx, const FVector& At, float Life)
	{
		UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		if (!W || W->GetNetMode() == NM_DedicatedServer) { return nullptr; }
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEONFxPiece* F = W->SpawnActor<AEONFxPiece>(AEONFxPiece::StaticClass(), At, FRotator::ZeroRotator, P);
		if (F) { F->Life = FMath::Max(0.01f, Life); F->CustomTimeDilation = GFxDilation; EnsureHook(); }
		return F;
	}
}

#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
namespace
{
	void AppliqueDilatation(UWorld* W, float D)
	{
		GFxDilation = D;
		for (const TWeakObjectPtr<UNiagaraComponent>& P : GLive) { if (UNiagaraComponent* C = P.Get()) { C->SetCustomTimeDilation(D); } }
		GLive.RemoveAllSwap([](const TWeakObjectPtr<UNiagaraComponent>& P) { return !P.IsValid(); });
		if (W) { for (TActorIterator<AEONFxPiece> It(W); It; ++It) { It->CustomTimeDilation = D; } }
	}

	void FxPreTick(UWorld* W, ELevelTick, float Dt)
	{
		if (!W || !W->IsGameWorld()) { return; }
		// Effets à 15 % pendant le gel : 0,15 ÷ dilatation du monde de CETTE image.
		const AWorldSettings* Ws = W->GetWorldSettings();
		const float Eff = Ws ? FMath::Max(1e-6f, Ws->GetEffectiveTimeDilation()) : 1.f;
		const float D = GHitstop ? 0.15f / Eff : 1.f;
		if (!FMath::IsNearlyEqual(D, GFxDilation, D * 0.01f)) { AppliqueDilatation(W, D); }
		PasFumee(W, Dt);
	}
}

void EONFx::SetHitstop(UWorld* W, bool bOn)
{
	GHitstop = bOn;
	// Le gel global (ou sa fin) ne s'applique qu'à l'image suivante : pour le reste de celle-ci, le monde tourne encore
	// à la vitesse d'avant. FxPreTick recalcule dès l'image suivante.
	AppliqueDilatation(W, bOn ? 0.15f : 1.f);
}

UMaterialInstanceDynamic* EONFx::Glow(UObject* Outer, EForme Forme, const FLinearColor& Col, float Intensite)
{
	static TStrongObjectPtr<UMaterialInterface> Base;
	if (!Base.IsValid()) { Base.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_EON_Glow.M_EON_Glow"))); }
	if (!Base.IsValid()) { return nullptr; }
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base.Get(), Outer);
	M->SetScalarParameterValue(TEXT("Forme"), (float)(uint8)Forme);
	M->SetVectorParameterValue(TEXT("Color"), Col);
	M->SetScalarParameterValue(TEXT("Intensite"), Intensite);
	return M;
}

void EONFx::Lumiere(const UObject* Ctx, const FVector& At, const FLinearColor& Col, float Intensity, float RadiusPx, float Life)
{
	if (AEONFxPiece* F = SpawnPiece(Ctx, At, Life)) { F->AddLight(Col, Intensity, EON::PxToCm(RadiusPx)); }
}

AEONFxPiece* EONFx::Charge(const UObject* Ctx, const FVector& At, float RadiusCm, const FLinearColor& Col, float Duree)
{
	static TStrongObjectPtr<UStaticMesh> Sphere;
	static TStrongObjectPtr<UMaterialInterface> DomeMat;
	if (!Sphere.IsValid()) { Sphere.Reset(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/EON/VFX/SM_EON_FumeeSphere.SM_EON_FumeeSphere"))); }
	if (!DomeMat.IsValid()) { DomeMat.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/VFX/M_EON_Dome.M_EON_Dome"))); }
	// Vincent, 24/11 : la boule part d'une sphère de 1 cm et grossit jusqu'à 50 cm (diamètres), puis explose.
	const float R0 = 0.5f, R1 = 25.f;
	AEONFxPiece* F = SpawnPiece(Ctx, At + FVector(0.f, 0.f, R1 * 0.7f), FMath::Max(0.05f, Duree));
	if (!F || !Sphere.IsValid() || !DomeMat.IsValid()) { return F; }
	F->bPas12 = true;
	// Opacite reste à 1 (âge 0 pour le matériau) : cœur blanc, fin bord coloré, pas d'érosion.
	F->bNoFade = true;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(DomeMat.Get(), F);
	M->SetVectorParameterValue(TEXT("Bord"), Col);
	// Bulles proportionnées à la boule (≈ 4 sur le rayon), relief ≈ 8 % du rayon.
	M->SetScalarParameterValue(TEXT("BulleFreq"), 2.f / R1);
	M->SetScalarParameterValue(TEXT("BulleAmp"), R1 * 0.08f);
	// Icosphère de rayon 50 cm.
	UStaticMeshComponent* C = F->AddMesh(Sphere.Get(), M, FVector(R0 / 50.f));
	F->Grow0 = FVector(R0 / 50.f); F->Grow1 = FVector(R1 / 50.f); F->GrowT = Duree;
	// Trait celluloïd au bord seulement (même stencil que la fumée, M_PP_EON_Trait).
	C->SetRenderCustomDepth(true);
	C->SetCustomDepthStencilValue(3);
	// Lueur faible pendant la charge (seule la détonation éclaire vraiment).
	F->AddLight(FMath::Lerp(FLinearColor::White, Col, 0.5f), 4000.f, R1 * 4.f)->SetIntensity(0.f);   // croît avec la boule (AEONFxPiece::Tick)
	F->bNoFade = true;
	return F;
}

TAutoConsoleVariable<float> CVarFlash(TEXT("eon.fx.flash"), 4.0e6f, TEXT("Intensité du flash des explosions (lumière à ombres nettes)."));

void EONFx::Flash(const UObject* Ctx, const FVector& At, const FLinearColor& Col, float RadiusCm)
{
	// 2 images à 12 i/s : la lumière décroît au carré de la vie restante par bonds (1 puis 1/4).
	if (AEONFxPiece* F = SpawnPiece(Ctx, At, 2.f / 12.f))
	{
		F->bPas12 = true;
		F->AddLight(Col, CVarFlash.GetValueOnGameThread(), RadiusCm, true);
	}
}

namespace
{
	/** Détonation : flash à ombres noires, boule de feu en amas, lumière, fumée de 4 s. */
	void Eclat(const UObject* Ctx, const FVector& At, float R, const FLinearColor& Col, bool bFumee)
	{
		UE_LOG(LogTemp, Display, TEXT("EON explosion %s r=%.0f"), *At.ToString(), R);
		EONFx::GrosCoup(Ctx, At, FMath::Lerp(Col, FLinearColor(1.f, 0.55f, 0.15f), 0.5f), 2);   // BOOM
		// Flash : lumière énorme à hauteur de la boule, ombres noires nettes sur tout ce qui l'entoure.
		EONFx::Flash(Ctx, At + FVector(0.f, 0.f, 40.f), FMath::Lerp(FLinearColor::White, Col, 0.2f), FMath::Max(1500.f, R * 12.f));
		// Boule de feu = le principe de la fumée (Vincent, 24/11) : amas de sphères NS_EON_Feu, cœur blanc aveuglant, bord
		// couleur Col qui envahit l'amas, bulles, érosion à bords nets, 12 i/s. Vie : 0,6 s (petite) à 0,95 s (grosse).
		const float Life = FMath::Clamp(0.5f + R / 300.f, 0.6f, 0.95f);
		Amas(Ctx, At, FMath::Clamp(FMath::RoundToInt(R / 4.f), 18, 60), Col, Life, FMath::Clamp(R / 80.f, 0.3f, 3.5f), 5, 6);
		// Seuls les gros effets éclairent la scène (Vincent, 24/11) : éclair blanc-orange, rayon 3 × celui de la boule.
		EONFx::Lumiere(Ctx, At + FVector(0.f, 0.f, R * 0.5f), FMath::Lerp(FLinearColor::White, Col, 0.5f), 45000.f, EON::CmToPx(R * 3.f), Life * 0.7f);
		// Fumée façon Akira, cachée par le feu au début, révélée quand il s'érode.
		if (bFumee) { EONFx::FumeeAkira(Ctx, At, R, EONFx::Hex(TEXT("2e2630"))); }
	}
}

void EONFx::FumeeAkira(const UObject* Ctx, const FVector& At, float RadiusCm, const FLinearColor& Col, float Life)
{
	// 2 fois plus de sphères que la poussière (NS_EON_Fumee), qui s'excentrent depuis le centre et dérivent vers le haut.
	const float R = FMath::Max(10.f, RadiusCm);
	const int32 N = FMath::Clamp(FMath::RoundToInt(R / 2.f), 36, 120);
	UE_LOG(LogTemp, Display, TEXT("EON fumée Akira %s n=%d"), *At.ToCompactString(), N);
	Amas(Ctx, At, N, Col, Life, FMath::Clamp(R / 75.f, 0.6f, 3.5f), 7, 8);
}

void EONFx::Explosion(const UObject* Ctx, const FVector& At, float RadiusCm, const FLinearColor& Col, bool bFumee, float ChargeS)
{
	const float R = FMath::Max(10.f, RadiusCm);
	if (ChargeS > 0.f)
	{
		if (AEONFxPiece* F = Charge(Ctx, At, R, Col, ChargeS))
		{
			// La boule de charge lance l'éclat à la fin de sa vie (elle sert de contexte : l'appelant peut avoir disparu).
			F->OnEnd = [At, R, Col, bFumee](AEONFxPiece* P) { Eclat(P, At, R, Col, bFumee); };
			return;
		}
	}
	Eclat(Ctx, At, R, Col, bFumee);
}

AEONFxPiece* EONFx::Fantome(const UObject* Ctx, USkeletalMeshComponent* Src, const FLinearColor& Col, float Alpha, float Life)
{
	if (!Src) { return nullptr; }
	AEONFxPiece* F = SpawnPiece(Ctx, Src->GetComponentLocation(), Life);
	if (!F) { return nullptr; }
	F->Opacity0 = Alpha;
	UMaterialInstanceDynamic* M = Glow(F, EForme::Uni, Col, 1.f);
	if (M) { M->SetScalarParameterValue(TEXT("Opacite"), Alpha); }
	F->AddGhost(Src, M);
	return F;
}

AEONFxPiece* EONFx::Rayon(const UObject* Ctx, const FVector& From, const FVector& To, float WidthCm, const FLinearColor& Col, float Life, float Intensite)
{
	const FVector D = To - From;
	const float L = D.Size();
	if (L < 1.f) { return nullptr; }
	AEONFxPiece* F = SpawnPiece(Ctx, (From + To) * 0.5f, Life);
	if (!F) { return nullptr; }
	// Ruban face caméra : axe X le long du rayon, normale du plan (Z) vers la caméra.
	FVector Cam = FVector::UpVector;
	if (const APlayerCameraManager* Pc = UGameplayStatics::GetPlayerCameraManager(F, 0)) { Cam = (Pc->GetCameraLocation() - F->GetActorLocation()).GetSafeNormal(); }
	F->SetActorRotation(FRotationMatrix::MakeFromXZ(D / L, Cam).Rotator());
	F->AddPlane(Glow(F, EForme::Uni, Col, Intensite), FVector::ZeroVector, 0.f, L, WidthCm);
	F->AddPlane(Glow(F, EForme::Uni, FLinearColor::White, Intensite * 1.3f), FVector(0.f, 0.f, 0.5f), 0.f, L, WidthCm * 0.35f);
	return F;
}

void EONFx::Douille(const UObject* Ctx, const FVector& At, const FVector& Vel, float FloorZ)
{
	AEONFxPiece* F = SpawnPiece(Ctx, At, 10.f);
	if (!F) { return; }
	F->bShell = true;
	F->ShellVel = Vel;
	F->ShellPos = At;
	F->ShellYaw = F->GetActorRotation().Yaw;
	F->FloorZ = FloorZ;
	static TStrongObjectPtr<UStaticMesh> Cyl;
	static TStrongObjectPtr<UMaterialInterface> Flat;
	if (!Cyl.IsValid()) { Cyl.Reset(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))); }
	// Celluloïd (Vincent, 24/11) : même matériau toon que le décor (M_EON_BG, teinte BC1) et trait du post-process.
	if (!Flat.IsValid()) { Flat.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"))); }
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(F);
	C->SetStaticMesh(Cyl.Get());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetupAttachment(F->GetRootComponent());
	// Cylindre moteur : 100 cm de diamètre, 100 cm de haut, axe Z. Douille couchée : 2 × 1 px (12,5 × 6,25 cm).
	C->SetRelativeLocationAndRotation(FVector(0.f, 0.f, EON::PxToCm(0.5f)), FRotator(90.f, 0.f, 0.f));
	C->SetRelativeScale3D(FVector(EON::PxToCm(1.f) / 100.f, EON::PxToCm(1.f) / 100.f, EON::PxToCm(2.f) / 100.f));
	if (Flat.IsValid())
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Flat.Get(), F);
		M->SetVectorParameterValue(TEXT("BC1"), Hex(TEXT("ffc23d")));
		C->SetMaterial(0, M);
	}
	C->RegisterComponent();
	UEONStatics::MarkCelComponent(C);
	AEONFxPiece::Shells.RemoveAll([](const TWeakObjectPtr<AEONFxPiece>& P) { return !P.IsValid(); });
	AEONFxPiece::Shells.Add(F);
	while (AEONFxPiece::Shells.Num() > 80) { if (AEONFxPiece* Old = AEONFxPiece::Shells[0].Get()) { Old->Destroy(); } AEONFxPiece::Shells.RemoveAt(0); }
}

AEONFxPiece* EONFx::Sol(const UObject* Ctx, EForme Forme, const FVector& At, float Yaw, float SizeXPx, float SizeYPx, const FLinearColor& Col, float Life,
	float Clignote, bool bFill, float Angle, float Graine, float Intensite)
{
	AEONFxPiece* F = SpawnPiece(Ctx, At + FVector(0.f, 0.f, 2.f), Life);
	if (!F) { return nullptr; }
	UMaterialInstanceDynamic* M = Glow(F, Forme, Col, Intensite);
	if (M)
	{
		M->SetScalarParameterValue(TEXT("Clignote"), Clignote);
		M->SetScalarParameterValue(TEXT("Angle"), Angle);
		M->SetScalarParameterValue(TEXT("Graine"), Graine);
		M->SetScalarParameterValue(TEXT("Remplissage"), bFill ? 0.f : 1.f);
	}
	F->bFill = bFill;
	F->AddPlane(M, FVector::ZeroVector, Yaw, EON::PxToCm(SizeXPx), EON::PxToCm(SizeYPx));
	return F;
}

namespace
{
	TAutoConsoleVariable<float> CVarSangIntensite(TEXT("eon.fx.sang.intensite"), 1.f, TEXT("Émissif des planches de sang (giclée, taches)."));

	UMaterialInstanceDynamic* PlancheMid(UObject* Outer, const TCHAR* Tex, float Cols, float Rows, const FLinearColor& Col)
	{
		static TStrongObjectPtr<UMaterialInterface> Base;
		if (!Base.IsValid()) { Base.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/VFX/M_EON_Planche.M_EON_Planche"))); }
		UTexture* T = LoadObject<UTexture>(nullptr, Tex);
		if (!Base.IsValid() || !T) { return nullptr; }
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base.Get(), Outer);
		M->SetTextureParameterValue(TEXT("Planche"), T);
		M->SetScalarParameterValue(TEXT("Cols"), Cols);
		M->SetScalarParameterValue(TEXT("Rows"), Rows);
		M->SetVectorParameterValue(TEXT("Color"), Col);
		M->SetScalarParameterValue(TEXT("Intensite"), CVarSangIntensite.GetValueOnGameThread());
		return M;
	}
	TArray<TWeakObjectPtr<AEONFxPiece>> GTaches;
}

namespace
{
	/** Planche 4 × 2 jouée face caméra : Anchor = décalage du centre de l'image le long de son +X (fraction du côté). */
	void PlancheFace(const UObject* Ctx, const TCHAR* Tex, const FVector& At, const FVector& Dir, float SizeCm, const FLinearColor& Col, float Life, float LiftCm, float Anchor)
	{
		AEONFxPiece* F = SpawnPiece(Ctx, At, Life);
		if (!F) { return; }
		UMaterialInstanceDynamic* M = PlancheMid(F, Tex, 4.f, 2.f, Col);
		if (!M) { F->Destroy(); return; }
		F->bNoFade = true;
		F->bFlip = true; F->FlipN = 8;
		F->FaceDir = Dir.GetSafeNormal();
		F->FaceSize = SizeCm;
		F->FaceAnchor = Anchor;
		F->FaceLift = LiftCm;
		F->FacePlane = F->AddPlane(M, FVector::ZeroVector, 0.f, SizeCm, SizeCm);
		F->UpdateFace();
	}
}

void EONFx::FlashTir(const UObject* Ctx, const FVector& At, const FVector& Dir, const FLinearColor& Col, float SizePx, float LightRadiusPx, bool bUneImage)
{
	// Vincent, 25/11 : « émission blanche à halo » de la couleur de l'arme. Deux couches de la même étoile, tenues une
	// image à 12 i/s : derrière, le halo (étoile ×1,7, couleur de l'arme) ; devant, le cœur blanc, un peu vers la caméra.
	static int32 Dernier = -1;
	int32 Img = FMath::RandRange(0, 2);
	if (Img >= Dernier) { ++Img; }
	Dernier = Img;
	const float S = EON::PxToCm(SizePx);
	auto Couche = [&](const FLinearColor& Teinte, float Taille, float VersCamera) -> AEONFxPiece*
	{
		AEONFxPiece* F = SpawnPiece(Ctx, At, 1.f / 12.f);
		if (!F) { return nullptr; }
		UMaterialInstanceDynamic* M = PlancheMid(F, TEXT("/Game/EON/VFX/Planches/T_EON_VFX_P4_Flash.T_EON_VFX_P4_Flash"), 2.f, 2.f, Teinte);
		if (!M) { F->Destroy(); return nullptr; }
		M->SetScalarParameterValue(TEXT("Intensite"), FlashTirEmission);
		M->SetScalarParameterValue(TEXT("Image"), (float)Img);
		F->bNoFade = true;
		F->FaceDir = Dir.GetSafeNormal();
		F->FaceSize = Taille;
		F->FaceAnchor = 0.f;   // centre de l'étoile sur le point donné (bout du canon)
		F->FaceLift = VersCamera;
		F->FacePlane = F->AddPlane(M, FVector::ZeroVector, 0.f, Taille, Taille);
		F->UpdateFace();
		// Flash d'impact (Vincent, 26/11 : 16 ms, une image) : vu sur l'image où il naît, retiré à la suivante.
		if (bUneImage) { F->UneImage = GFrameCounter; }
		return F;
	};
	AEONFxPiece* Halo = Couche(Col, S * 1.7f, 0.f);
	Couche(FLinearColor::White, S, 1.f);
	if (Halo && LightRadiusPx > 0.f) { Halo->AddLight(Col, 6000.f, EON::PxToCm(LightRadiusPx)); }
}

void EONFx::Giclee(const UObject* Ctx, const FVector& At, const FVector& Dir, float SizeCm, const FLinearColor& Col, float Life, float LiftCm)
{
	// Le point d'impact est à 20 % du bord gauche de l'image : centre décalé de 0,3 côté vers +X.
	PlancheFace(Ctx, TEXT("/Game/EON/VFX/Planches/T_EON_VFX_P2_Giclee.T_EON_VFX_P2_Giclee"), At, Dir, SizeCm, Col, Life, LiftCm, 0.3f);
}

void EONFx::Eclats(const UObject* Ctx, const FVector& At, const FVector& Dir, float SizeCm, const FLinearColor& Col, float Life, float LiftCm)
{
	PlancheFace(Ctx, TEXT("/Game/EON/VFX/Planches/T_EON_VFX_P1_Eclats.T_EON_VFX_P1_Eclats"), At, Dir, SizeCm, Col, Life, LiftCm, 0.f);
}

namespace
{
	double GHorlogeGC = 0.0;
	EONFx::FGrosCoup GGrosCoup;
	struct FOnde
	{
		FVector At = FVector::ZeroVector;
		double T0 = 0.0;
		float Duree = 0.f, Rayon = 0.f, Force = 0.f;
		/** > 0 : rayon final dans le monde (cm) ; sinon Rayon en hauteurs d'écran. */
		float RayonCm = 0.f;
		float Emission = 0.f;
		FLinearColor Teinte = FLinearColor::White;
	};
	TArray<FOnde> GOndes;
}

void EONFx::AvancerHorlogeEcran(float Dt)
{
	GHorlogeGC += FMath::Clamp(Dt, 0.f, 0.1f);
	GOndes.RemoveAll([](const FOnde& O) { return GHorlogeGC - O.T0 >= O.Duree; });
}

void EONFx::OndeChoc(const UObject* Ctx, const FVector& At, float Delai, float Duree, float Rayon, float Force)
{
	if (GOndes.Num() >= OndesChocMax) { GOndes.RemoveAt(0); }
	FOnde& O = GOndes.AddDefaulted_GetRef();
	O.At = At;
	O.T0 = GHorlogeGC + FMath::Max(0.f, Delai);
	O.Duree = FMath::Max(0.05f, Duree);
	O.Rayon = Rayon;
	O.Force = Force;
}

void EONFx::OndeChocMonde(const UObject* Ctx, const FVector& At, float RayonCm, float Delai, float Duree, float Force, float Emission, const FLinearColor& Teinte)
{
	OndeChoc(Ctx, At, Delai, Duree, 0.f, Force);
	GOndes.Last().RayonCm = FMath::Max(1.f, RayonCm);
	GOndes.Last().Emission = Emission;
	GOndes.Last().Teinte = Teinte;
}

void EONFx::OndesChocActives(TArray<FOndeChocVue>& Out)
{
	Out.Reset();
	for (const FOnde& O : GOndes)
	{
		// 12 i/s (Vincent, 26/11) : l'onde grandit par bonds de 1/12 s.
		const float K = (float)(FMath::FloorToDouble((GHorlogeGC - O.T0) * 12.0) / 12.0 / O.Duree);
		if (K < 0.f || K >= 1.f) { continue; }
		const float Ease = 1.f - FMath::Square(1.f - K);
		FOndeChocVue& V = Out.AddDefaulted_GetRef();
		V.At = O.At;
		V.Rayon = 0.04f + (O.Rayon - 0.04f) * Ease;
		V.RayonCm = O.RayonCm > 0.f ? O.RayonCm * (0.08f + 0.92f * Ease) : 0.f;
		V.Force = O.Force * (1.f - K * K);
		V.Emission = O.Emission * (1.f - K * K);
		V.Teinte = O.Teinte;
	}
}

void EONFx::GrosCoup(const UObject* Ctx, const FVector& At, const FLinearColor& Col, int32 Mot)
{
	if (GHorlogeGC - GGrosCoup.T0 < 0.35) { return; }
	GGrosCoup.At = At;
	GGrosCoup.Col = Col;
	GGrosCoup.Mot = FMath::Clamp(Mot, 0, 5);
	GGrosCoup.T0 = GHorlogeGC;
	GGrosCoup.Rot = FMath::FRandRange(-14.f, 14.f);
	GGrosCoup.Graine = FMath::Rand();
	// Onde de choc à la fin de l'écran bichrome (sinon cachée dessous), jusqu'au bout du gros coup.
	OndeChoc(Ctx, At, GrosCoupBichrome, GrosCoupDuree - GrosCoupBichrome);
	UE_LOG(LogTemp, Display, TEXT("EON gros coup %d en %s"), GGrosCoup.Mot, *At.ToCompactString());
}

const EONFx::FGrosCoup* EONFx::GrosCoupActif(float* OutAge)
{
	const float Age = (float)(GHorlogeGC - GGrosCoup.T0);
	if (Age < 0.f || Age >= GrosCoupDuree) { return nullptr; }
	if (OutAge) { *OutAge = FMath::FloorToFloat(Age * 12.f) / 12.f; }   // 12 i/s (Vincent, 26/11)
	return &GGrosCoup;
}

void EONFx::TraceMur(const UObject* Ctx, const FVector& At, const FVector& Normal, float SizePx, float Life)
{
	const FVector N = Normal.GetSafeNormal();
	if (N.IsNearlyZero()) { return; }
	AEONFxPiece* F = SpawnPiece(Ctx, At + N * 0.5f, Life);
	if (!F) { return; }
	UMaterialInstanceDynamic* M = PlancheMid(F, TEXT("/Game/EON/VFX/Planches/T_EON_VFX_P4_Flash.T_EON_VFX_P4_Flash"), 2.f, 2.f, FLinearColor(0.015f, 0.013f, 0.012f));
	if (!M) { F->Destroy(); return; }
	M->SetScalarParameterValue(TEXT("Image"), (float)FMath::RandRange(0, 3));
	F->bNoFade = true;
	const float S = EON::PxToCm(SizePx) * FMath::FRandRange(0.8f, 1.2f);
	UStaticMeshComponent* P = F->AddPlane(M, FVector::ZeroVector, 0.f, S, S);
	// Le plan (normale +Z) collé au mur, tourné au hasard autour de la normale.
	const FVector T = FVector::CrossProduct(N, FMath::Abs(N.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
	P->SetWorldRotation(FRotationMatrix::MakeFromZX(N, T.RotateAngleAxis(FMath::FRandRange(0.f, 360.f), N)).Rotator());
	F->bTacheSol = true;   // érosion de la dernière seconde, comme les taches au sol
	static TArray<TWeakObjectPtr<AEONFxPiece>> Traces;
	Traces.RemoveAll([](const TWeakObjectPtr<AEONFxPiece>& X) { return !X.IsValid(); });
	Traces.Add(F);
	while (Traces.Num() > 100) { if (AEONFxPiece* Old = Traces[0].Get()) { Old->Destroy(); } Traces.RemoveAt(0); }
}

void EONFx::TacheSol(const UObject* Ctx, const FVector& At, float Yaw, float SizeCm, const FLinearColor& Col, float Delay, float Life)
{
	// Traînée de giclée au sol (Vincent, 25/11 : « des giclées qui restent par terre », au moins 1,50 m, dans le sens du
	// tir). Image 2:1 : l'impact est à 6 % du bord gauche, la gerbe file vers +X. At = point d'impact au sol, Yaw = sens
	// du tir, SizeCm = longueur (largeur = la moitié). Pas de sol sous l'impact : rien ; pas de sol au bout (mur, vide) :
	// la traînée est raccourcie de moitié, puis abandonnée.
	UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
	if (!W) { return; }
	FCollisionQueryParams Q(SCENE_QUERY_STAT(EONTache), false);
	auto Sol = [W, &Q](const FVector& P, FHitResult& H)
	{
		return W->LineTraceSingleByChannel(H, P + FVector(0.f, 0.f, 40.f), P - FVector(0.f, 0.f, 60.f), ECC_Visibility, Q) && H.ImpactNormal.Z >= 0.7f;
	};
	FHitResult H, He;
	if (!Sol(At, H)) { return; }
	const FVector Fw = FRotator(0.f, Yaw, 0.f).Vector();
	if (!Sol(H.ImpactPoint + Fw * SizeCm * 0.9f, He) || FMath::Abs(He.ImpactPoint.Z - H.ImpactPoint.Z) > 10.f)
	{
		SizeCm *= 0.5f;
		if (!Sol(H.ImpactPoint + Fw * SizeCm * 0.9f, He) || FMath::Abs(He.ImpactPoint.Z - H.ImpactPoint.Z) > 10.f) { return; }
	}
	AEONFxPiece* F = SpawnPiece(Ctx, H.ImpactPoint + Fw * SizeCm * 0.44f + FVector(0.f, 0.f, 1.5f), Life);
	if (!F) { return; }
	UMaterialInstanceDynamic* M = PlancheMid(F, TEXT("/Game/EON/VFX/Planches/T_EON_VFX_P3_TachesSol.T_EON_VFX_P3_TachesSol"), 2.f, 4.f, Col);
	if (!M) { F->Destroy(); return; }
	M->SetScalarParameterValue(TEXT("Image"), (float)FMath::RandRange(0, 7));
	// Opacité pleine jusqu'à la dernière seconde, puis érosion (masque × opacité < 0,5).
	F->bNoFade = true;
	F->AddPlane(M, FVector::ZeroVector, Yaw, SizeCm, SizeCm * 0.5f);
	F->Delay = Delay;
	if (Delay > 0.f) { F->SetActorHiddenInGame(true); }
	F->bTacheSol = true;
	GTaches.RemoveAll([](const TWeakObjectPtr<AEONFxPiece>& P) { return !P.IsValid(); });
	GTaches.Add(F);
	while (GTaches.Num() > 100) { if (AEONFxPiece* Old = GTaches[0].Get()) { Old->Destroy(); } GTaches.RemoveAt(0); }
}

// eon.fx [anneau|gerbe|dir] : effet de test sur Jira (vérifier le rendu des systèmes Niagara).
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
static FAutoConsoleCommandWithWorldAndArgs GEONCmdFx(TEXT("eon.fx"), TEXT("eon.fx [anneau|gerbe|dir|flash|lumiere|flaque|organique|cercle|cone|ligne] : effet de test sur Jira"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
	{
		const APawn* P = W ? UGameplayStatics::GetPlayerPawn(W, 0) : nullptr;
		if (!P) { return; }
		const FVector Feet = P->GetActorLocation() - FVector(0.f, 0.f, P->GetSimpleCollisionHalfHeight());
		const FString K = Args.Num() ? Args[0] : TEXT("anneau");
		if (K == TEXT("gerbe")) { EONFx::Gerbe2(P, Feet + FVector(0.f, 0.f, 30.f), 34, 3, EONFx::Hex(TEXT("ff9a3d")), FLinearColor::White, 180.f, 0.5f, 1.25f, 5.f); }
		else if (K == TEXT("dir")) { EONFx::GerbeDir2(P, Feet + FVector(0.f, 0.f, 30.f), P->GetActorForwardVector(), 18, 3, EONFx::Hex(TEXT("e0405a")), FLinearColor::White, 170.f, 0.6f, 1.3f, 5.f); }
		else if (K == TEXT("flash")) { EONFx::FlashTir(P, Feet + FVector(0.f, 0.f, 90.f), P->GetActorForwardVector(), EONFx::Hex(TEXT("3ef2e0"))); }
		else if (K == TEXT("explosion")) { EONFx::Explosion(P, Feet + P->GetActorForwardVector() * 200.f, Args.Num() > 1 ? FCString::Atof(*Args[1]) : 160.f, EONFx::Hex(TEXT("ff7a1a")), true, 0.5f); }
		else if (K == TEXT("lumiere")) { EONFx::Lumiere(P, Feet + FVector(0.f, 0.f, 60.f), EONFx::Hex(TEXT("ff7a1a")), 60000.f, 60.f, 1.f); }
		// Gardiens 2 et 3 (GAR-V18, GAR-V19) : effets prêts, branchés quand la Mechakuve et le Chercheur seront portés.
		else if (K == TEXT("flaque")) { EONFx::Sol(P, EONFx::EForme::Flaque, Feet, 0.f, 30.f, 30.f, EONFx::Hex(TEXT("7aff3a")), 4.5f, 0.f, false, 0.6f, FMath::FRand() * 10.f, 1.2f); }
		else if (K == TEXT("organique")) { EONFx::Sol(P, EONFx::EForme::Flaque, Feet, 0.f, 40.f, 40.f, EONFx::Hex(TEXT("c84a7a")), 6.f, 0.f, false, 0.6f, FMath::FRand() * 10.f, 1.2f); }
		else if (K == TEXT("cercle")) { EONFx::Sol(P, EONFx::EForme::Disque, Feet, 0.f, 48.f, 48.f, EONFx::Hex(TEXT("ff3d55")), 0.8f, 8.f, true); }
		else if (K == TEXT("cone")) { EONFx::Sol(P, EONFx::EForme::Cone, Feet, P->GetActorRotation().Yaw, 152.f, 152.f, EONFx::Hex(TEXT("ff3d55")), 0.55f, 0.f, true, 1.15f); }
		else if (K == TEXT("ligne")) { EONFx::Sol(P, EONFx::EForme::Ligne, Feet + P->GetActorForwardVector() * EON::PxToCm(60.f), P->GetActorRotation().Yaw, 120.f, 12.f, EONFx::Hex(TEXT("ff3d55")), 0.55f, 0.f, true); }
		else { EONFx::Anneau(P, Feet, 60.f, 1.5f, EONFx::Hex(TEXT("ff3d8b"))); }
	}));
