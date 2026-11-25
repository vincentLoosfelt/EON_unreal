# EON — lecture seule : hiérarchie de SKEL_Onibi et trajectoire des os des pattes dans A_Onibi_Run (repère composant).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_onibi_os_diag.py
import unreal
L = []
seq = unreal.load_asset("/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run")
sk = unreal.load_asset("/Game/EON/Ennemis/Onibi/SK_Onibi")
opt = unreal.AnimPoseEvaluationOptions()
n = seq.get_play_length()
ref = unreal.AnimPoseExtensions.get_reference_pose(sk.get_editor_property("skeleton"))
names = unreal.AnimPoseExtensions.get_bone_names(ref)
L.append("os %d : %s" % (len(names), ", ".join(str(b) for b in names)))
# Os racines : repère local == repère composant
for b in names:
    lo = unreal.AnimPoseExtensions.get_bone_pose(ref, b, unreal.AnimPoseSpaces.LOCAL).translation
    wo = unreal.AnimPoseExtensions.get_bone_pose(ref, b, unreal.AnimPoseSpaces.WORLD).translation
    if (lo - wo).length() < 0.01: L.append("  racine ? %s" % b)
for k in range(6):
    t = n * k / 6.0
    p = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opt)
    row = []
    for b in ("root", "thight_L", "leg_L", "foot_L", "toe_L", "foot_R", "leg_IK_L"):
        try:
            tr = unreal.AnimPoseExtensions.get_bone_pose(p, b, unreal.AnimPoseSpaces.WORLD)
            v = tr.translation; row.append("%s(%.0f,%.0f,%.0f)" % (b, v.x, v.y, v.z))
        except Exception as e: row.append("%s ERR" % b)
    L.append("t=%.2f %s" % (t, " ".join(row)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_onibi_os_diag.txt", "w", encoding="utf-8").write("\n".join(L))
