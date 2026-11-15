# EON — locomotion de Jira = Game Animation Sample 5.8 d'Epic (Vincent, 15/11 : « importe Locomotion et toutes ses
# animations pour Jira »). Le contenu est copié tel quel depuis le cache Fab (VaultCache/GameAnimationSample_5.8)
# dans /Game/Characters/UEFN_Mannequin (animations sur le squelette UEFN) et /Game/Characters/UE5_Mannequins/Rigs.
# Éditeur fermé :
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_jira_gasp.py
# 1. RTG_UEFN_Jira : copie de RTG_Quinn_Jira, source = IK_UEFN_Mannequin (mêmes noms de chaînes, sauf les pieds :
#    LeftToe/RightToe chez UEFN, LeftFoot/RightFoot chez Jira).
# 2. Retarget du lot de locomotion (idle, marche et course 8 directions, sprint) dans Anims/Locomotion_GASP.
# 3. Échelle des os rendue (SK_Jira vient de Blender, échelles x100 dans la pose de référence), vitesse au sol
#    mesurée sur le root motion puis retirée (même méthode qu'eon_jira_locomotion.py).
# 4. BS_Idle_Walk_Run_Jira refait avec ces animations (les anciennes MF_Unarmed_*_Jira seront retirées une fois validé).
import json, math, unreal
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
ALib = unreal.AnimationLibrary
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DIR = "/Game/EON/Characters/Jira"
SRC = "/Game/Characters/UEFN_Mannequin/Animations"
OUT = DIR + "/Anims/Locomotion_GASP"
RTG = DIR + "/RTG_UEFN_Jira"
SCALE = (29 * 6.25) / 198.0   # échelle de SK_Jira en jeu (181 cm / 198 cm), voir eon_jira_locomotion.py
LOG = []
def log(s):
    LOG.append(str(s)); unreal.log("EON_GASP " + str(s))
    open(PROJ + "Saved/Logs/eon_jira_gasp.txt", "w", encoding="utf-8").write("\n".join(LOG))

# Directions du blendspace (degrés, + = droite) -> suffixe GASP (LL / RR = pas latéraux, pied extérieur devant)
DIRS = {"F": 0, "FR": 45, "RR": 90, "BR": 135, "B": 180, "BL": -135, "LL": -90, "FL": -45}

# --- 1. Retargeter
if not EAL.does_asset_exist(RTG):
    EAL.duplicate_asset(DIR + "/RTG_Quinn_Jira", RTG)
rtg = unreal.load_asset(RTG)
c = unreal.IKRetargeterController.get_controller(rtg)
src_ik = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Rigs/IK_UEFN_Mannequin")
uefn = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
c.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_ik)
c.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, uefn)
c.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
for side in ("Left", "Right"):
    c.set_source_chain(side + "Toe", side + "Foot")
EAL.save_loaded_asset(rtg, False)
log("RTG_UEFN_Jira prêt (source %s)" % c.get_ik_rig(unreal.RetargetSourceOrTarget.SOURCE).get_name())

# --- 2. Retarget du lot
noms = ["Idle/M_Neutral_Stand_Idle_Loop"]
for kind in ("Walk", "Run"):
    for suf in DIRS:
        noms.append("%s/M_Neutral_%s_Loop_%s" % (kind, kind, suf))
noms.append("Sprint/M_Neutral_Sprint_Loop_F")
jira = unreal.load_asset(DIR + "/SK_Jira")
a_faire = [EAL.find_asset_data("%s/%s" % (SRC, n)) for n in noms if not EAL.does_asset_exist("%s/%s_Jira" % (OUT, n.split("/")[-1]))]
if a_faire:
    res = unreal.IKRetargetBatchOperation.duplicate_and_retarget(a_faire, uefn, jira, rtg, "", "", "", "_Jira", OUT, False, True, True)
    log("retargetés : %d" % len(res))
    EAL.save_directory(OUT, False, True)

# --- 3a. Échelle des os (pose de référence de Jira)
REF = APE.get_reference_pose(jira.get_editor_property("skeleton"))
BONES = [str(b) for b in APE.get_bone_names(REF)]
MS = APE.get_bone_pose(REF, "master", unreal.AnimPoseSpaces.LOCAL).scale3d.x
def fix_lengths(seq):
    n = ALib.get_num_frames(seq)
    fixed = 0
    for b in BONES:
        rs = APE.get_bone_pose(REF, b, unreal.AnimPoseSpaces.LOCAL).scale3d
        s0 = ALib.get_bone_pose_for_frame(seq, b, 0, False).scale3d
        if (rs - s0).length() < 0.01 * max(1.0, rs.length()):
            continue
        tr = [ALib.get_bone_pose_for_frame(seq, b, f, False) for f in range(n + 1)]
        seq.controller.set_bone_track_keys(b, [t.translation for t in tr], [t.rotation for t in tr], [rs for _ in tr], False)
        fixed += 1
    for b in BONES:
        if b == "master":
            continue
        rt = APE.get_bone_pose(REF, b, unreal.AnimPoseSpaces.LOCAL).translation.length()
        t0 = ALib.get_bone_pose_for_frame(seq, b, 0, False).translation.length()
        if rt < 0.001 or t0 < rt * MS * 0.3:
            continue
        tr = [ALib.get_bone_pose_for_frame(seq, b, f, False) for f in range(n + 1)]
        seq.controller.set_bone_track_keys(b, [t.translation * (1.0 / MS) for t in tr], [t.rotation for t in tr], [t.scale3d for t in tr], False)
        fixed += 1
    return fixed

# --- 3b. Vitesse au sol (avance de master), puis avance retirée ; mémorisée pour les relances
VJSON = PROJ + "Saved/Scripts/eon_jira_gasp_vitesses.json"
try: MEM = json.load(open(VJSON, encoding="utf-8"))
except Exception: MEM = {}
def ground_speed(seq):
    n = ALib.get_num_frames(seq)
    d = ALib.get_bone_pose_for_frame(seq, "master", n, False).translation - ALib.get_bone_pose_for_frame(seq, "master", 0, False).translation
    v = math.hypot(d.x, d.y) / seq.get_play_length() * SCALE
    name = seq.get_name()
    if v > 1:
        MEM[name] = v
        json.dump(MEM, open(VJSON, "w", encoding="utf-8"), indent=1)
        tr = [ALib.get_bone_pose_for_frame(seq, "master", f, False) for f in range(n + 1)]
        seq.controller.set_bone_track_keys("master", [unreal.Vector(0, 0, 0) for _ in tr], [t.rotation for t in tr], [t.scale3d for t in tr], False)
    seq.set_editor_property("enable_root_motion", False)
    seq.set_editor_property("force_root_lock", False)
    return MEM.get(name, 0.0)

anims = {}
for n in noms:
    seq = unreal.load_asset("%s/%s_Jira" % (OUT, n.split("/")[-1]))
    k = fix_lengths(seq)
    v = ground_speed(seq)
    EAL.save_loaded_asset(seq, False)
    anims[n.split("/")[-1]] = (seq, v)
    log("%s : %d os corrigés, %.0f cm/s au sol, %.2f s" % (seq.get_name(), k, v, seq.get_play_length()))

# --- 4. Blendspace : rangées = vitesses de Jira en jeu (marche, course), sprint au-delà
dt = unreal.load_asset("/Game/EON/Data/DT_GameFeel")
def feel(col):
    names = [str(x) for x in unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)]
    return float(unreal.DataTableFunctionLibrary.get_data_table_column_as_string(dt, col)[names.index("studio")])
walk_px = unreal.get_default_object(unreal.EONCharacter).get_editor_property("walk_speed_px")
row = {"Walk": walk_px * 6.25 * feel("SpeedMul3D")}
row["Run"] = row["Walk"] * feel("RunMul")
log("rangées : marche %.0f cm/s, course %.0f cm/s" % (row["Walk"], row["Run"]))
bs = unreal.load_asset(DIR + "/Anims/BS_Idle_Walk_Run_Jira")
samples = []
def sample(anim, x, y, rate):
    s = unreal.BlendSample()
    s.set_editor_property("animation", anim)
    s.set_editor_property("sample_value", unreal.Vector(x, y, 0))
    s.set_editor_property("rate_scale", rate)
    samples.append(s)
idle = anims["M_Neutral_Stand_Idle_Loop"][0]
for ang in (-180, -135, -90, -45, 0, 45, 90, 135, 180):
    sample(idle, ang, 0.0, 1.0)
for kind in ("Walk", "Run"):
    for suf, ang in DIRS.items():
        a, v = anims["M_Neutral_%s_Loop_%s" % (kind, suf)]
        r = row[kind] / v if v > 1 else 1.0
        sample(a, ang, row[kind], r)
        if ang == 180:
            sample(a, -180, row[kind], r)
params = list(bs.get_editor_property("blend_parameters"))
params[1].set_editor_property("max", round(row["Run"]))
params[1].set_editor_property("grid_num", 2)
bs.set_editor_property("blend_parameters", params)
bs.set_editor_property("sample_data", samples)
bs.set_editor_property("axis_to_scale_animation", unreal.BlendSpaceAxis.BSA_NONE)
EAL.save_loaded_asset(bs, False)
log("BS_Idle_Walk_Run_Jira : %d échantillons (GASP)" % len(samples))

log("fin")
