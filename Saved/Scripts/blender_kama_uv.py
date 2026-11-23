# EON — Kama : le rig du studio (Slasher_mode_MASTER.blend, 26/07) porte d'anciennes UV ; la texture
# T_Slasher.mode_MASTER_UV_Body_BaseColor a été peinte sur Slasher_mode_MASTER_UV.blend (21/07, autre topologie).
# On reporte les UV du corps (Data Transfer, face la plus proche interpolée) sur le corps riggé, au repos,
# et on écrit une copie à part (le fichier du studio n'est pas modifié) :
#   blender -b Slasher_mode_MASTER.blend --python blender_kama_uv.py -- <Slasher_mode_MASTER_UV.blend> <sortie.blend>
import bpy, sys
src_file, out = sys.argv[sys.argv.index("--") + 1:][:2]
if bpy.context.object and bpy.context.object.mode != "OBJECT":
    bpy.ops.object.mode_set(mode="OBJECT")
# Pose de repos pendant le report
arm = bpy.data.objects["rig"]
pos = arm.data.pose_position
arm.data.pose_position = "REST"
with bpy.data.libraries.load(src_file, link=False) as (src, dst):
    dst.objects = ["body"]
uvsrc = dst.objects[0]
uvsrc.name = "UV_source_body"
bpy.context.scene.collection.objects.link(uvsrc)
tgt = bpy.data.objects["body"]
uvsrc.matrix_world = uvsrc.matrix_world  # même repère que dans son fichier
m = tgt.modifiers.new("UV_depuis_MASTER_UV", "DATA_TRANSFER")
m.object = uvsrc
m.use_loop_data = True
m.data_types_loops = {"UV"}
m.loop_mapping = "POLYINTERP_NEAREST"
bpy.context.view_layer.objects.active = tgt
for o in bpy.context.view_layer.objects:
    o.select_set(o == tgt)
bpy.ops.object.modifier_move_to_index(modifier=m.name, index=0)
bpy.ops.object.modifier_apply(modifier=m.name)
bpy.data.objects.remove(uvsrc)
arm.data.pose_position = pos
bpy.ops.wm.save_as_mainfile(filepath=out, copy=True)
print("@@KAMA_UV", out, "boucles", len(tgt.data.loops))
