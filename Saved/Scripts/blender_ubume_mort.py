# EON — mort de l'Ubume clée à la main (Vincent, 25/11 : « mort clée dans Blender »). Part d'Invocateur_Iddle_MASTER.blend
# (rig du studio, 64 os FK), pose de départ = image 1 de l'attente. Sursaut en arrière, le corps plie, s'effondre vers
# l'avant, les tentacules retombent à plat derrière, petit rebond puis immobile. 30 i/s.
# Les poses sont des rotations en axes monde autour de la tête de chaque os (indépendant des axes locaux du rig).
#   blender -b <Invocateur_Iddle_MASTER.blend> --python blender_ubume_mort.py -- <sortie.blend> [dossier_planche]
import bpy, sys, math, mathutils
V = mathutils.Vector; M = mathutils.Matrix
argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]; PL = argv[1] if len(argv) > 1 else None
sc = bpy.context.scene; sc.render.fps = 30
arm = bpy.data.objects["Armature"]
idle = arm.animation_data.action
sc.frame_set(1)
bpy.context.view_layer.update()
# Les tentacules ont des Damped Track vers leur enfant (cycle de dépendance) : on fige leur effet de l'image 1 dans la
# pose de base, puis on les retire pour clée les tentacules directement.
VIS = {pb.name: pb.matrix.copy() for pb in arm.pose.bones}
for pb in arm.pose.bones:
    for c in list(pb.constraints): pb.constraints.remove(c)
bpy.context.view_layer.update()
for n in [b.name for b in sorted(arm.data.bones, key=lambda b: len(b.parent_recursive))]:
    arm.pose.bones[n].matrix = VIS[n]; bpy.context.view_layer.update()
BASE = {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}
order = []
def walk(b):
    order.append(b.name)
    for c in b.children: walk(c)
for b in arm.data.bones:
    if b.parent is None: walk(b)
X, Y, Z = V((1, 0, 0)), V((0, 1, 0)), V((0, 0, 1))
# angle > 0 autour de +X : le haut de l'os part vers -Y (l'avant du modèle). Autour de +Y : bras G (x>0) levé si < 0.
def tent(a1, a2, a3, a4, a5, side=0.0):
    return {"Tentacles_1": [(X, a1), (Y, side)], "Tentacles_2": [(X, a2)], "Tentacles_3": [(X, a3)], "Tentacles_4": [(X, a4)], "Tentacles_5": [(X, a5)]}
def pose(pel=(0, 0, 0), rot=None, **k):
    d = {"Pelvis": {"t": V(pel), "r": rot or []}}
    for n, r in k.items(): d[n] = {"r": r}
    return d
P = {}
P[1] = {}
P[6] = dict(pose((0, 0.03, 0.04), [(X, -8)]), Spine_02={"r": [(X, -16)]}, Neck={"r": [(X, -12)]}, Head={"r": [(X, -18)]},
            Upper_arm_L={"r": [(Y, -45), (X, -10)]}, Upper_arm_R={"r": [(Y, 45), (X, -10)]},
            Lower_arm_L={"r": [(Y, -20)]}, Lower_arm_R={"r": [(Y, 20)]}, Mutation={"r": [(X, -25)]},
            **{n: {"r": r} for n, r in tent(-6, -8, -10, -10, -8).items()})
P[10] = dict(pose((0, 0.04, 0.02), [(X, -12)]), Spine_02={"r": [(X, -22)]}, Neck={"r": [(X, -15)]}, Head={"r": [(X, -25)]},
             Upper_arm_L={"r": [(Y, -60), (X, -15)]}, Upper_arm_R={"r": [(Y, 58), (X, -12)]},
             Lower_arm_L={"r": [(Y, -30)]}, Lower_arm_R={"r": [(Y, 25)]}, Mutation={"r": [(X, -35)]},
             **{n: {"r": r} for n, r in tent(-8, -12, -14, -14, -10).items()})
P[15] = dict(pose((0, -0.01, -0.14), [(X, 10), (Z, 3)]), Spine_02={"r": [(X, 14)]}, Neck={"r": [(X, 6)]}, Head={"r": [(X, 12), (Y, 5)]},
             Upper_arm_L={"r": [(Y, -25), (X, 5)]}, Upper_arm_R={"r": [(Y, 22), (X, 8)]},
             Lower_arm_L={"r": [(X, 10)]}, Lower_arm_R={"r": [(X, 12)]}, Mutation={"r": [(X, 10)]},
             **{n: {"r": r} for n, r in tent(-14, 4, 8, 6, 4, 2).items()})   # pas de redressement : il plie déjà vers l'avant
P[22] = dict(pose((0, -0.05, -0.42), [(X, 22), (Z, 6)]), Spine_02={"r": [(X, 30)]}, Neck={"r": [(X, 15)]}, Head={"r": [(X, 35), (Y, 10)]},
             Upper_arm_L={"r": [(Y, 10), (X, 20)]}, Upper_arm_R={"r": [(Y, -8), (X, 25)]},
             Lower_arm_L={"r": [(X, 25)]}, Lower_arm_R={"r": [(X, 30)]}, Mutation={"r": [(X, 30)]},
             **{n: {"r": r} for n, r in tent(-30, 20, 25, 20, 15, 4).items()})
P[34] = dict(pose((0, -0.35, -1.12), [(X, 82), (Z, 12)]), Spine_02={"r": [(X, 8)]}, Neck={"r": [(X, 5)]}, Head={"r": [(X, 20), (Y, 25)]},
             Upper_arm_L={"r": [(Y, -55), (X, -20)]}, Upper_arm_R={"r": [(Y, 50), (X, -25)]},
             Lower_arm_L={"r": [(X, -15)]}, Lower_arm_R={"r": [(X, -10)]}, Mutation={"r": [(X, -15)]},
             **{n: {"r": r} for n, r in tent(-8, -4, 6, 8, 6, 6).items()})
P[40] = dict(pose((0, -0.37, -1.05), [(X, 78), (Z, 12)]), Spine_02={"r": [(X, 6)]}, Neck={"r": [(X, 2)]}, Head={"r": [(X, 12), (Y, 28)]},
             Upper_arm_L={"r": [(Y, -58), (X, -22)]}, Upper_arm_R={"r": [(Y, 53), (X, -26)]},
             Lower_arm_L={"r": [(X, -18)]}, Lower_arm_R={"r": [(X, -12)]}, Mutation={"r": [(X, -25)]},
             **{n: {"r": r} for n, r in tent(-12, -6, 4, 10, 10, 6).items()})
P[48] = dict(pose((0, -0.36, -1.13), [(X, 84), (Z, 12)]), Spine_02={"r": [(X, 8)]}, Neck={"r": [(X, 6)]}, Head={"r": [(X, 22), (Y, 30)]},
             Upper_arm_L={"r": [(Y, -56), (X, -20)]}, Upper_arm_R={"r": [(Y, 52), (X, -25)]},
             Lower_arm_L={"r": [(X, -15)]}, Lower_arm_R={"r": [(X, -10)]}, Mutation={"r": [(X, -8)]},
             **{n: {"r": r} for n, r in tent(-6, -2, 6, 6, 4, 6).items()})
P[60] = P[48]
act = bpy.data.actions.new("Ubume_Death")
arm.animation_data.action = act
if hasattr(arm.animation_data, "action_slot") and act.slots: arm.animation_data.action_slot = act.slots[0]
for pb in arm.pose.bones:
    if pb.rotation_mode != 'QUATERNION': pb.rotation_mode = 'QUATERNION'
for fr in sorted(P):
    for pb in arm.pose.bones: pb.matrix_basis = BASE[pb.name].copy()
    bpy.context.view_layer.update()
    spec = P[fr]
    for n in order:
        if n not in spec: continue
        pb = arm.pose.bones[n]; s = spec[n]
        Mx = pb.matrix.copy(); h = Mx.to_translation()
        R = M.Identity(3)
        for ax, a in s.get("r", []): R = M.Rotation(math.radians(a), 3, ax) @ R
        t = s.get("t", V())
        pb.matrix = M.Translation(h + t) @ R.to_4x4() @ M.Translation(-h) @ Mx
        bpy.context.view_layer.update()
    for pb in arm.pose.bones:
        pb.keyframe_insert("rotation_quaternion", frame=fr); pb.keyframe_insert("location", frame=fr)
        if pb.name == "Root": pb.keyframe_insert("scale", frame=fr)
sc.frame_start, sc.frame_end = 1, 60
# rebond et chute : sortie de courbe un peu plus raide à l'impact
for fc in (act.fcurves if hasattr(act, "fcurves") else []):
    for k in fc.keyframe_points:
        if int(k.co.x) == 34: k.interpolation = 'LINEAR'
# action unique dans le fichier pour l'export FBX (l'attente reste dans son propre fichier)
for a in list(bpy.data.actions):
    if a != act: bpy.data.actions.remove(a)
bpy.ops.wm.save_as_mainfile(filepath=OUT)
print("@@ sauvé", OUT)
# sol le plus bas atteint (maillages évalués) par image
dg = bpy.context.evaluated_depsgraph_get()
for fr in (1, 10, 22, 34, 40, 48, 60):
    sc.frame_set(fr); dg = bpy.context.evaluated_depsgraph_get()
    zs = []
    for o in bpy.data.objects:
        if o.type == 'MESH' and any(m.type == 'ARMATURE' for m in o.modifiers):
            e = o.evaluated_get(dg); mw = e.matrix_world; zs += [(mw @ v.co).z for v in e.data.vertices]
    print("@@ image %d : z min %.3f  z max %.3f" % (fr, min(zs), max(zs)))
if PL:
    sc.render.engine = 'BLENDER_WORKBENCH'; sc.render.image_settings.file_format = 'PNG'; sc.display.shading.light = 'STUDIO'; sc.display.shading.show_cavity = True
    sc.render.resolution_x, sc.render.resolution_y = 500, 500
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); sc.collection.objects.link(cam); sc.camera = cam
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.6
    for o in bpy.data.objects:
        if o.name.startswith("WDGT") or o.type == 'EMPTY': o.hide_render = True
    bpy.ops.mesh.primitive_plane_add(size=6, location=(0, 0, 0))
    for vue, loc, rot in (("face", (0, -6, 0.9), (math.pi / 2, 0, 0)), ("profil", (6, 0, 0.9), (math.pi / 2, 0, math.pi / 2))):
        cam.location = loc; cam.rotation_euler = rot
        for fr in (1, 6, 10, 13, 15, 18, 22, 28, 34, 40, 48, 60):
            sc.frame_set(fr); sc.render.filepath = "%s/ubume_mort_%s_%02d.png" % (PL, vue, fr)
            bpy.ops.render.render(write_still=True)
