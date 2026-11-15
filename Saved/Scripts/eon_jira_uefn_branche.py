# EON — Jira (SK_Jira_UEFN, squelette du mannequin UEFN) devient le corps des personnages du Game Animation Sample :
#  - squelette UEFN : translations en mode Skeleton (proportions de chaque maillage), pelvis et os IK en AnimationScaled ;
#  - SandboxCharacter_CMC / _Mover : CharacterMesh = SK_Jira_UEFN, pas de visuel retargeté par-dessus ;
#  - suppression du montage précédent (BP_Jira, entrée RTG_UEFN_Jira d'ABP_GenericRetarget, entrée de GM_Sandbox).
import unreal
EAL = unreal.EditorAssetLibrary
L = []
def log(*a):
    L.append(" ".join(str(x) for x in a))
    open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_jira_uefn_branche.txt", "w", encoding="utf-8").write("\n".join(L))
uefn = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
skel = uefn.get_editor_property("skeleton")
anim = ["root", "attach", "ik_foot_root", "ik_hand_root", "props_root", "prop_01", "poi"]
scaled = ["pelvis", "ik_foot_l", "ik_foot_r", "ik_hand_gun", "ik_hand_l", "ik_hand_r"]
log("os réglés :", unreal.EONStatics.set_translation_retargeting(skel, anim, scaled))
EAL.save_loaded_asset(skel, False)
jira = unreal.load_asset("/Game/EON/Characters/Jira/SK_Jira_UEFN")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for p in ("/Game/Blueprints/SandboxCharacter_CMC", "/Game/Blueprints/SandboxCharacter_Mover"):
    bp = unreal.load_asset(p)
    for h in sub.k2_gather_subobject_data_for_blueprint(bp):
        o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        if isinstance(o, unreal.SkeletalMeshComponent) and o.get_editor_property("skeletal_mesh_asset") == uefn:
            o.set_editor_property("skeletal_mesh_asset", jira)
            log(p, o.get_name(), "->", jira.get_name(), "anim", o.get_editor_property("anim_class").get_name() if o.get_editor_property("anim_class") else None)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp, False)
# --- ménage du montage retargeté
abp = unreal.load_asset("/Game/Blueprints/RetargetedCharacters/ABP_GenericRetarget")
cdo = unreal.get_default_object(abp.generated_class())
m = cdo.get_editor_property("IKRetargeter_Map")
if "RTG_UEFN_Jira" in m:
    m.pop("RTG_UEFN_Jira"); cdo.set_editor_property("IKRetargeter_Map", m); EAL.save_loaded_asset(abp, False)
log("map ABP :", list(cdo.get_editor_property("IKRetargeter_Map").keys()))
gm = unreal.load_asset("/Game/Blueprints/GM_Sandbox")
g = unreal.get_default_object(gm.generated_class())
lst = [x for x in g.get_editor_property("VisualOverrides_Soft") if "BP_Jira" not in str(x)]
g.set_editor_property("VisualOverrides_Soft", lst); EAL.save_loaded_asset(gm, False)
log("liste GM :", len(lst))
if EAL.does_asset_exist("/Game/EON/Characters/Jira/BP_Jira"):
    log("BP_Jira supprimé :", EAL.delete_asset("/Game/EON/Characters/Jira/BP_Jira"))
for t in ("uefn_calib",):
    if EAL.does_asset_exist("/Game/EON/Temp/" + t): EAL.delete_asset("/Game/EON/Temp/" + t)
for x in EAL.list_assets("/Game/EON/Temp", recursive=True): EAL.delete_asset(x)
log("fin")
