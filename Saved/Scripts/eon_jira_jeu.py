# EON — Jira (SK_Jira_UEFN) dans le jeu avec la locomotion du Game Animation Sample :
#  - SandboxCharacter_CMC_ABP : couche haut du corps (depuis spine_01) jouant A_Jira_shoot_front, poids EONTirAlpha ;
#  - BP_Jira (pion du jeu) : implémente BPI_SandboxCharacter_Pawn, Get_PropertiesForAnimation = UEONStatics::FillAnimProps.
import unreal
EAL = unreal.EditorAssetLibrary
L = []
def log(*a):
    L.append(" ".join(str(x) for x in a))
    open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_jira_jeu.txt", "w", encoding="utf-8").write("\n".join(L))
abp = unreal.load_asset("/Game/Blueprints/SandboxCharacter_CMC_ABP")
pose = unreal.load_asset("/Game/EON/Characters/Jira/Anims/UEFN/A_Jira_shoot_front")
log("couche tir :", unreal.EONStatics.add_upper_body_layer(abp, pose, "spine_01", "EONTirAlpha"))
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
log("statut ABP après recompilation :", abp.get_editor_property("status") if hasattr(abp, "status") else "?")
log("sauvé ABP :", EAL.save_loaded_asset(abp, False))
bp = unreal.load_asset("/Game/EON/Blueprints/BP_Jira")
bpi = unreal.load_asset("/Game/Blueprints/BPI_SandboxCharacter_Pawn")
log("BP_Jira parent :", unreal.get_default_object(bp.generated_class()).get_class().get_name())
log("interface :", unreal.EONStatics.implement_anim_props_interface(bp, bpi.generated_class(), "Get_PropertiesForAnimation"))
log("sauvé BP_Jira :", EAL.save_loaded_asset(bp, False))
cdo = unreal.get_default_object(bp.generated_class())
m = cdo.get_editor_property("mesh")
log("BP_Jira maillage :", m.get_editor_property("skeletal_mesh_asset"), "anim :", m.get_editor_property("anim_class"))
gm = unreal.load_asset("/Game/EON/Blueprints/BP_EONGameMode")
if gm:
    log("pion par défaut :", unreal.get_default_object(gm.generated_class()).get_editor_property("default_pawn_class"))
