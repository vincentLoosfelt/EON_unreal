# EON — lecture seule : hauteur du bassin et de la tête de Jira au fil de A_Jira_death (Saved/Logs/eon_jira_mort_diag.txt).
import unreal
seq = unreal.load_asset("/Game/EON/Characters/Jira/Anims/UEFN/A_Jira_death")
opt = unreal.AnimPoseEvaluationOptions()
n = seq.get_play_length(); L = []
for k in range(11):
    t = n * k / 10.0
    p = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opt)
    z = [unreal.AnimPoseExtensions.get_bone_pose(p, b, unreal.AnimPoseSpaces.WORLD).translation.z for b in ("pelvis", "head")]
    L.append("t=%.2f bassin %.0f tête %.0f" % (t, z[0], z[1]))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_jira_mort_diag.txt", "w", encoding="utf-8").write("\n".join(L))
