# EON — locomotion de Jira adoucie (Vincent, 14/11 : course trop ample, Jira trop rapide).
#   blender -b -P blender_jira_anims.py      (lit Saved/Jira/Jira_rig.blend, écrit Saved/Jira/Anims/*.fbx)
# Les rotations de chaque os sont rapprochées de la pose de repos (= idle, image 1) d'un facteur K,
# les translations aussi ; la cadence est rééchantillonnée pour que le pied d'appui ne glisse pas
# à la vitesse de jeu (vitesse du proto × SpeedMul3D = 0,55).
#   Jira_walk  <- course d'origine, K = 0,55, pour 82 px/s × 0,55 = 2,82 m/s (la « marche » d'origine est une flânerie à 0,5 m/s)
#   Jira_run   <- course d'origine, K = 0,75, pour 82 × 1,7 × 0,55 = 4,79 m/s
# Pied d'appui de la course d'origine : ~6,5 m/s (mesuré). Vitesse ≈ 6,5 × K × cadence.
import bpy, mathutils, os
SRC = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/Jira_rig.blend"
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/Anims/"
os.makedirs(OUT, exist_ok=True)
V0 = 6.5
JOBS = [("Jira_walk", "Jira_run", 0.55, 2.82), ("Jira_run", "Jira_run", 0.75, 4.79)]

bpy.ops.wm.open_mainfile(filepath=SRC)
sc = bpy.context.scene
arm = bpy.data.objects["Armature"]
fps = sc.render.fps / sc.render.fps_base
# La course d'origine est gardée sous Jira_run_origine : le script reste relançable.
src = bpy.data.actions.get("Jira_run_origine")
if src is None:
    src = bpy.data.actions["Jira_run"].copy(); src.name = "Jira_run_origine"


def use(act):
    arm.animation_data.action = act
    arm.animation_data.action_slot = act.slots[0]


def sample(act, t):
    use(act)
    sc.frame_set(int(t), subframe=t - int(t))
    out = {}
    for pb in arm.pose.bones:
        q = pb.rotation_quaternion.copy() if pb.rotation_mode == 'QUATERNION' else pb.rotation_euler.to_quaternion()
        out[pb.name] = (q, pb.location.copy())
    return out


for name, srcname, K, target in JOBS:
    rate = target / (V0 * K)
    f0, f1 = src.frame_range
    period = f1 - f0                       # la dernière image = la première (boucle)
    n = max(8, round(period / rate))
    poses = [sample(src, f0 + period * i / n) for i in range(n + 1)]
    old = bpy.data.actions.get(name)
    if old: bpy.data.actions.remove(old)
    act = bpy.data.actions.new(name); act.use_fake_user = True
    arm.animation_data.action = act
    for i, pose in enumerate(poses):
        for pb in arm.pose.bones:
            q, l = pose[pb.name]
            q = mathutils.Quaternion().slerp(q, K)
            pb.rotation_mode = 'QUATERNION'
            pb.rotation_quaternion = q; pb.location = l * K
            pb.keyframe_insert("rotation_quaternion", frame=1 + i)
            pb.keyframe_insert("location", frame=1 + i)
    print("ANIM %s : K=%.2f cadence ×%.2f, %d images (%.2f s)" % (name, K, rate, n, n / fps))

# Export : une prise par fichier, armature seule
for o in bpy.data.objects:
    o.select_set(o == arm)
bpy.context.view_layer.objects.active = arm
for name, *_ in JOBS:
    act = bpy.data.actions[name]
    for a in list(bpy.data.actions):
        a.use_fake_user = (a == act)
    use(act)
    sc.frame_start, sc.frame_end = int(act.frame_range[0]), int(act.frame_range[1])
    bpy.ops.export_scene.fbx(filepath=OUT + name + ".fbx", use_selection=True, object_types={'ARMATURE'},
                             add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False,
                             bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
    print("ANIM export", name)
for a in bpy.data.actions:
    a.use_fake_user = True
bpy.ops.wm.save_as_mainfile(filepath=SRC)
