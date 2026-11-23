# EON — export FBX des ennemis du studio (Drive BRISCARD_PROD/3D, copie dans Reference/3D_Studio/Drive_20261123).
# Un appel par fichier .blend :
#   blender -b <fichier.blend> --python blender_ennemis_export.py -- <sortie.fbx> <armature|-> <anim 0/1> [objets...]
# Armature : seuls ses maillages skinnés (modificateur Armature) partent, os de déformation seulement (rigs Auto-Rig Pro
# du Slasher et du Shooter : contrôleurs exclus). Sans armature ("-") : maillages statiques nommés (ou tous, sauf le sol).
import bpy, sys, os

argv = sys.argv[sys.argv.index("--") + 1:]
out, arm_name, anim = argv[0], argv[1], argv[2] == "1"
names = argv[3:]
os.makedirs(os.path.dirname(out), exist_ok=True)

# Collections exclues ou cachées (Kamikaze_run_variation) : sans évaluation, la pose ne bouge pas et l'anim part vide.
def show(lc):
    lc.exclude = False
    lc.hide_viewport = False
    lc.collection.hide_viewport = False
    for c in lc.children:
        show(c)
show(bpy.context.view_layer.layer_collection)
for o in bpy.context.view_layer.objects:
    o.hide_set(False)
    o.select_set(False)

sel = []
if arm_name != "-":
    arm = bpy.data.objects[arm_name]
    sel.append(arm)
    for o in bpy.data.objects:
        if o.type == "MESH" and any(m.type == "ARMATURE" and m.object == arm for m in o.modifiers):
            sel.append(o)
    types = {"ARMATURE", "MESH"}
else:
    for o in bpy.data.objects:
        if o.type == "MESH" and (o.name in names if names else o.name not in ("Sol", "Ground")):
            sel.append(o)
    types = {"MESH"}

for o in sel:
    o.hide_viewport = False
    o.hide_set(False)
    o.select_set(True)
bpy.context.view_layer.objects.active = sel[0]

kw = dict(filepath=out, use_selection=True, object_types=types, add_leaf_bones=False, apply_unit_scale=True,
          apply_scale_options="FBX_SCALE_NONE", global_scale=1.0, mesh_smooth_type="FACE", use_mesh_modifiers=True)
if arm_name != "-":
    kw.update(use_armature_deform_only=True, bake_anim=anim)
    if anim:
        a = arm.animation_data.action
        s, e = a.frame_range
        bpy.context.scene.frame_start, bpy.context.scene.frame_end = int(s), int(e)
        kw.update(bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                  bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
else:
    kw.update(bake_anim=False)
bpy.ops.export_scene.fbx(**kw)
print("@@EXPORT", os.path.basename(out), "objets=%d" % len(sel), [o.name for o in sel][:8],
      ("action=%s %s" % (arm.animation_data.action.name, tuple(arm.animation_data.action.frame_range))) if anim else "")
