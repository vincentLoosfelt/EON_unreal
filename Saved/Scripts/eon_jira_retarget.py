# EON — Jira (captain_v0.2.fbx d'Unity, squelette Auto-Rig Pro) branché sur la locomotion du mannequin :
# IK Rig auto (Quinn + Jira) -> retargeter Quinn->Jira (chaînes auto, os alignés) -> ABP_Unarmed et ses anims dupliqués pour Jira.
import unreal
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Characters/Jira"
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_JIRA " + str(m))


quinn = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
jira = unreal.load_asset("/Game/EON/Unity/import/fbx/chara/captain_v0_2")
for p in ("IK_Quinn", "IK_Jira", "RTG_Quinn_Jira"):
    if EAL.does_asset_exist(DIR + "/" + p):
        EAL.delete_asset(DIR + "/" + p)


def make_rig(name, mesh):
    rig = AT.create_asset(name, DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    log("%s auto retarget : %s" % (name, c.apply_auto_generated_retarget_definition()))
    log("%s auto FBIK : %s" % (name, c.apply_auto_fbik()))
    log("%s racine=%s chaînes=%s" % (name, c.get_retarget_root(), [str(x.get_editor_property("chain_name")) for x in c.get_retarget_chains()]))
    EAL.save_loaded_asset(rig)
    return rig


rq = make_rig("IK_Quinn", quinn)
rj = make_rig("IK_Jira", jira)
# Squelette Auto-Rig Pro : le gabarit automatique ne reconnaît que bras/jambes. Chaînes posées à la main,
# mêmes noms que celles du mannequin pour que le mapping soit direct.
cj = unreal.IKRigController.get_controller(rj)
for ch in list(cj.get_retarget_chains()):
    cj.remove_retarget_chain(ch.get_editor_property("chain_name"))
cj.set_retarget_root("root_x")   # à l'import, les « . » des os Auto-Rig Pro deviennent « _ »
CH = [("Spine", "spine_01_x", "spine_02_x"), ("Neck", "neck_x", "neck_x"), ("Head", "head_x", "head_x")]
for sd, S_ in (("l", "Left"), ("r", "Right")):
    CH += [(S_ + "Clavicle", "shoulder_" + sd, "shoulder_" + sd), (S_ + "Arm", "arm_stretch_" + sd, "hand_" + sd),
           (S_ + "Leg", "thigh_stretch_" + sd, "foot_" + sd), (S_ + "Foot", "toes_01_" + sd, "toes_01_" + sd)]
    for f, q in (("Thumb", "thumb"), ("Index", "index"), ("Middle", "middle"), ("Ring", "ring"), ("Pinky", "pinky")):
        CH.append((S_ + f, "c_%s1_%s" % (q, sd), "c_%s3_%s" % (q, sd)))
for name, a, b in CH:
    r = cj.add_retarget_chain(name, a, b, "")
    if not r:
        log("chaîne refusée : %s %s->%s" % (name, a, b))
log("Jira : racine=%s, %d chaînes" % (cj.get_retarget_root(), len(cj.get_retarget_chains())))
EAL.save_loaded_asset(rj)
cq = unreal.IKRigController.get_controller(rq)
for ch in cq.get_retarget_chains():
    n = ch.get_editor_property("chain_name")
    if str(n) in ("LeftLeg", "LeftFoot", "LeftArm", "Spine"):
        log("Quinn %s : %s -> %s" % (n, cq.get_retarget_chain_start_bone(n), cq.get_retarget_chain_end_bone(n)))
rtg = AT.create_asset("RTG_Quinn_Jira", DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET
rc.set_ik_rig(S, rq); rc.set_ik_rig(T, rj)
rc.set_preview_mesh(S, quinn); rc.set_preview_mesh(T, jira)
try:
    rc.add_default_ops(); rc.assign_ik_rig_to_all_ops(S, rq); rc.assign_ik_rig_to_all_ops(T, rj)
except Exception as e:
    log("ops : %s" % e)
try:
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
except Exception as e:
    log("auto_map : %s" % e)
for side in (T,):
    try:
        rc.auto_align_all_bones(side)
        log("alignement auto des os : ok")
    except Exception as e:
        log("auto_align : %s" % e)
EAL.save_loaded_asset(rtg)

abp = unreal.EditorAssetLibrary.find_asset_data("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed")
res = unreal.IKRetargetBatchOperation.duplicate_and_retarget([abp], quinn, jira, rtg, "", "", "", "_Jira", DIR + "/Anims", False, True, True)
log("retargetés : %d" % len(res))
EAL.save_directory(DIR, False, True)
for ad in res:
    log("  %s" % ad.package_name)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_jira_retarget.txt", "w", encoding="utf-8").write("\n".join(LOG))
