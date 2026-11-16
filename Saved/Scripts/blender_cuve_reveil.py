# EON — cuve de réveil du Kibo : Cuve_1 (FBX Unity, lecture seule) séparée en deux maillages dans Blender :
#   Cuve_Reveil_Corps : tout sauf le tube de verre ;  Cuve_Reveil_Verre : le tube de verre seul (translucide, il s'ouvre).
# Échelle ×100 dans le maillage (1 m Blender = 100 cm UE), pivot au sol au centre, comme Cuve_1 importé.
#   blender -b -P blender_cuve_reveil.py
import bpy, bmesh, mathutils, os
FBX = "D:/_PROJETS/briscard/GIT/EON_Project/Assets/import/fbx/Cuve_1.fbx"
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Import/Cuve_Reveil"
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
o = [x for x in bpy.context.scene.objects if x.type == "MESH"][0]
bpy.ops.object.select_all(action="DESELECT"); o.select_set(True); bpy.context.view_layer.objects.active = o
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
o.data.transform(mathutils.Matrix.Scale(100.0, 4))
gi = [i for i, m in enumerate(o.data.materials) if m and m.name.startswith("Glass")][0]
bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="DESELECT")
bpy.ops.mesh.select_mode(type="FACE"); bpy.ops.object.mode_set(mode="OBJECT")
# le tube de verre seul (faces Glass entre 1 m et 3,4 m) ; le dôme de verre du sommet reste sur le corps
for f in o.data.polygons:
    zs = [(o.matrix_world @ o.data.vertices[v].co).z for v in f.vertices]
    f.select = f.material_index == gi and min(zs) > 100 and max(zs) < 340
bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.separate(type="SELECTED")
bpy.ops.object.mode_set(mode="OBJECT")
corps = o; verre = [x for x in bpy.context.scene.objects if x.type == "MESH" and x != o][0]
corps.name = "Cuve_Reveil_Corps"; verre.name = "Cuve_Reveil_Verre"
for x in (corps, verre):
    bpy.ops.object.select_all(action="DESELECT"); x.select_set(True); bpy.context.view_layer.objects.active = x
    bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT"); bpy.ops.mesh.delete_loose(); bpy.ops.object.mode_set(mode="OBJECT")
# le verre ne garde que son matériau
bpy.context.view_layer.objects.active = verre
for i in reversed(range(len(verre.data.materials))):
    if i != gi:
        verre.active_material_index = i; bpy.ops.object.material_slot_remove()
log = []
for x in (corps, verre):
    ps = [x.matrix_world @ v.co for v in x.data.vertices]
    mn = [min(p[k] for p in ps) for k in range(3)]; mx = [max(p[k] for p in ps) for k in range(3)]
    log.append("%s verts %d min %s max %s mats %s loc %s" % (x.name, len(ps), [round(v, 1) for v in mn], [round(v, 1) for v in mx], [m.name for m in x.data.materials], tuple(x.location)))
    bpy.ops.object.select_all(action="DESELECT"); x.select_set(True); bpy.context.view_layer.objects.active = x
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, x.name + ".fbx"), use_selection=True, object_types={"MESH"}, apply_scale_options="FBX_SCALE_NONE", bake_space_transform=False)
open(os.path.join(OUT, "cuve.txt"), "w", encoding="utf-8").write("\n".join(log))
print("\n".join(log))
