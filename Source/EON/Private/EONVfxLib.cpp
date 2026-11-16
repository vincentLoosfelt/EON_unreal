#include "EONVfxLib.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterStore.h"
#include "UObject/UnrealType.h"

namespace
{
	// Résout un chemin « A.B[2].C » : renvoie la propriété finale, l'adresse de sa valeur et l'objet qui la porte.
	bool Resolve(UObject* Root, const FString& Path, FProperty*& OutProp, void*& OutPtr, UObject*& OutOwner)
	{
		TArray<FString> Parts;
		Path.ParseIntoArray(Parts, TEXT("."));
		const UStruct* Struct = Root->GetClass();
		void* Container = Root;
		OutOwner = Root;
		for (int32 i = 0; i < Parts.Num(); ++i)
		{
			FString Name = Parts[i];
			int32 Index = INDEX_NONE;
			int32 Br;
			if (Name.FindChar(TEXT('['), Br))
			{
				Index = FCString::Atoi(*Name.Mid(Br + 1));
				Name = Name.Left(Br);
			}
			FProperty* Prop = FindFProperty<FProperty>(Struct, *Name);
			if (!Prop) { UE_LOG(LogTemp, Error, TEXT("EONVfx : propriété %s introuvable dans %s"), *Name, *Struct->GetName()); return false; }
			void* Ptr = Prop->ContainerPtrToValuePtr<void>(Container);
			if (Index != INDEX_NONE)
			{
				FArrayProperty* Arr = CastField<FArrayProperty>(Prop);
				if (!Arr) { return false; }
				FScriptArrayHelper H(Arr, Ptr);
				if (!H.IsValidIndex(Index)) { UE_LOG(LogTemp, Error, TEXT("EONVfx : index %d hors de %s"), Index, *Name); return false; }
				Ptr = H.GetRawPtr(Index);
				Prop = Arr->Inner;
			}
			if (i == Parts.Num() - 1) { OutProp = Prop; OutPtr = Ptr; return true; }
			if (FStructProperty* SP = CastField<FStructProperty>(Prop)) { Struct = SP->Struct; Container = Ptr; continue; }
			if (FObjectPropertyBase* OP = CastField<FObjectPropertyBase>(Prop))
			{
				UObject* Sub = OP->GetObjectPropertyValue(Ptr);
				if (!Sub) { return false; }
				Struct = Sub->GetClass(); Container = Sub; OutOwner = Sub; continue;
			}
			return false;
		}
		return false;
	}

	void DumpRec(UObject* Obj, const FString& Prefix, FString& Out, int32 Depth)
	{
		if (!Obj || Depth > 4) { return; }
		Out += FString::Printf(TEXT("%s<%s %s>\n"), *Prefix, *Obj->GetClass()->GetName(), *Obj->GetName());
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			FProperty* P = *It;
			const void* V = P->ContainerPtrToValuePtr<void>(Obj);
			FArrayProperty* Arr = CastField<FArrayProperty>(P);
			FObjectPropertyBase* OP = CastField<FObjectPropertyBase>(Arr ? Arr->Inner : P);
			if (OP && P->HasAnyPropertyFlags(CPF_InstancedReference | CPF_ContainsInstancedReference | CPF_PersistentInstance))
			{
				if (Arr)
				{
					FScriptArrayHelper H(Arr, V);
					for (int32 i = 0; i < H.Num(); ++i) { DumpRec(OP->GetObjectPropertyValue(H.GetRawPtr(i)), FString::Printf(TEXT("%s%s[%d] "), *Prefix, *P->GetName(), i), Out, Depth + 1); }
				}
				else { DumpRec(OP->GetObjectPropertyValue(V), Prefix + P->GetName() + TEXT(" "), Out, Depth + 1); }
				continue;
			}
			FString Txt;
			P->ExportTextItem_Direct(Txt, V, nullptr, Obj, PPF_None);
			if (Txt.Len() > 1500) { Txt = Txt.Left(1500) + TEXT("..."); }
			Out += FString::Printf(TEXT("%s  %s = %s\n"), *Prefix, *P->GetName(), *Txt);
		}
	}
}

UObject* UEONVfxLib::GetStatelessEmitter(UNiagaraSystem* System, int32 Index)
{
#if WITH_EDITOR
	if (System && System->GetEmitterHandles().IsValidIndex(Index)) { return reinterpret_cast<UObject*>(System->GetEmitterHandle(Index).GetStatelessEmitter()); }
#endif
	return nullptr;
}

FString UEONVfxLib::DumpObject(UObject* Object)
{
	FString Out;
	DumpRec(Object, TEXT(""), Out, 0);
	return Out;
}

bool UEONVfxLib::SetProp(UObject* Object, const FString& Path, const FString& Value)
{
#if WITH_EDITOR
	FProperty* Prop = nullptr; void* Ptr = nullptr; UObject* Owner = nullptr;
	if (!Object || !Resolve(Object, Path, Prop, Ptr, Owner)) { return false; }
	Owner->Modify();
	const TCHAR* R = Prop->ImportText_Direct(*Value, Ptr, Owner, PPF_None);
	if (!R) { UE_LOG(LogTemp, Error, TEXT("EONVfx : valeur refusée pour %s : %s"), *Path, *Value); return false; }
	return true;
#else
	return false;
#endif
}

bool UEONVfxLib::BindUser(UNiagaraSystem* System, UObject* Object, const FString& Path, const FString& UserName, const FString& Type)
{
#if WITH_EDITOR
	FProperty* Prop = nullptr; void* Ptr = nullptr; UObject* Owner = nullptr;
	if (!System || !Object || !Resolve(Object, Path, Prop, Ptr, Owner)) { return false; }
	FStructProperty* SP = CastField<FStructProperty>(Prop);
	if (!SP) { return false; }
	FNiagaraTypeDefinition Def;
	if (Type == TEXT("float")) { Def = FNiagaraTypeDefinition::GetFloatDef(); }
	else if (Type == TEXT("int")) { Def = FNiagaraTypeDefinition::GetIntDef(); }
	else if (Type == TEXT("color")) { Def = FNiagaraTypeDefinition::GetColorDef(); }
	else if (Type == TEXT("vec2")) { Def = FNiagaraTypeDefinition::GetVec2Def(); }
	else if (Type == TEXT("vec3")) { Def = FNiagaraTypeDefinition::GetVec3Def(); }
	else { return false; }
	FProperty* ModeP = FindFProperty<FProperty>(SP->Struct, TEXT("Mode"));
	FStructProperty* BindP = CastField<FStructProperty>(FindFProperty<FProperty>(SP->Struct, TEXT("ParameterBinding")));
	if (!ModeP || !BindP) { UE_LOG(LogTemp, Error, TEXT("EONVfx : %s n'est pas une distribution"), *Path); return false; }
	Owner->Modify();
	ModeP->ImportText_Direct(TEXT("Binding"), ModeP->ContainerPtrToValuePtr<void>(Ptr), Owner, PPF_None);
	const FName Full(*(TEXT("User.") + UserName));
	*BindP->ContainerPtrToValuePtr<FNiagaraVariableBase>(Ptr) = FNiagaraVariableBase(Def, Full);
	FNiagaraParameterStore& Store = System->GetExposedParameters();
	const FNiagaraVariable Var(Def, Full);
	if (Store.IndexOf(Var) == INDEX_NONE) { System->Modify(); Store.AddParameter(Var, true, true); }
	return true;
#else
	return false;
#endif
}

void UEONVfxLib::FinishSystem(UNiagaraSystem* System, UObject* Emitter)
{
#if WITH_EDITOR
	if (Emitter) { Emitter->PostEditChange(); }
	if (System)
	{
		System->PostEditChange();
		System->RequestCompile(false);
		System->WaitForCompilationComplete();
		System->MarkPackageDirty();
	}
#endif
}
