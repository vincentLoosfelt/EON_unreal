#include "EONStatics.h"
#include "EONTuningSubsystem.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/ToonProfile.h"
#include "Materials/MaterialInstanceConstant.h"
#include "BoneContainer.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "EONCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#if WITH_EDITOR
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "K2Node_VariableGet.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

FEONGameFeelRow UEONStatics::GetGameFeel(const UObject* WorldContext)
{
	if (const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(WorldContext))
	{
		return T->GetGameFeel();
	}
	return FEONGameFeelRow();
}

bool UEONStatics::SetMeshSectionCollision(UStaticMesh* Mesh, int32 LodIndex, int32 SectionIndex, bool bEnable, bool bUseComplexAsSimple)
{
#if WITH_EDITOR
	if (!Mesh) { return false; }
	FMeshSectionInfoMap& Map = Mesh->GetSectionInfoMap();
	if (!Map.IsValidSection(LodIndex, SectionIndex)) { return false; }
	Mesh->Modify();
	FMeshSectionInfo Info = Map.Get(LodIndex, SectionIndex);
	Info.bEnableCollision = bEnable;
	Map.Set(LodIndex, SectionIndex, Info);
	if (UBodySetup* Body = Mesh->GetBodySetup())
	{
		Body->CollisionTraceFlag = bUseComplexAsSimple ? CTF_UseComplexAsSimple : CTF_UseDefault;
	}
	Mesh->Build();
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UEONStatics::ConfigureToonProfile(UToonProfile* Profile, float Threshold, float Smooth, FLinearColor ShadowColor, float DiffuseIndirectScale, bool bRampIncludesShadow)
{
#if WITH_EDITOR
	if (!Profile) { return false; }
	FToonProfileStruct& S = Profile->Settings;
	const float A = FMath::Clamp(Threshold, 0.f, 1.f);
	const float B = FMath::Clamp(Threshold + FMath::Max(Smooth, 0.001f), 0.f, 1.f);
	for (int32 c = 0; c < 4; ++c)
	{
		FRichCurve& Diff = S.DiffuseRamp.ColorCurves[c];
		Diff.Reset();
		const float Lo = c < 3 ? ShadowColor.Component(c) : 1.f;
		Diff.AddKey(0.f, Lo); Diff.AddKey(A, Lo);
		// smoothstep : trois points intermédiaires
		for (float u : {0.25f, 0.5f, 0.75f})
		{
			const float sm = u * u * (3.f - 2.f * u);
			Diff.AddKey(FMath::Lerp(A, B, u), FMath::Lerp(Lo, 1.f, sm));
		}
		Diff.AddKey(B, 1.f); Diff.AddKey(1.f, 1.f);
		for (auto It = Diff.GetKeyHandleIterator(); It; ++It) { Diff.SetKeyInterpMode(*It, RCIM_Linear); }

		FRichCurve& Spec = S.SpecularRamp.ColorCurves[c];
		Spec.Reset(); Spec.AddKey(0.f, c < 3 ? 0.f : 1.f); Spec.AddKey(1.f, c < 3 ? 0.f : 1.f);
	}
	S.DiffuseIndirectScale = DiffuseIndirectScale;
	S.SpecularIndirectScale = 0.f;
	S.bDiffuseRampIncludeShadow = bRampIncludesShadow;
	Profile->PostEditChange();
	Profile->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UEONStatics::SetInstanceToonProfile(UMaterialInstanceConstant* Instance, UToonProfile* Profile)
{
#if WITH_EDITOR
	if (!Instance) { return false; }
	Instance->Modify();
	Instance->bOverrideToonProfile = Profile != nullptr;
	Instance->ToonProfileOverride = Profile;
	Instance->PostEditChange();
	// Le profil surchargé donne à l'instance sa propre permutation : ses usages doivent être recalculés.
	Instance->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
	Instance->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

int32 UEONStatics::SetTranslationRetargeting(USkeleton* Skeleton, const TArray<FName>& AnimationBones, const TArray<FName>& ScaledBones)
{
	if (!Skeleton) return 0;
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	int32 N = 0;
	for (int32 i = 0; i < Ref.GetRawBoneNum(); ++i)   // os virtuels exclus (hors BoneTree)
	{
		const FName Name = Ref.GetBoneName(i);
		const EBoneTranslationRetargetingMode::Type Mode = AnimationBones.Contains(Name) ? EBoneTranslationRetargetingMode::Animation
			: ScaledBones.Contains(Name) ? EBoneTranslationRetargetingMode::AnimationScaled : EBoneTranslationRetargetingMode::Skeleton;
		Skeleton->SetBoneTranslationRetargetingMode(i, Mode, false);
		++N;
	}
	Skeleton->MarkPackageDirty();
	return N;
}

FString UEONStatics::AddUpperBodyLayer(UAnimBlueprint* AnimBlueprint, UAnimSequence* Pose, FName SpineBone, FName AlphaVar)
{
#if WITH_EDITOR
	if (!AnimBlueprint || !Pose) return TEXT("arguments manquants");
	UEdGraph* G = nullptr;
	for (UEdGraph* X : AnimBlueprint->FunctionGraphs)
		if (X && X->GetFName() == UEdGraphSchema_K2::GN_AnimGraph) G = X;
	if (!G) return TEXT("AnimGraph introuvable");
	UAnimGraphNode_Root* Root = nullptr;
	for (UEdGraphNode* N : G->Nodes)
	{
		if (N && N->NodeComment == TEXT("EON_TIR")) return TEXT("déjà en place");
		if (UAnimGraphNode_Root* R = Cast<UAnimGraphNode_Root>(N)) Root = R;
	}
	if (!Root) return TEXT("nœud de sortie introuvable");
	UEdGraphPin* RootIn = nullptr;
	for (UEdGraphPin* P : Root->Pins) if (P->Direction == EGPD_Input) RootIn = P;
	if (!RootIn || RootIn->LinkedTo.Num() != 1) return TEXT("entrée de la sortie non reliée");
	UEdGraphPin* Prev = RootIn->LinkedTo[0];

	if (FBlueprintEditorUtils::FindNewVariableIndex(AnimBlueprint, AlphaVar) == INDEX_NONE)
	{
		FEdGraphPinType T; T.PinCategory = UEdGraphSchema_K2::PC_Real; T.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		FBlueprintEditorUtils::AddMemberVariable(AnimBlueprint, AlphaVar, T, TEXT("0.0"));
	}
	const int32 X0 = Root->NodePosX, Y0 = Root->NodePosY;
	Root->NodePosX += 700;

	FGraphNodeCreator<UAnimGraphNode_SequencePlayer> SC(*G);
	UAnimGraphNode_SequencePlayer* Seq = SC.CreateNode();
	Seq->SetAnimationAsset(Pose);
	Seq->NodePosX = X0; Seq->NodePosY = Y0 + 250;
	SC.Finalize();

	FGraphNodeCreator<UAnimGraphNode_LayeredBoneBlend> LC(*G);
	UAnimGraphNode_LayeredBoneBlend* Lay = LC.CreateNode();
	if (Lay->Node.BlendPoses.Num() == 0) Lay->Node.AddPose();
	Lay->Node.LayerSetup.SetNum(1);
	FBranchFilter F; F.BoneName = SpineBone; F.BlendDepth = 0;
	Lay->Node.LayerSetup[0].BranchFilters = { F };
	Lay->Node.bMeshSpaceRotationBlend = true;
	Lay->NodePosX = X0 + 350; Lay->NodePosY = Y0;
	Lay->NodeComment = TEXT("EON_TIR");
	Lay->bCommentBubbleVisible = true;
	LC.Finalize();

	FGraphNodeCreator<UK2Node_VariableGet> VC(*G);
	UK2Node_VariableGet* V = VC.CreateNode();
	V->VariableReference.SetSelfMember(AlphaVar);
	V->NodePosX = X0; V->NodePosY = Y0 + 450;
	VC.Finalize();

	const UEdGraphSchema* S = G->GetSchema();
	auto Pin = [](UEdGraphNode* N, const TCHAR* Name, EEdGraphPinDirection D) -> UEdGraphPin*
	{
		for (UEdGraphPin* P : N->Pins) if (P->Direction == D && P->PinName == FName(Name)) return P;
		return nullptr;
	};
	auto FirstOut = [](UEdGraphNode* N) -> UEdGraphPin*
	{
		for (UEdGraphPin* P : N->Pins) if (P->Direction == EGPD_Output) return P;
		return nullptr;
	};
	FString R;
	Prev->BreakLinkTo(RootIn);
	R += FString::Printf(TEXT("base:%d "), S->TryCreateConnection(Prev, Pin(Lay, TEXT("BasePose"), EGPD_Input)));
	R += FString::Printf(TEXT("pose:%d "), S->TryCreateConnection(FirstOut(Seq), Pin(Lay, TEXT("BlendPoses_0"), EGPD_Input)));
	R += FString::Printf(TEXT("poids:%d "), S->TryCreateConnection(V->GetValuePin(), Pin(Lay, TEXT("BlendWeights_0"), EGPD_Input)));
	R += FString::Printf(TEXT("sortie:%d "), S->TryCreateConnection(FirstOut(Lay), RootIn));
	FString Pins;
	for (UEdGraphPin* P : Lay->Pins) Pins += P->PinName.ToString() + TEXT(",");
	R += TEXT("| pins couche : ") + Pins;

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	R += FString::Printf(TEXT(" | statut compilation %d"), (int32)AnimBlueprint->Status);
	return R;
#else
	return TEXT("éditeur seulement");
#endif
}

namespace
{
	// Valeur d'énumération par nom affiché (énumérations Blueprint du sample : E_Gait, E_RotationMode...).
	void SetEnumByName(FProperty* P, void* Addr, const TCHAR* Name)
	{
		UEnum* E = nullptr; FNumericProperty* Under = nullptr;
		if (FEnumProperty* EP = CastField<FEnumProperty>(P)) { E = EP->GetEnum(); Under = EP->GetUnderlyingProperty(); }
		else if (FByteProperty* BP = CastField<FByteProperty>(P)) { E = BP->Enum; Under = BP; }
		if (!E || !Under) return;
		for (int32 i = 0; i < E->NumEnums() - 1; ++i)
		{
			if (E->GetDisplayNameTextByIndex(i).ToString() == Name || E->GetNameStringByIndex(i) == Name)
			{
				Under->SetIntPropertyValue(Addr, E->GetValueByIndex(i)); return;
			}
		}
	}
	void SetReal(FProperty* P, void* Addr, double V)
	{
		if (FDoubleProperty* D = CastField<FDoubleProperty>(P)) D->SetPropertyValue(Addr, V);
		else if (FFloatProperty* F = CastField<FFloatProperty>(P)) F->SetPropertyValue(Addr, (float)V);
	}
	void SetBool(FProperty* P, void* Addr, bool V)
	{
		if (FBoolProperty* B = CastField<FBoolProperty>(P)) B->SetPropertyValue(Addr, V);
	}
	template <typename T> void SetStruct(FProperty* P, void* Addr, const T& V)
	{
		FStructProperty* S = CastField<FStructProperty>(P);
		if (S && S->Struct == TBaseStructure<T>::Get()) { *(T*)Addr = V; }
	}
}

void UEONStatics::FillAnimPropsImpl(AActor* Actor, const UScriptStruct* Struct, void* Data)
{
	ACharacter* C = Cast<ACharacter>(Actor);
	if (!C || !Struct || !Data) return;
	AEONCharacter* J = Cast<AEONCharacter>(C);
	UCharacterMovementComponent* M = C->GetCharacterMovement();
	const FVector Vel = C->GetVelocity();
	const float Speed2D = Vel.Size2D();
	const bool bFire = J && J->IsFireHeld();
	const bool bRun = J && J->IsRunning();
	const float Yaw = J ? FMath::RadiansToDegrees(J->GetAimAngle()) : C->GetActorRotation().Yaw;
	const FVector Facing = FRotator(0.f, Yaw, 0.f).Vector();
	FVector GroundLoc = C->GetActorLocation() - FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FVector GroundN = FVector::UpVector;
	if (M && M->CurrentFloor.IsWalkableFloor()) { GroundLoc = M->CurrentFloor.HitResult.ImpactPoint; GroundN = M->CurrentFloor.HitResult.ImpactNormal; }
	const TCHAR* Gait = bRun ? TEXT("Sprint") : (Speed2D > 300.f ? TEXT("Run") : TEXT("Walk"));

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* P = *It;
		void* A = P->ContainerPtrToValuePtr<void>(Data);
		const FString N = P->GetAuthoredName();
		if (N == TEXT("ActorTransform")) SetStruct(P, A, C->GetActorTransform());
		else if (N == TEXT("AimingRotation")) SetStruct(P, A, FRotator(0.f, Yaw, 0.f));
		else if (N == TEXT("BasedMovementDelta") || N == TEXT("LandVelocity")) SetStruct(P, A, FVector::ZeroVector);
		else if (N == TEXT("CurrentMaxAcceleration")) SetReal(P, A, M ? M->GetMaxAcceleration() : 0.0);
		else if (N == TEXT("CurrentMaxDeceleration")) SetReal(P, A, M ? M->GetMaxBrakingDeceleration() : 0.0);
		else if (N == TEXT("Gait")) SetEnumByName(P, A, Gait);
		else if (N == TEXT("GroundLocation")) SetStruct(P, A, GroundLoc);
		else if (N == TEXT("GroundNormal")) SetStruct(P, A, GroundN);
		else if (N == TEXT("InputAcceleration")) SetStruct(P, A, M ? M->GetCurrentAcceleration() : FVector::ZeroVector);
		else if (N == TEXT("JustLanded")) SetBool(P, A, false);
		else if (N == TEXT("MovementMode")) SetEnumByName(P, A, TEXT("OnGround"));   // Jira toujours au sol (Vincent 16/11)
		else if (N == TEXT("OrientationIntent")) SetStruct(P, A, Facing);
		else if (N == TEXT("RotationMode")) SetEnumByName(P, A, bFire ? TEXT("Strafe") : TEXT("OrientToMovement"));
		else if (N == TEXT("Stance")) SetEnumByName(P, A, TEXT("Stand"));
		else if (N == TEXT("Velocity")) SetStruct(P, A, Vel);
		else if (N == TEXT("WantsToStrafe")) SetBool(P, A, bFire);
		else if (N == TEXT("WantsToSprint")) SetBool(P, A, bRun);
		else if (N.StartsWith(TEXT("WantsTo"))) SetBool(P, A, false);
		else if (N == TEXT("InputState"))
		{
			if (FStructProperty* S = CastField<FStructProperty>(P))
			{
				for (TFieldIterator<FProperty> I2(S->Struct); I2; ++I2)
				{
					const FString N2 = I2->GetAuthoredName();
					void* A2 = I2->ContainerPtrToValuePtr<void>(A);
					SetBool(*I2, A2, (N2 == TEXT("WantsToStrafe") && bFire) || (N2 == TEXT("WantsToSprint") && bRun));
				}
			}
		}
	}
}

FString UEONStatics::ImplementAnimPropsInterface(UBlueprint* BP, UClass* Interface, FName FuncName)
{
#if WITH_EDITOR
	if (!BP || !Interface) return TEXT("arguments manquants");
	bool bHas = false;
	for (const FBPInterfaceDescription& D : BP->ImplementedInterfaces) if (D.Interface == Interface) bHas = true;
	if (!bHas && !FBlueprintEditorUtils::ImplementNewInterface(BP, Interface->GetClassPathName())) return TEXT("interface refusée");
	UEdGraph* G = nullptr;
	for (const FBPInterfaceDescription& D : BP->ImplementedInterfaces)
		if (D.Interface == Interface) for (UEdGraph* X : D.Graphs) if (X && X->GetFName() == FuncName) G = X;
	if (!G) return TEXT("graphe de fonction introuvable");
	UK2Node_FunctionEntry* Entry = nullptr; UK2Node_FunctionResult* Result = nullptr;
	TArray<UEdGraphNode*> Old;
	for (UEdGraphNode* N : G->Nodes)
	{
		UK2Node_CallFunction* CF = Cast<UK2Node_CallFunction>(N);
		if (CF && CF->FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UEONStatics, FillAnimProps))
		{
			UEdGraphPin* Then = CF->FindPin(UEdGraphSchema_K2::PN_Then);
			if (Then && Then->LinkedTo.Num() > 0) return TEXT("déjà en place");
			Old.Add(N);   // reste d'un essai incomplet : refait
		}
		if (UK2Node_VariableGet* VG = Cast<UK2Node_VariableGet>(N); VG && VG->VariableReference.GetMemberName() == FName(TEXT("EONProps"))) Old.Add(N);
		if (UK2Node_FunctionEntry* E = Cast<UK2Node_FunctionEntry>(N)) Entry = E;
		if (UK2Node_FunctionResult* R = Cast<UK2Node_FunctionResult>(N)) Result = R;
	}
	for (UEdGraphNode* N : Old) { N->BreakAllNodeLinks(); G->RemoveNode(N); }
	if (!Entry) return TEXT("entrée introuvable");
	if (!Result)
	{
		FGraphNodeCreator<UK2Node_FunctionResult> RC(*G); Result = RC.CreateNode();
		Result->NodePosX = Entry->NodePosX + 900; Result->NodePosY = Entry->NodePosY; RC.Finalize();
	}
	UEdGraphPin* OutPin = nullptr;
	for (UEdGraphPin* P : Result->Pins) if (P->Direction == EGPD_Input && P->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct) OutPin = P;
	if (!OutPin) return TEXT("sortie structure introuvable");
	const FName Local(TEXT("EONProps"));
	if (!FBlueprintEditorUtils::FindLocalVariableGuidByName(BP, G, Local).IsValid())
	{
		FBlueprintEditorUtils::AddLocalVariable(BP, G, Local, OutPin->PinType);
	}
	const FGuid LocalGuid = FBlueprintEditorUtils::FindLocalVariableGuidByName(BP, G, Local);

	FGraphNodeCreator<UK2Node_CallFunction> FC(*G);
	UK2Node_CallFunction* Fill = FC.CreateNode();
	Fill->SetFromFunction(UEONStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UEONStatics, FillAnimProps)));
	Fill->NodePosX = Entry->NodePosX + 350; Fill->NodePosY = Entry->NodePosY;
	FC.Finalize();
	FGraphNodeCreator<UK2Node_VariableGet> VC(*G);
	UK2Node_VariableGet* V = VC.CreateNode();
	V->VariableReference.SetLocalMember(Local, G->GetName(), LocalGuid);
	V->NodePosX = Entry->NodePosX + 100; V->NodePosY = Entry->NodePosY + 250;
	VC.Finalize();

	const UEdGraphSchema* S = G->GetSchema();
	auto Find = [](UEdGraphNode* N, FName Name, EEdGraphPinDirection D) -> UEdGraphPin*
	{
		for (UEdGraphPin* P : N->Pins) { if (P->Direction == D && P->PinName == Name) return P; }
		return nullptr;
	};
	UEdGraphPin* EntryThen = Find(Entry, UEdGraphSchema_K2::PN_Then, EGPD_Output);
	UEdGraphPin* FillExec = Find(Fill, UEdGraphSchema_K2::PN_Execute, EGPD_Input);
	UEdGraphPin* FillThen = Find(Fill, UEdGraphSchema_K2::PN_Then, EGPD_Output);
	UEdGraphPin* ResExec = Find(Result, UEdGraphSchema_K2::PN_Execute, EGPD_Input);
	UEdGraphPin* FillProps = Fill->FindPin(TEXT("Props"));
	FString Missing;
	if (!EntryThen) Missing += TEXT(" entree.then");
	if (!FillExec) Missing += TEXT(" fill.exec");
	if (!FillThen) Missing += TEXT(" fill.then");
	if (!ResExec) Missing += TEXT(" sortie.exec");
	if (!FillProps) Missing += TEXT(" fill.Props");
	if (!Missing.IsEmpty())
	{
		FString All;
		for (UEdGraphNode* N : { (UEdGraphNode*)Entry, (UEdGraphNode*)Fill, (UEdGraphNode*)Result })
			for (UEdGraphPin* P : N->Pins) All += N->GetName() + TEXT(".") + P->PinName.ToString() + TEXT(" ");
		return TEXT("broches manquantes :") + Missing + TEXT(" | ") + All;
	}
	EntryThen->BreakAllPinLinks();
	FString R;
	R += FString::Printf(TEXT("exec1:%d "), S->TryCreateConnection(EntryThen, FillExec));
	R += FString::Printf(TEXT("exec2:%d "), S->TryCreateConnection(FillThen, ResExec));
	R += FString::Printf(TEXT("props:%d "), S->TryCreateConnection(V->GetValuePin(), FillProps));
	R += FString::Printf(TEXT("sortie:%d "), S->TryCreateConnection(V->GetValuePin(), OutPin));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	R += FString::Printf(TEXT("| statut compilation %d"), (int32)BP->Status);
	return R;
#else
	return TEXT("éditeur seulement");
#endif
}
