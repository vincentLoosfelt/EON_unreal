# EON — reporte sur un rig les UV d'un OBJ extrait d'un projet Substance Painter (scene.assbin -> OBJ, triangulé,
# coupé par ensemble de textures : objets <nom>_m<matériau>). L'OBJ est recalé sur le rig (échelle uniforme + centre
# des boîtes englobantes), puis Data Transfer (face la plus proche interpolée), au repos. Copie à part :
#   blender -b <rig.blend> --python blender_uv_depuis_obj.py -- <uv.obj> <sortie.blend> <armature> nom1,nom2,...
import bpy, sys
from mathutils import Vector, kdtree
obj_file, out, arm_name, names = sys.argv[sys.argv.index("--") + 1:][:4]
names = names.split(",")
if bpy.context.object and bpy.context.object.mode != "OBJECT":
    bpy.ops.object.mode_set(mode="OBJECT")
arm = bpy.data.objects[arm_name]
pos = arm.data.pose_position
arm.data.pose_position = "REST"
before = set(bpy.data.objects)
bpy.ops.wm.obj_import(filepath=obj_file)
imp = [o for o in bpy.data.objects if o not in before]


def bbox(objs):
    pts = [o.matrix_world @ v.co for o in objs for v in o.data.vertices]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


tg = [bpy.data.objects[n] for n in names]
lo_t, hi_t = bbox(tg)
lo_s, hi_s = bbox(imp)
k = (hi_t - lo_t).length / (hi_s - lo_s).length
c_t, c_s = (lo_t + hi_t) / 2, (lo_s + hi_s) / 2
for o in imp:
    mw = o.matrix_world.copy()
    for v in o.data.vertices:
        w = mw @ v.co
        v.co = mw.inverted() @ (c_t + (w - c_s) * k)
    o.data.update()
# Contrôle du recalage : distance moyenne sommet du rig -> sommet OBJ le plus proche
allp = [o.matrix_world @ v.co for o in imp for v in o.data.vertices]
kd = kdtree.KDTree(len(allp))
for i, p in enumerate(allp):
    kd.insert(p, i)
kd.balance()
diag = (hi_t - lo_t).length
for t in tg:
    ds = [kd.find(t.matrix_world @ v.co)[2] for v in t.data.vertices]
    print("@@ECART", t.name, "moyen %.5f max %.5f (diagonale %.3f)" % (sum(ds) / len(ds), max(ds), diag))
for n in names:
    parts = [o for o in bpy.data.objects if o not in before and o.name.startswith(n + "_m")]
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts:
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    if len(parts) > 1:
        bpy.ops.object.join()
    src = bpy.context.view_layer.objects.active
    t = bpy.data.objects[n]
    m = t.modifiers.new("UV_depuis_spp", "DATA_TRANSFER")
    m.object = src
    m.use_loop_data = True
    m.data_types_loops = {"UV"}
    m.loop_mapping = "POLYINTERP_NEAREST"
    bpy.ops.object.select_all(action="DESELECT")
    t.select_set(True)
    bpy.context.view_layer.objects.active = t
    bpy.ops.object.modifier_move_to_index(modifier=m.name, index=0)
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(src)
    print("@@UV", n, "reporté")
arm.data.pose_position = pos
bpy.ops.wm.save_as_mainfile(filepath=out, copy=True)
print("@@SORTIE", out, "échelle %.4f" % k)
