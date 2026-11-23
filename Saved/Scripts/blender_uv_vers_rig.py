# EON — copie les UV d'un fichier déplié (MASTER_UV) sur les maillages riggés du même nom, au repos.
# Même topologie : copie exacte coin par coin ; sinon Data Transfer (face la plus proche interpolée).
# Écrit une copie à part (le fichier du studio n'est pas modifié) :
#   blender -b <rig.blend> --python blender_uv_vers_rig.py -- <MASTER_UV.blend> <sortie.blend> <armature> obj1,obj2,...
import bpy, sys
src_file, out, arm_name, names = sys.argv[sys.argv.index("--") + 1:][:4]
names = names.split(",")
if bpy.context.object and bpy.context.object.mode != "OBJECT":
    bpy.ops.object.mode_set(mode="OBJECT")
arm = bpy.data.objects[arm_name]
pos = arm.data.pose_position
arm.data.pose_position = "REST"
with bpy.data.libraries.load(src_file, link=False) as (src, dst):
    dst.objects = list(names)
for name, s in zip(names, dst.objects):
    s.name = "UV_source_" + name
    bpy.context.scene.collection.objects.link(s)
    t = bpy.data.objects[name]
    sm, tm = s.data, t.data
    if len(sm.loops) == len(tm.loops) and all(a.vertex_index == b.vertex_index for a, b in zip(sm.loops, tm.loops)):
        su, tu = sm.uv_layers.active.data, tm.uv_layers.active.data
        for i in range(len(tm.loops)):
            tu[i].uv = su[i].uv
        how = "copie exacte"
    else:
        m = t.modifiers.new("UV_depuis_MASTER_UV", "DATA_TRANSFER")
        m.object = s
        m.use_loop_data = True
        m.data_types_loops = {"UV"}
        m.loop_mapping = "POLYINTERP_NEAREST"
        bpy.context.view_layer.objects.active = t
        for o in bpy.context.view_layer.objects:
            o.select_set(o == t)
        bpy.ops.object.modifier_move_to_index(modifier=m.name, index=0)
        bpy.ops.object.modifier_apply(modifier=m.name)
        how = "data transfer"
    bpy.data.objects.remove(s)
    print("@@UV", name, how, len(tm.loops), "coins")
arm.data.pose_position = pos
bpy.ops.wm.save_as_mainfile(filepath=out, copy=True)
print("@@SORTIE", out)
