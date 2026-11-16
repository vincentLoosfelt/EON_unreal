#include "EONTileRoom.h"
#include "EONDoorComponent.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EON.h"
#include "EONTypes.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TS = 16.f;   // px par tuile
	UInstancedStaticMeshComponent* MakeISM(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh, USceneComponent* Parent, bool bCollide)
	{
		UInstancedStaticMeshComponent* C = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetStaticMesh(Mesh);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		C->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		C->SetCanEverAffectNavigation(false);
		return C;
	}
}

AEONTileRoom::AEONTileRoom()
{
	bLiftBox = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/EON/Meshes/SM_Cube_N.SM_Cube_N"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Game/EON/Meshes/SM_Cylinder_N.SM_Cylinder_N"));
	// Sol éclairé (MI_Sol_Eclaire, teinte du sol Unity) pour recevoir les ombres ; décor rouge (pipe), cuves bleues (pipe 1) en M_EON_BG.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MFloor(TEXT("/Game/EON/Materials/MI_Sol_Eclaire.MI_Sol_Eclaire"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MWall(TEXT("/Game/EON/Materials/BG/MI_BG_walll_painted.MI_BG_walll_painted"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MCrate(TEXT("/Game/EON/Materials/BG/MI_BG_Barrier01.MI_BG_Barrier01"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MTank(TEXT("/Game/EON/Materials/BG/MI_BG_pipe_1.MI_BG_pipe_1"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MDoor(TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MGap(TEXT("/Game/EON/Materials/BG/MI_BG_Grille01.MI_BG_Grille01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HubWall(TEXT("/Game/EON/Unity/import/fbx/enviro/wall/WallA320.WallA320"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MHubWall(TEXT("/Game/EON/Materials/BG/MI_BG_walll_painted.MI_BG_walll_painted"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MHubNeon(TEXT("/Game/EON/Materials/BG/MI_BG_Room_light.MI_BG_Room_light"));
	HubWallMesh = HubWall.Object; HubWallMaterial = MHubWall.Object; HubWallLightMaterial = MHubNeon.Object;
	WallHeightCm = 220.f;
	FloorMaterial = MFloor.Object; WallMaterial = MWall.Object; CrateMaterial = MCrate.Object;
	TankMaterial = MTank.Object; DoorMaterial = MDoor.Object; GapMaterial = MGap.Object;

	USceneComponent* Root = RootComponent;
	FloorISM = MakeISM(this, TEXT("Sol"), Cube.Object, Root, true);
	GapISM = MakeISM(this, TEXT("BordsDuVide"), Cube.Object, Root, false);
	PitFloorISM = MakeISM(this, TEXT("SolDuVide"), Cube.Object, Root, true);
	PitFloorISM->SetHiddenInGame(true);
	PitFloorISM->SetVisibility(false);
	PitFloorISM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	PitFloorISM->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	WallISM = MakeISM(this, TEXT("Murs"), Cube.Object, Root, true);
	CrateISM = MakeISM(this, TEXT("Decor"), Cube.Object, Root, true);
	TankISM = MakeISM(this, TEXT("Cuves"), Cyl.Object, Root, true);
	DoorISM = MakeISM(this, TEXT("Portes"), Cube.Object, Root, true);
	RedDoorISM = MakeISM(this, TEXT("PorteRouge"), Cube.Object, Root, true);
	InnerDoorISM = MakeISM(this, TEXT("PortesInterieures"), Cube.Object, Root, true);
	HubWallISM = MakeISM(this, TEXT("MursHUB"), HubWall.Object, Root, false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	InfestISM = MakeISM(this, TEXT("Chair"), Sph.Object, Root, false);
}

void AEONTileRoom::SetInfest(const TArray<int32>& Tiles, float K)
{
	InfestTiles = Tiles;
	InfestK = Tiles.Num() ? K : 0.f;
	if (!InfestMID && DoorMaterial)
	{
		// Chair mutante : rose-violet sombre du proto (#8a2a4a), une lueur faible.
		InfestMID = UMaterialInstanceDynamic::Create(DoorMaterial, this);
		InfestMID->SetVectorParameterValue(TEXT("BC1"), FLinearColor(FColor(138, 42, 74)));
		InfestMID->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.25f, 0.02f, 0.08f));
	}
	if (InfestISM && InfestMID) { InfestISM->SetMaterial(0, InfestMID); }
	RebuildInfest();
}

void AEONTileRoom::SetInfestK(float K)
{
	InfestK = FMath::Max(0.f, K);
	RebuildInfest();
}

void AEONTileRoom::RebuildInfest()
{
	if (!InfestISM) { return; }
	InfestISM->ClearInstances();
	if (InfestK <= 0.f || !bHasGrid) { return; }
	const float T = EON::CmPerTile;
	const FVector Origin(-Grid.W * T * 0.5f, -Grid.H * T * 0.5f, 0.f);
	for (const int32 I : InfestTiles)
	{
		const int32 X = I % Grid.W, Y = I / Grid.W;
		// Une boule de chair par tuile, décalée et de taille variable (comme drawInfest : ox, oy, R0 = 11·k).
		const FVector Off(FMath::Sin(I * 1.7f) * 18.f, FMath::Cos(I * 2.3f) * 18.f, 0.f);
		const float S = 1.35f * InfestK * (0.85f + 0.15f * FMath::Sin(I * 0.9f));
		const FVector C = Origin + FVector((X + 0.5f) * T, (Y + 0.5f) * T, 0.f) + Off;
		InfestISM->AddInstance(FTransform(FRotator(0.f, I * 37.f, 0.f), C + FVector(0, 0, 30.f * S), FVector(S, S, S * 0.75f)));
	}
}

void AEONTileRoom::SetRedDoor(uint8 Door)
{
	RedDoor = Door;
	RedSet.Reset();
	if (!RedDoorISM) { return; }
	RedDoorISM->ClearInstances();
	if (!Door || !bHasGrid) { return; }
	if (!RedDoorMID && DoorMaterial)
	{
		RedDoorMID = UMaterialInstanceDynamic::Create(DoorMaterial, this);
		RedDoorMID->SetVectorParameterValue(TEXT("BC1"), FLinearColor(1.f, 0.24f, 0.33f));
		RedDoorMID->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.9f, 0.05f, 0.12f));
	}
	RedDoorISM->SetMaterial(0, RedDoorMID ? RedDoorMID.Get() : DoorMaterial.Get());
	const float T = EON::CmPerTile;
	const FVector Origin(-Grid.W * T * 0.5f, -Grid.H * T * 0.5f, 0.f);
	FIntPoint P[2];
	Grid.GetDoorTiles(Door, P);
	for (const FIntPoint& Q : P)
	{
		RedSet.Add(Q.Y * Grid.W + Q.X);
		const FVector C = Origin + FVector((Q.X + 0.5f) * T, (Q.Y + 0.5f) * T, 0.f);
		RedDoorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, WallHeightCm * 0.5f), FVector(0.98f, 0.98f, WallHeightCm / 100.f)));
	}
}

void AEONTileRoom::OnConstruction(const FTransform& Transform)
{
	// Plan dessiné : on le relit à chaque modification dans l'éditeur (aperçu immédiat).
	if (!bHasGrid || (HasLayout() && GetWorld() && !GetWorld()->IsGameWorld())) { Generate(); return; }
	SizeTiles = FIntPoint(Grid.W, Grid.H);
	Super::OnConstruction(Transform);
	RebuildVisuals();
}

void AEONTileRoom::BeginPlay()
{
	Super::BeginPlay();
	if (!bHasGrid) { Generate(); }
}

void AEONTileRoom::Generate()
{
	FRandomStream Rng(Seed);
	FEONTileGrid G;
	FString Err;
	TArray<FEONDoorSpec> Specs;
	GetInstanceDoorSpecs(Specs);
	const uint8 Mask = SpecMask(Specs) ? SpecMask(Specs) : (uint8)DoorMask;
	if (HasLayout() && LayoutToGrid(Layout, Mask, G, &Err, &Specs)) { SetGrid(G); return; }
	if (HasLayout()) { UE_LOG(LogEON, Warning, TEXT("%s : plan inutilisable (%s), salle générée à la place"), *GetName(), *Err); }
	if (RoomRole == TEXT("TRAINING")) { G = FEONTileGrid::BuildTraining(); }
	else if (RoomRole == TEXT("COM")) { G = FEONTileGrid::ComTiles((uint8)DoorMask); }
	else { G = FEONTileGrid::GenTiles(RoomRole, Shape, (uint8)DoorMask, Rng, FloorN); }
	SetGrid(G);
}

void AEONTileRoom::SetGrid(const FEONTileGrid& InGrid)
{
	Grid = InGrid;
	bHasGrid = true;
	InnerAll.Reset();
	for (const FEONInnerDoor& D : Grid.Inner) { for (const int32 I : D.Tiles) { InnerAll.Add(I); } }
	InnerClosed = InnerAll;
	Flow.Init(-1, Grid.W * Grid.H);
	FlowTile = -1;
	SizeTiles = FIntPoint(Grid.W, Grid.H);
	AEONRoomBounds::OnConstruction(GetActorTransform());
	RebuildVisuals();
}

void AEONTileRoom::SetLocked(bool bInLocked)
{
	if (bLocked == bInLocked) { return; }
	bLocked = bInLocked;
	UpdateDoorCollision();
}

void AEONTileRoom::UpdateDoorCollision()
{
	// Porte ouverte : la barre est rentrée dans le sol, sans collision. Verrouillée : elle sort et bloque.
	if (!DoorISM) { return; }
	DoorISM->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	DoorISM->SetVisibility(bLocked);
}

void AEONTileRoom::RebuildVisuals()
{
	for (UInstancedStaticMeshComponent* C : { FloorISM.Get(), GapISM.Get(), PitFloorISM.Get(), WallISM.Get(), CrateISM.Get(), TankISM.Get(), DoorISM.Get() })
	{
		if (C) { C->ClearInstances(); }
	}
	if (!bHasGrid) { return; }
	FloorISM->SetMaterial(0, FloorMaterial);
	GapISM->SetMaterial(0, GapMaterial);
	if (bHubWalls && HubWallMesh)
	{
		// Dessus des murs épais : gris très sombre, pour que seules les façades peintes ressortent.
		UMaterialInterface* Top = WallTopMaterial;
		if (!Top)
		{
			if (UMaterialInterface* Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_Flat.M_Flat")))
			{
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Flat, this);
				M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.012f, 0.011f, 0.016f));
				Top = M;
			}
		}
		WallISM->SetMaterial(0, Top ? Top : WallMaterial.Get());
	}
	else { WallISM->SetMaterial(0, WallMaterial); }
	CrateISM->SetMaterial(0, CrateMaterial);
	TankISM->SetMaterial(0, TankMaterial);
	DoorISM->SetMaterial(0, DoorMaterial);

	// Repère local : coin haut-gauche de la salle en (−W/2, −H/2) tuiles. Cube de base = 100 cm, pivot au centre.
	const float T = EON::CmPerTile;
	const FVector Origin(-Grid.W * T * 0.5f, -Grid.H * T * 0.5f, 0.f);
	auto Cell = [&](int32 X, int32 Y) { return Origin + FVector((X + 0.5f) * T, (Y + 0.5f) * T, 0.f); };
	const float Th = FloorThicknessCm;

	for (int32 Y = 0; Y < Grid.H; ++Y)
	{
		for (int32 X = 0; X < Grid.W; ++X)
		{
			const EEONTile V = Grid.Get(X, Y);
			const FVector C = Cell(X, Y);
			switch (V)
			{
			case EEONTile::Pit:
				PitFloorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, -Th * 0.5f), FVector(1.f, 1.f, Th / 100.f)));
				// Fossé (gap) : bord grillagé fin pour qu'on lise le passage à dasher. Le vide lui-même n'a rien.
				if (Grid.Gap.Contains(Y * Grid.W + X))
				{
					GapISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, -Th - 60.f), FVector(0.98f, 0.98f, 0.04f)));
				}
				break;
			case EEONTile::Wall:
				{
					// Avec les façades du HUB, le bloc s'arrête juste sous leur sommet (on voit le haut du mur peint).
					// Bloc sous une façade (sol voisin, même en diagonale) : plus petit et plus bas, caché dans la façade,
					// sinon ses faces tombent pile sur celles du mur du HUB et scintillent.
					bool bCovered = false;
					if (bHubWalls && HubWallMesh)
					{
						for (int32 Dy = -1; Dy <= 1 && !bCovered; ++Dy)
						{
							for (int32 Dx = -1; Dx <= 1; ++Dx)
							{
								const EEONTile N = Grid.Get(X + Dx, Y + Dy);
								if (Grid.IsValidTile(X + Dx, Y + Dy) && N != EEONTile::Wall && N != EEONTile::Door) { bCovered = true; break; }
							}
						}
					}
					const float Hw = !(bHubWalls && HubWallMesh) ? WallHeightCm : bCovered ? WallHeightCm - 12.f : WallHeightCm - 6.f;
					const float Sxy = bCovered ? 0.9f : 1.f;
					WallISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, (Hw - Th) * 0.5f), FVector(Sxy, Sxy, (Hw + Th) / 100.f)));
				}
				break;
			case EEONTile::Crate:
				FloorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, -Th * 0.5f), FVector(1.f, 1.f, Th / 100.f)));
				CrateISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, 40.f), FVector(0.84f, 0.84f, 0.8f)));
				break;
			case EEONTile::Door:
				FloorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, -Th * 0.5f), FVector(1.f, 1.f, Th / 100.f)));
				if (InnerAll.Contains(Y * Grid.W + X)) { break; }
				DoorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, WallHeightCm * 0.5f), FVector(1.f, 1.f, WallHeightCm / 100.f) * FVector(1.f, 1.f, 1.f) * 0.98f));
				break;
			case EEONTile::Tank:
				FloorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, -Th * 0.5f), FVector(1.f, 1.f, Th / 100.f)));
				break;
			default:
				FloorISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, -Th * 0.5f), FVector(1.f, 1.f, Th / 100.f)));
				break;
			}
		}
	}
	// Cuves : un cylindre par bloc de tuiles K (bloc = composante 4-connexe), à l'emprise du bloc.
	// Salle de réveil du Kibo : les cuves sont des AEONWakePod posées par le FloorManager, pas de cylindre.
	TArray<uint8> Seen; Seen.SetNumZeroed(Grid.W * Grid.H);
	for (int32 I = 0; I < Grid.W * Grid.H && !Grid.Feat.Contains(TEXT("training")) && !Grid.Feat.Contains(TEXT("pods")); ++I)
	{
		if (Seen[I] || Grid.T[I] != (uint8)EEONTile::Tank) { continue; }
		int32 X0 = 9999, Y0 = 9999, X1 = -1, Y1 = -1;
		TArray<int32> Q = { I }; Seen[I] = 1;
		while (Q.Num())
		{
			const int32 J = Q.Pop(EAllowShrinking::No);
			const int32 X = J % Grid.W, Y = J / Grid.W;
			X0 = FMath::Min(X0, X); Y0 = FMath::Min(Y0, Y); X1 = FMath::Max(X1, X); Y1 = FMath::Max(Y1, Y);
			for (const FIntPoint D : { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) })
			{
				const int32 Nx = X + D.X, Ny = Y + D.Y;
				if (!Grid.IsValidTile(Nx, Ny)) { continue; }
				const int32 K = Ny * Grid.W + Nx;
				if (!Seen[K] && Grid.T[K] == (uint8)EEONTile::Tank) { Seen[K] = 1; Q.Add(K); }
			}
		}
		const FVector C = Origin + FVector((X0 + X1 + 1) * 0.5f * T, (Y0 + Y1 + 1) * 0.5f * T, 0.f);
		const float Sx = (X1 - X0 + 1) * 0.94f, Sy = (Y1 - Y0 + 1) * 0.94f, Hh = 2.2f;
		TankISM->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(0, 0, Hh * 50.f), FVector(Sx, Sy, Hh)));
	}
	BuildHubWalls(Origin);
	UpdateDoorCollision();
	RebuildInnerDoors();
}

void AEONTileRoom::BuildHubWalls(const FVector& Origin)
{
	HubWallISM->ClearInstances();
	if (!bHubWalls || !HubWallMesh) { return; }
	HubWallISM->SetStaticMesh(HubWallMesh);
	// Par nom d'emplacement (l'ordre d'Unreal n'est pas celui d'Unity) : « light » = fines lignes lumineuses.
	const TArray<FStaticMaterial>& Slots = HubWallMesh->GetStaticMaterials();
	for (int32 I = 0; I < Slots.Num(); ++I)
	{
		UMaterialInterface* M = Slots[I].MaterialSlotName == FName(TEXT("light")) ? HubWallLightMaterial.Get() : HubWallMaterial.Get();
		HubWallISM->SetMaterial(I, M ? M : HubWallMesh->GetMaterial(I));
	}
	// Module du HUB : largeur en X, épaisseur en Y, face travaillée (piliers, panneaux, lignes lumineuses) vers +Y local.
	// Elle doit regarder la salle (avant le 16/11 elle regardait dehors : murs retournés, vu par Vincent).
	const FBox Bx = HubWallMesh->GetBoundingBox();
	const FVector Sz = Bx.GetSize();
	if (Sz.X < 1.f || Sz.Y < 1.f || Sz.Z < 1.f) { return; }
	const float T = EON::CmPerTile;
	auto IsW = [&](int32 X, int32 Y) { return Grid.IsValidTile(X, Y) && Grid.Get(X, Y) == EEONTile::Wall; };
	auto Open = [&](int32 X, int32 Y) { const EEONTile V = Grid.Get(X, Y); return Grid.IsValidTile(X, Y) && V != EEONTile::Wall && V != EEONTile::Door; };
	struct FFace { FIntPoint F; FIntPoint A; float Yaw; };
	// F : vers la salle ; A : sens de la rangée de murs ; Yaw : +Y local tourné vers F.
	const FFace Faces[4] = { { FIntPoint(0, 1), FIntPoint(1, 0), 0.f }, { FIntPoint(0, -1), FIntPoint(1, 0), 180.f }, { FIntPoint(1, 0), FIntPoint(0, 1), -90.f }, { FIntPoint(-1, 0), FIntPoint(0, 1), 90.f } };
	const float Sy = T / Sz.Y;
	for (const FFace& Fc : Faces)
	{
		// Murs nord/sud : au ras du sol de la salle, prolongés dans les coins, pleine hauteur.
		// Murs est/ouest : 2 cm en retrait, 2 cm plus bas, sans prolongement : aucune face ne coïncide avec une autre.
		const bool bNS = Fc.F.Y != 0;
		const float Recess = bNS ? 0.f : 2.f;
		const float Sz2 = (WallHeightCm - (bNS ? 0.f : 2.f)) / Sz.Z;
		auto Facing = [&](int32 X, int32 Y) { return IsW(X, Y) && Open(X + Fc.F.X, Y + Fc.F.Y); };
		for (int32 Y = 0; Y < Grid.H; ++Y)
		{
			for (int32 X = 0; X < Grid.W; ++X)
			{
				if (!Facing(X, Y) || Facing(X - Fc.A.X, Y - Fc.A.Y)) { continue; }
				int32 L = 0;
				while (Facing(X + Fc.A.X * L, Y + Fc.A.Y * L)) { ++L; }
				// Dans un coin, la rangée se prolonge d'une tuile pour rejoindre la rangée perpendiculaire.
				float S0 = 0.f, S1 = (float)L;
				if (bNS && IsW(X - Fc.A.X, Y - Fc.A.Y)) { S0 -= 1.f; }
				if (bNS && IsW(X + Fc.A.X * L, Y + Fc.A.Y * L)) { S1 += 1.f; }
				const float Len = (S1 - S0) * T;
				const int32 N = FMath::Max(1, FMath::RoundToInt(Len / Sz.X));
				const float Wm = Len / N, Sx = Wm / Sz.X;
				// Bord de la tuile côté salle, et début de la rangée.
				const FVector Tile0 = Origin + FVector((X + 0.5f) * T, (Y + 0.5f) * T, 0.f);
				const FVector Fv(Fc.F.X, Fc.F.Y, 0.f), Av(Fc.A.X, Fc.A.Y, 0.f);
				const FVector Edge0 = Tile0 + Fv * (T * 0.5f - Recess) - Av * (T * 0.5f) + Av * (S0 * T);
				const FRotator Rot(0.f, Fc.Yaw, 0.f);
				// Face travaillée (+Y local, Max.Y) au bord de la tuile ; l'épaisseur part dans la tuile de mur.
				const FVector Anchor((Bx.Min.X + Bx.Max.X) * 0.5f * Sx, Bx.Max.Y * Sy, Bx.Min.Z * Sz2);
				for (int32 I = 0; I < N; ++I)
				{
					const FVector Tg = Edge0 + Av * ((I + 0.5f) * Wm);
					const FTransform Xf(Rot, Tg - Rot.RotateVector(Anchor), FVector(Sx, Sy, Sz2));
					HubWallISM->AddInstance(Xf);
				}
			}
		}
	}
}

bool AEONTileRoom::CanServe(FName InRole, FName InShape, uint8 Doors) const
{
	if (!(HasLayout() && Weight > 0.f && (RoomRole == InRole || ExtraRoles.Contains(InRole)) && (Shape.IsNone() || Shape == InShape))) { return false; }
	// Portes posées : il en faut une de chaque côté demandé par la case (les autres resteront murées).
	TArray<FEONDoorSpec> Specs;
	GetClassDoorSpecs(GetClass(), Specs);
	const uint8 M = SpecMask(Specs);
	return !M || (M & Doors) == Doors;
}

void AEONTileRoom::GetClassDoorSpecs(const UClass* Cl, TArray<FEONDoorSpec>& Out)
{
	Out.Reset();
	for (const UClass* C = Cl; C; C = C->GetSuperClass())
	{
		const UBlueprintGeneratedClass* BP = Cast<UBlueprintGeneratedClass>(C);
		if (!BP || !BP->SimpleConstructionScript) { continue; }
		for (const USCS_Node* N : BP->SimpleConstructionScript->GetAllNodes())
		{
			const UEONDoorComponent* D = N ? Cast<UEONDoorComponent>(N->ComponentTemplate) : nullptr;
			if (!D) { continue; }
			FEONDoorSpec S;
			S.Dir = UEONDoorComponent::DirFromYaw(D->GetRelativeRotation().Yaw);
			S.bExt = D->bExterieure;
			S.LocalCm = FVector2D(D->GetRelativeLocation());
			S.Len = D->Largeur;
			Out.Add(S);
		}
	}
}

void AEONTileRoom::GetInstanceDoorSpecs(TArray<FEONDoorSpec>& Out) const
{
	Out.Reset();
	TArray<UEONDoorComponent*> Ds;
	GetComponents<UEONDoorComponent>(Ds);
	const FTransform Xf = GetActorTransform();
	for (const UEONDoorComponent* D : Ds)
	{
		FEONDoorSpec S;
		S.Dir = UEONDoorComponent::DirFromYaw(D->GetComponentRotation().Yaw - GetActorRotation().Yaw);
		S.bExt = D->bExterieure;
		S.LocalCm = FVector2D(Xf.InverseTransformPosition(D->GetComponentLocation()));
		S.Len = D->Largeur;
		Out.Add(S);
	}
}

uint8 AEONTileRoom::SpecMask(const TArray<FEONDoorSpec>& Specs)
{
	uint8 M = 0;
	for (const FEONDoorSpec& S : Specs) { if (S.bExt) { M |= S.Dir; } }
	return M;
}

bool AEONTileRoom::TryOpenInnerDoor(const FVector2D& Px, float RangePx)
{
	for (const FEONInnerDoor& D : Grid.Inner)
	{
		if (!D.Tiles.Num() || !InnerClosed.Contains(D.Tiles[0]) || FVector2D::Distance(Px, D.CenterPx) > RangePx) { continue; }
		for (const int32 I : D.Tiles) { InnerClosed.Remove(I); }
		FlowTile = -1;   // les ennemis passent désormais par là
		RebuildInnerDoors();
		return true;
	}
	return false;
}

bool AEONTileRoom::IsNearClosedInnerDoor(const FVector2D& Px, float RangePx) const
{
	for (const FEONInnerDoor& D : Grid.Inner)
	{
		if (D.Tiles.Num() && InnerClosed.Contains(D.Tiles[0]) && FVector2D::Distance(Px, D.CenterPx) <= RangePx) { return true; }
	}
	return false;
}

void AEONTileRoom::RebuildInnerDoors()
{
	if (!InnerDoorISM) { return; }
	InnerDoorISM->ClearInstances();
	if (!InnerClosed.Num()) { return; }
	if (!InnerDoorMID && DoorMaterial)
	{
		// Porte intérieure : panneau orangé, légèrement lumineux, plus bas que les murs.
		InnerDoorMID = UMaterialInstanceDynamic::Create(DoorMaterial, this);
		InnerDoorMID->SetVectorParameterValue(TEXT("BC1"), FLinearColor(1.f, 0.55f, 0.12f));
		InnerDoorMID->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.6f, 0.25f, 0.02f));
	}
	if (InnerDoorMID) { InnerDoorISM->SetMaterial(0, InnerDoorMID); }
	const float T = EON::CmPerTile, Hh = WallHeightCm * 0.75f;
	const FVector Origin(-Grid.W * T * 0.5f, -Grid.H * T * 0.5f, 0.f);
	for (const FEONInnerDoor& D : Grid.Inner)
	{
		for (const int32 I : D.Tiles)
		{
			if (!InnerClosed.Contains(I)) { continue; }
			const FVector C = Origin + FVector((I % Grid.W + 0.5f) * T, (I / Grid.W + 0.5f) * T, Hh * 0.5f);
			InnerDoorISM->AddInstance(FTransform(FRotator::ZeroRotator, C, D.bAcrossX ? FVector(0.35f, 1.f, Hh / 100.f) : FVector(1.f, 0.35f, Hh / 100.f)));
		}
	}
}

bool AEONTileRoom::LayoutToGrid(const TArray<FString>& Rows, uint8 Doors, FEONTileGrid& Out, FString* Err, const TArray<FEONDoorSpec>* Specs)
{
	const int32 H = Rows.Num();
	int32 W = 0;
	for (const FString& R : Rows) { W = FMath::Max(W, R.Len()); }
	if (W < 8 || H < 6) { if (Err) { *Err = FString::Printf(TEXT("plan de %dx%d, minimum 8x6"), W, H); } return false; }
	Out = FEONTileGrid();
	Out.W = W; Out.H = H; Out.Doors = Doors; Out.Feat = TEXT("plan");
	Out.T.Init((uint8)EEONTile::Wall, W * H);
	int32 NFloor = 0;
	for (int32 Y = 0; Y < H; ++Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			const TCHAR Ch = X < Rows[Y].Len() ? Rows[Y][X] : TEXT('#');
			EEONTile V = EEONTile::Wall;
			switch (Ch)
			{
			case TEXT('.'): V = EEONTile::Floor; ++NFloor; break;
			case TEXT('_'): case TEXT(' '): V = EEONTile::Pit; break;
			case TEXT('='): V = EEONTile::Pit; Out.Gap.Add(Y * W + X); break;
			case TEXT('X'): case TEXT('x'): V = EEONTile::Crate; break;
			case TEXT('K'): case TEXT('k'): V = EEONTile::Tank; break;
			default: break;
			}
			Out.Set(X, Y, V);
		}
	}
	if (NFloor < 12) { if (Err) { *Err = TEXT("moins de 12 tuiles de sol"); } return false; }
	// Portes posées (composants Porte EON) : la flèche donne le côté, la position le long du bord est libre.
	uint8 Placed = 0;
	const float T = EON::CmPerTile;
	auto ToTile = [&](const FVector2D& Cm) { return FVector2D((Cm.X + W * T * 0.5f) / T, (Cm.Y + H * T * 0.5f) / T); };
	if (Specs)
	{
		for (const FEONDoorSpec& S : *Specs)
		{
			const FVector2D Tt = ToTile(S.LocalCm);
			if (S.bExt)
			{
				if (Placed & S.Dir) { UE_LOG(LogEON, Warning, TEXT("Plan : deux portes extérieures %s, la seconde est ignorée"), EONDoor::Name(S.Dir)); continue; }
				Placed |= S.Dir;
				if (Out.DoorAt.Num() != 4) { Out.DoorAt.Init(FIntPoint(-1, -1), 4); }
				const int32 Ax = FMath::Clamp(FMath::RoundToInt(Tt.X) - 1, 1, W - 3), Ay = FMath::Clamp(FMath::RoundToInt(Tt.Y) - 1, 1, H - 3);
				Out.DoorAt[FEONTileGrid::DoorIndex(S.Dir)] = S.Dir == EONDoor::N ? FIntPoint(Ax, 0) : S.Dir == EONDoor::S ? FIntPoint(Ax, H - 1)
					: S.Dir == EONDoor::W ? FIntPoint(0, Ay) : FIntPoint(W - 1, Ay);
			}
			else
			{
				// Porte intérieure : en travers du passage indiqué par la flèche, Len tuiles.
				FEONInnerDoor In;
				In.bAcrossX = S.Dir == EONDoor::E || S.Dir == EONDoor::W;
				const int32 L = FMath::Clamp(S.Len, 1, 8);
				const int32 X0 = In.bAcrossX ? FMath::Clamp(FMath::FloorToInt(Tt.X), 1, W - 2) : FMath::Clamp(FMath::RoundToInt(Tt.X - L * 0.5f), 1, W - 1 - L);
				const int32 Y0 = In.bAcrossX ? FMath::Clamp(FMath::RoundToInt(Tt.Y - L * 0.5f), 1, H - 1 - L) : FMath::Clamp(FMath::FloorToInt(Tt.Y), 1, H - 2);
				for (int32 K = 0; K < L; ++K)
				{
					const int32 X = In.bAcrossX ? X0 : X0 + K, Y = In.bAcrossX ? Y0 + K : Y0;
					Out.Set(X, Y, EEONTile::Door);
					In.Tiles.Add(Y * W + X);
				}
				In.CenterPx = In.bAcrossX ? FVector2D((X0 + 0.5f) * TS, (Y0 + L * 0.5f) * TS) : FVector2D((X0 + L * 0.5f) * TS, (Y0 + 0.5f) * TS);
				Out.Inner.Add(In);
			}
		}
	}
	// Portes de la case : posées, sinon au milieu de chaque côté ; la tuile devant est dégagée si le plan y met un mur.
	// Une porte posée sans voisin dans l'étage reste un mur.
	for (const uint8 D : { EONDoor::N, EONDoor::S, EONDoor::E, EONDoor::W })
	{
		if (!(Doors & D))
		{
			if (Placed & D) { FIntPoint Q[2]; Out.GetDoorTiles(D, Q); for (const FIntPoint& P : Q) { Out.Set(P.X, P.Y, EEONTile::Wall); } }
			continue;
		}
		FIntPoint Q[2]; Out.GetDoorTiles(D, Q);
		const FIntPoint In = FIntPoint::ZeroValue - EONDoor::Dir(D);
		for (const FIntPoint& P : Q)
		{
			Out.Set(P.X, P.Y, EEONTile::Door);
			if (Out.Get(P.X + In.X, P.Y + In.Y) == EEONTile::Wall) { Out.Set(P.X + In.X, P.Y + In.Y, EEONTile::Floor); }
		}
	}
	return true;
}

void AEONTileRoom::GridToLayout(const FEONTileGrid& G, TArray<FString>& Rows)
{
	Rows.Reset();
	for (int32 Y = 0; Y < G.H; ++Y)
	{
		FString S;
		for (int32 X = 0; X < G.W; ++X)
		{
			switch (G.Get(X, Y))
			{
			case EEONTile::Floor: S.AppendChar(TEXT('.')); break;
			case EEONTile::Pit: S.AppendChar(G.Gap.Contains(Y * G.W + X) ? TEXT('=') : TEXT('_')); break;
			case EEONTile::Door: S.AppendChar(TEXT('D')); break;
			case EEONTile::Crate: S.AppendChar(TEXT('X')); break;
			case EEONTile::Tank: S.AppendChar(TEXT('K')); break;
			default: S.AppendChar(TEXT('#')); break;
			}
		}
		Rows.Add(S);
	}
}

void AEONTileRoom::PlanDepuisGenerateur()
{
	FRandomStream Rng(Seed);
	const FEONTileGrid G = RoomRole == TEXT("COM") ? FEONTileGrid::ComTiles(15) : FEONTileGrid::GenTiles(RoomRole, Shape.IsNone() ? FName(TEXT("std")) : Shape, 15, Rng, FloorN);
	Modify();
	GridToLayout(G, Layout);
	Generate();
}

FVector2D AEONTileRoom::WorldToRoomPx(const FVector& World) const
{
	const FVector L = World - GetActorLocation();
	return FVector2D(EON::CmToPx(L.X + Grid.W * EON::CmPerTile * 0.5f), EON::CmToPx(L.Y + Grid.H * EON::CmPerTile * 0.5f));
}

FVector AEONTileRoom::RoomPxToWorld(const FVector2D& Px, float Z) const
{
	return GetActorLocation() + FVector(EON::PxToCm(Px.X) - Grid.W * EON::CmPerTile * 0.5f, EON::PxToCm(Px.Y) - Grid.H * EON::CmPerTile * 0.5f, Z);
}

EEONTile AEONTileRoom::TileAtPx(const FVector2D& P) const
{
	return Grid.Get(FMath::FloorToInt(P.X / TS), FMath::FloorToInt(P.Y / TS));
}

bool AEONTileRoom::IsTileSolid(int32 I, bool bPits) const
{
	if (!Grid.T.IsValidIndex(I)) { return true; }
	const EEONTile V = (EEONTile)Grid.T[I];
	if (InnerClosed.Contains(I)) { return true; }
	return V == EEONTile::Wall || V == EEONTile::Crate || V == EEONTile::Tank || (InfestK > 0.35f && InfestTiles.Contains(I)) || (V == EEONTile::Door && !InnerAll.Contains(I) && (bLocked || RedSet.Contains(I))) || (bPits && V == EEONTile::Pit);
}

bool AEONTileRoom::SolidAtPx(const FVector2D& P) const
{
	const int32 X = FMath::FloorToInt(P.X / TS), Y = FMath::FloorToInt(P.Y / TS);
	if (!Grid.IsValidTile(X, Y)) { return true; }
	return IsTileSolid(Y * Grid.W + X, false);
}

bool AEONTileRoom::CollideCircle(FVector2D& O, float R, bool bPits) const
{
	bool bHit = false;
	for (int32 It = 0; It < 2; ++It)
	{
		const int32 X0 = FMath::FloorToInt((O.X - R) / TS), X1 = FMath::FloorToInt((O.X + R) / TS);
		const int32 Y0 = FMath::FloorToInt((O.Y - R) / TS), Y1 = FMath::FloorToInt((O.Y + R) / TS);
		for (int32 Ty = Y0; Ty <= Y1; ++Ty)
		{
			for (int32 Tx = X0; Tx <= X1; ++Tx)
			{
				const bool bOut = !Grid.IsValidTile(Tx, Ty);
				if (!bOut && !IsTileSolid(Ty * Grid.W + Tx, bPits)) { continue; }
				const float Sx = Tx * TS, Sy = Ty * TS;
				const float Cx = FMath::Clamp(O.X, Sx, Sx + TS), Cy = FMath::Clamp(O.Y, Sy, Sy + TS);
				const float Dx = O.X - Cx, Dy = O.Y - Cy, D2 = Dx * Dx + Dy * Dy;
				if (D2 < R * R)
				{
					bHit = true;
					if (D2 == 0.f)
					{
						const float L = O.X - Sx, Rr = Sx + TS - O.X, Tt = O.Y - Sy, Bb = Sy + TS - O.Y, M = FMath::Min(FMath::Min(L, Rr), FMath::Min(Tt, Bb));
						if (M == L) { O.X = Sx - R; } else if (M == Rr) { O.X = Sx + TS + R; } else if (M == Tt) { O.Y = Sy - R; } else { O.Y = Sy + TS + R; }
					}
					else
					{
						const float D = FMath::Sqrt(D2);
						O.X += Dx / D * (R - D); O.Y += Dy / D * (R - D);
					}
				}
			}
		}
	}
	return bHit;
}

void AEONTileRoom::ComputeFlow(const FVector2D& TargetPx)
{
	const int32 W = Grid.W, H = Grid.H;
	const int32 St = FMath::FloorToInt(TargetPx.Y / TS) * W + FMath::FloorToInt(TargetPx.X / TS);
	if (St == FlowTile) { return; }
	FlowTile = St;
	Flow.Init(-1, W * H);
	if (St < 0 || St >= W * H) { return; }
	Flow[St] = 0;
	TArray<int32> Q; Q.Reserve(W * H); Q.Add(St);
	for (int32 Qi = 0; Qi < Q.Num(); ++Qi)
	{
		const int32 I = Q[Qi], X = I % W, Y = I / W;
		for (const FIntPoint D : { FIntPoint(0, -1), FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(-1, 0) })
		{
			const int32 Nx = X + D.X, Ny = Y + D.Y;
			if (Nx < 0 || Ny < 0 || Nx >= W || Ny >= H) { continue; }
			const int32 J = Ny * W + Nx;
			if (Flow[J] >= 0) { continue; }
			const EEONTile V = (EEONTile)Grid.T[J];
			if ((V == EEONTile::Floor || V == EEONTile::Door) && !InnerClosed.Contains(J)) { Flow[J] = Flow[I] + 1; Q.Add(J); }
		}
	}
}

void AEONTileRoom::GetWalkableTiles(TArray<FIntPoint>& Out) const
{
	Out.Reset();
	for (int32 Y = 1; Y < Grid.H - 1; ++Y)
	{
		for (int32 X = 1; X < Grid.W - 1; ++X)
		{
			if (Grid.Get(X, Y) != EEONTile::Floor) { continue; }
			if (Grid.Get(X + 1, Y) == EEONTile::Pit || Grid.Get(X - 1, Y) == EEONTile::Pit || Grid.Get(X, Y + 1) == EEONTile::Pit || Grid.Get(X, Y - 1) == EEONTile::Pit) { continue; }
			Out.Add(FIntPoint(X, Y));
		}
	}
}

FVector2D AEONTileRoom::DoorCenterPx(uint8 D) const
{
	FIntPoint P[2];
	Grid.GetDoorTiles(D, P);
	const float Cx = (P[0].X + P[1].X + 1) * 0.5f * TS, Cy = (P[0].Y + P[1].Y + 1) * 0.5f * TS, RW = Grid.W * TS, RH = Grid.H * TS;
	return D == EONDoor::N ? FVector2D(Cx, 28.f) : D == EONDoor::S ? FVector2D(Cx, RH - 24.f) : D == EONDoor::W ? FVector2D(28.f, Cy + 2.f) : FVector2D(RW - 28.f, Cy + 2.f);
}

AEONTileRoom* AEONTileRoom::FindTileRoomAt(const UObject* WorldContext, FVector Location)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) { return nullptr; }
	const FVector2D P(Location.X, Location.Y);
	for (TActorIterator<AEONTileRoom> It(World); It; ++It)
	{
		if (It->GetRect2D().IsInside(P)) { return *It; }
	}
	return nullptr;
}
