# Rendus de contrôle du Jira skinné (Jira_rig.blend) : textures T_Cyborg_V2, plusieurs actions/images.
import bpy, math, sys
T = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/Textures/Textures_Chara/T_Cyborg_V2/"
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
bpy.ops.wm.open_mainfile(filepath=OUT + "Jira_rig.blend")
sc = bpy.context.scene
TEX = {"Material": "T_Cyborg_BaseColor.png", "Material.001": "T_Cyborg_BaseColor_2.png"}
for mat in bpy.data.materials:
    if mat.name not in TEX: continue
    mat.use_nodes = True; nt = mat.node_tree
    bsdf = [n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'][0]
    im = nt.nodes.new('ShaderNodeTexImage'); im.image = bpy.data.images.load(T + TEX[mat.name])
    nt.links.new(im.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(im.outputs['Alpha'], bsdf.inputs['Alpha'])
    bsdf.inputs['Metallic'].default_value = 0; bsdf.inputs['Roughness'].default_value = 0.6
    mat.blend_method = 'HASHED' if hasattr(mat, 'blend_method') else None
sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
sc.render.resolution_x, sc.render.resolution_y = 600, 800
sc.render.film_transparent = False
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes['Background'].inputs[0].default_value = (0.35, 0.37, 0.42, 1); w.node_tree.nodes['Background'].inputs[1].default_value = 1.0
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sc.collection.objects.link(sun)
sun.data.energy = 3; sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.lens = 50
arm = bpy.data.objects["Armature"]
shots = [("idle", 1, 0), ("idle", 1, 140), ("run", 6, 60), ("run", 14, -90), ("walk", 20, 30), ("hacking", 25, 20)]
for act, fr, yaw in shots:
    ac = bpy.data.actions["Jira_" + act]; arm.animation_data.action = ac; arm.animation_data.action_slot = ac.slots[0]
    sc.frame_set(fr)
    a = math.radians(yaw); d = 4.2
    cam.location = (d * math.sin(a), -d * math.cos(a), 1.3)
    cam.rotation_euler = (math.radians(88), 0, a)
    sc.render.filepath = OUT + "shot_%s_%d_%d.png" % (act, fr, yaw)
    bpy.ops.render.render(write_still=True)
