# Référence « comme Unity » de la COM : FBX Unity + rampe toon de BG.shadergraph + Glass additif,
# caméra identique au jeu Unreal (plongée 55°, FOV 25°, 30 m de large). Lancer :
#   blender -b -P blender_ref_com.py -- sortie.png
import bpy, math, sys, mathutils
OUT = sys.argv[sys.argv.index("--") + 1]
FBX = "D:/_PROJETS/briscard/GIT/EON_Project/Assets/import/fbx/enviro/COMv1.fbx"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
objs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
for o in [o for o in bpy.context.scene.objects if o.type != "MESH"]:
    if o.type in ("LIGHT", "CAMERA"):
        bpy.data.objects.remove(o)
# Taille : mesh Unreal 1536 cm de large × 1,5 (prefab) = 23,04 m
lo = mathutils.Vector((1e9,) * 3); hi = -lo
bpy.context.view_layer.update()
for o in objs:
    for c in o.bound_box:
        w = o.matrix_world @ mathutils.Vector(c)
        lo = mathutils.Vector(map(min, lo, w)); hi = mathutils.Vector(map(max, hi, w))
k = 23.04 / max(hi.x - lo.x, hi.y - lo.y)
root = bpy.data.objects.new("racine", None); bpy.context.scene.collection.objects.link(root)
for o in bpy.context.scene.objects:
    if o.parent is None and o is not root:
        o.parent = root
root.scale = (k, k, k)
bpy.context.view_layer.update()
print("REF taille brute", hi - lo, "facteur", k)

# Lumière = soleil Unreal (vers la lumière, repère Blender : bx=-Y_ue, by=-X_ue, bz=Z_ue)
L = mathutils.Vector((-0.413, 0.492, 0.766)).normalized()


def toon(name, rgb, smooth=0.085, emis=None):
    m = bpy.data.materials.new(name); m.use_nodes = True
    nt = m.node_tree; N = nt.nodes; Lk = nt.links; N.clear()
    out = N.new("ShaderNodeOutputMaterial")
    em = N.new("ShaderNodeEmission")
    if emis:
        em.inputs[0].default_value = (*emis, 1); em.inputs[1].default_value = 3.0
    else:
        geo = N.new("ShaderNodeNewGeometry")
        dot = N.new("ShaderNodeVectorMath"); dot.operation = "DOT_PRODUCT"; dot.inputs[1].default_value = L
        Lk.new(geo.outputs["Normal"], dot.inputs[0])
        d = N.new("ShaderNodeMath"); d.operation = "MULTIPLY_ADD"; d.inputs[1].default_value = 0.5; d.inputs[2].default_value = 0.5
        Lk.new(dot.outputs["Value"], d.inputs[0])
        mr = N.new("ShaderNodeMapRange"); mr.interpolation_type = "SMOOTHSTEP"
        mr.inputs[1].default_value = 0.682; mr.inputs[2].default_value = 0.682 + smooth
        Lk.new(d.outputs[0], mr.inputs[0])
        add = N.new("ShaderNodeMath"); add.operation = "ADD"; add.inputs[1].default_value = 0.4905
        Lk.new(mr.outputs[0], add.inputs[0])
        mul = N.new("ShaderNodeMix"); mul.data_type = "RGBA"; mul.blend_type = "MULTIPLY"; mul.inputs[0].default_value = 1
        mul.inputs[6].default_value = (*rgb, 1)
        comb = N.new("ShaderNodeCombineColor"); [Lk.new(add.outputs[0], comb.inputs[i]) for i in range(3)]
        Lk.new(comb.outputs[0], mul.inputs[7])
        Lk.new(mul.outputs[2], em.inputs[0])
    Lk.new(em.outputs[0], out.inputs[0])
    return m


def glass():
    m = bpy.data.materials.new("Glass"); m.use_nodes = True
    nt = m.node_tree; N = nt.nodes; Lk = nt.links; N.clear()
    out = N.new("ShaderNodeOutputMaterial")
    lw = N.new("ShaderNodeLayerWeight"); lw.inputs[0].default_value = 0.5
    pw = N.new("ShaderNodeMath"); pw.operation = "POWER"; pw.inputs[1].default_value = 5.28
    Lk.new(lw.outputs["Facing"], pw.inputs[0])
    fr = N.new("ShaderNodeMix"); fr.data_type = "RGBA"; fr.blend_type = "MULTIPLY"; fr.inputs[0].default_value = 1
    fr.inputs[7].default_value = (1.26, 0.547 * 1.26, 0, 1)
    cb = N.new("ShaderNodeCombineColor"); [Lk.new(pw.outputs[0], cb.inputs[i]) for i in range(3)]
    Lk.new(cb.outputs[0], fr.inputs[6])
    ad = N.new("ShaderNodeMix"); ad.data_type = "RGBA"; ad.blend_type = "ADD"; ad.inputs[0].default_value = 1
    ad.inputs[6].default_value = (0.358, 0.005, 0.005, 1)
    Lk.new(fr.outputs[2], ad.inputs[7])
    em = N.new("ShaderNodeEmission"); em.inputs[1].default_value = 0.17   # alpha 0,17 en additif
    Lk.new(ad.outputs[2], em.inputs[0])
    tr = N.new("ShaderNodeBsdfTransparent")
    sh = N.new("ShaderNodeAddShader"); Lk.new(tr.outputs[0], sh.inputs[0]); Lk.new(em.outputs[0], sh.inputs[1])
    Lk.new(sh.outputs[0], out.inputs[0])
    try:
        m.surface_render_method = "BLENDED"
    except Exception:
        m.blend_method = "BLEND"
    return m


MAP = {"metal": toon("metal", (0.523545, 0.627, 0.6105413), 0.24),
       "ground": toon("ground", (0.5377358,) * 3),
       "metal.001": toon("metal.001", (0.7830189,) * 3),
       "Material.006": toon("Material.006", (0.735849, 0.6692896, 0.5449448)),
       "light": toon("light", (1, 1, 1), emis=(0.35, 0.8, 1.0))}
G = glass()
for o in objs:
    if not o.material_slots:
        o.data.materials.append(G)
    for s in o.material_slots:
        nm = s.material.name.split(".0")[0] if s.material and s.material.name not in MAP else (s.material.name if s.material else None)
        s.material = MAP.get(s.material.name if s.material else "", MAP.get(nm, G))

# Sol sombre (proche du sol Unreal) pour lire le dôme additif
bpy.ops.mesh.primitive_plane_add(size=80, location=(0, 0, lo.z * k - 0.01))
bpy.context.object.data.materials.append(toon("sol", (0.12, 0.12, 0.12)))

# Caméra du jeu : plongée 55°, FOV 25°, 30 m de large, regarde +X Blender (= -Y Unreal)
cx, cy = (lo.x + hi.x) / 2 * k, (lo.y + hi.y) / 2 * k
# Cadrage du comparatif : centré sur la COM, ~39 m de large (échelle de la capture Unreal : ~33 px/m)
cz = (lo.z + hi.z) / 2 * k
D = 19.5 / math.tan(math.radians(12.5))
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); bpy.context.scene.collection.objects.link(cam)
cam.location = (cx - D * math.cos(math.radians(55)), cy, cz + D * math.sin(math.radians(55)))
cam.rotation_euler = (math.radians(35), 0, math.radians(-90))
cam.data.angle = math.radians(25); cam.data.sensor_fit = "HORIZONTAL"
sc = bpy.context.scene; sc.camera = cam
sc.render.resolution_x, sc.render.resolution_y = 1280, 720
sc.render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items] else "BLENDER_EEVEE"
sc.view_settings.view_transform = "Standard"   # Unity : pas de tonemapping
w = bpy.data.worlds.new("w"); w.color = (0, 0, 0); sc.world = w
sc.render.filepath = OUT
bpy.ops.render.render(write_still=True)
print("REF ok", OUT)
