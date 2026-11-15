# EON — calibration : reconstruit le squelette UEFN (uefn_skel.json) dans Blender, scène en cm, et l'exporte en FBX avec
# un petit cube lié à pelvis. Réimporté dans Unreal, il doit redonner exactement les transforms du mannequin.
#   blender -b --factory-startup -P blender_uefn_calib.py
import bpy, json, mathutils, bmesh
J = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
S = mathutils.Matrix.Diagonal((1, -1, 1))
def ue_frame(b):
    """Repère UE (quat x,y,z,w ; position cm) -> matrice 4x4 Blender (miroir Y)."""
    x, y, z, w = b["q"]
    M = mathutils.Quaternion((w, x, y, z)).to_matrix()
    R = S @ M @ S
    T = mathutils.Matrix.Translation((b["t"][0], -b["t"][1], b["t"][2]))
    return T @ R.to_4x4()
def build(bones, frames, name="Armature"):
    for o in list(bpy.data.objects): bpy.data.objects.remove(o)
    sc = bpy.context.scene
    sc.unit_settings.system = 'METRIC'; sc.unit_settings.scale_length = 0.01
    ad = bpy.data.armatures.new(name); ao = bpy.data.objects.new(name, ad); sc.collection.objects.link(ao)
    bpy.context.view_layer.objects.active = ao; bpy.ops.object.mode_set(mode='EDIT')
    eb = {}
    for b in bones:
        e = ad.edit_bones.new(b["name"]); M = frames[b["name"]]
        e.head = (0, 0, 0); e.tail = (0, 3, 0); e.roll = 0
        e.matrix = M
        if b["parent"] != "None": e.parent = eb[b["parent"]]
        eb[b["name"]] = e
    bpy.ops.object.mode_set(mode='OBJECT')
    return ao
def export(path, objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
                             bake_anim=False, use_armature_deform_only=False, mesh_smooth_type='FACE',
                             apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE', global_scale=1.0)
if __name__ == "__main__":
    d = json.load(open(J + "uefn_skel.json"))
    frames = {b["name"]: ue_frame(b) for b in d["bones"]}
    ao = build(d["bones"], frames)
    me = bpy.data.meshes.new("calib"); bm = bmesh.new(); bmesh.ops.create_cube(bm, size=20); bm.to_mesh(me); bm.free()
    mo = bpy.data.objects.new("calib", me); bpy.context.scene.collection.objects.link(mo)
    mo.location = (0, 0, 95); mo.parent = ao
    g = mo.vertex_groups.new(name="pelvis"); g.add(list(range(len(me.vertices))), 1.0, 'REPLACE')
    mo.modifiers.new("Armature", 'ARMATURE').object = ao
    export(J + "uefn_calib.fbx", [ao, mo])
    print("CALIB ok")
