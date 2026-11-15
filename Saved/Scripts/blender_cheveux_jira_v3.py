# EON — Jira : distance de sécurité cheveux / épaulettes (Vincent, 15/11 : « une petite distance de sécurité, pas
# des montagnes russes »). Sur Saved/Jira/Jira_rig_v3.blend (blender_skin_jira_v3.py), cheveux d'origine.
# Pénétration mesurée sur des poses de tir / marche / course / idle ; décalage vers le dos en une bosse lisse
# (gaussienne sur la distance au repos, R = 12 cm) qui garantit MARGE cm d'écart, jamais sommet par sommet ;
# racine fixe (recul nul en haut, plein 15 cm plus bas).
#   blender -b --factory-startup -P blender_cheveux_jira_v3.py -- DOSSIER_RENDUS/
import bpy, bmesh, mathutils, collections, math, sys
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
SHOT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else OUT
MARGE, R = 0.015, 0.12
bpy.ops.wm.open_mainfile(filepath=OUT + "Jira_rig_v3.blend")
arm = bpy.data.objects["Armature"]; sc = bpy.context.scene
MESH = {o.name: o for o in bpy.data.objects if o.type == 'MESH'}
hair = MESH["Hair"]
LOG = []
def log(*a):
    s = " ".join(str(x) for x in a); print("CHEV", s); LOG.append(s)
def act(name, fr):
    ac = bpy.data.actions[name]; arm.animation_data.action = ac
    if hasattr(arm.animation_data, "action_slot") and ac.slots: arm.animation_data.action_slot = ac.slots[0]
    sc.frame_set(fr)
POSES = [("Jira_shoot_front", f) for f in (1, 5, 10, 15)] + [("Jira_run", f) for f in (1, 6, 11, 16)] + \
        [("Jira_walk", f) for f in (1, 15, 30, 45)] + [("Jira_idle", f) for f in (1, 100)] + \
        [("Jira_shoot_back", 10), ("Jira_shoot_left", 10), ("Jira_shoot_right", 10)]
def besoin():
    """Pour chaque sommet des cheveux : profondeur dans l'armure + MARGE (0 si déjà à MARGE ou plus)."""
    need = collections.defaultdict(float)
    for a, fr in POSES:
        act(a, fr); dg = bpy.context.evaluated_depsgraph_get()
        trees = []
        for n in ("shoulders_armor", "Body"):
            ev = MESH[n].evaluated_get(dg); me = ev.to_mesh()
            bm = bmesh.new(); bm.from_mesh(me); bm.transform(MESH[n].matrix_world)
            trees.append(mathutils.bvhtree.BVHTree.FromBMesh(bm)); bm.free(); ev.to_mesh_clear()
        hv = hair.evaluated_get(dg); hm = hv.to_mesh()
        for v in hm.vertices:
            p = hair.matrix_world @ v.co
            for t in trees:
                loc, nor, idx, dist = t.find_nearest(p, 0.1)
                if loc is None: continue
                s = (p - loc).dot(nor)
                if s < MARGE: need[v.index] = max(need[v.index], MARGE - s)
        hv.to_mesh_clear()
    return need

T = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/Textures/Textures_Chara/T_Cyborg_V2/"
for mat, f in (("Material", "T_Cyborg_BaseColor.png"), ("Material.001", "T_Cyborg_BaseColor_2.png")):
    mat = bpy.data.materials.get(mat)
    if not mat: continue
    mat.use_nodes = True; nt = mat.node_tree
    bsdf = [n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'][0]
    if not any(n.type == 'TEX_IMAGE' for n in nt.nodes):
        im = nt.nodes.new('ShaderNodeTexImage'); im.image = bpy.data.images.load(T + f)
        nt.links.new(im.outputs['Color'], bsdf.inputs['Base Color']); nt.links.new(im.outputs['Alpha'], bsdf.inputs['Alpha'])
sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
sc.render.resolution_x, sc.render.resolution_y = 600, 800
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes['Background'].inputs[0].default_value = (0.6, 0.62, 0.66, 1)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sc.collection.objects.link(sun)
sun.data.energy = 3; sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.lens = 50
def shots(tag):
    for a, fr, yaw in (("Jira_shoot_front", 10, 180), ("Jira_shoot_front", 10, 120), ("Jira_run", 6, 150), ("Jira_idle", 1, 180)):
        act(a, fr); r = math.radians(yaw); d = 2.4
        cam.location = (d * math.sin(r), -d * math.cos(r), 1.4); cam.rotation_euler = (math.radians(90), 0, r)
        sc.render.filepath = SHOT + "chev_%s_%s_%d.png" % (tag, a[5:], yaw); bpy.ops.render.render(write_still=True)

shots("avant")
rest = [v.co.copy() for v in hair.data.vertices]
# Racine des cheveux fixe (collée à la nuque) : recul nul en haut, plein 15 cm plus bas, en courbe douce.
ZTOP = max(p.z for p in rest)
def rampe(p):
    t = min(max((ZTOP - p.z) / 0.15, 0.0), 1.0)
    return t * t * (3 - 2 * t)
total = [0.0] * len(rest)
need = besoin()
log("avant : %d sommets à moins de %.1f cm, max %.1f cm" % (len(need), MARGE * 100, 100 * max(need.values(), default=0)))
for it in range(4):
    if not need: break
    # bosse lisse : chaque sommet recule de max_j besoin_j * gauss(distance au repos)
    add = [max((d * math.exp(-((rest[i] - rest[j]).length / R) ** 2) for j, d in need.items()), default=0) for i in range(len(rest))]
    for i, v in enumerate(hair.data.vertices):
        total[i] += add[i] * rampe(rest[i])
        v.co = rest[i] + mathutils.Vector((0, total[i], 0))   # +Y = vers le dos
    hair.data.update()
    need = besoin()
    log("passe %d : recul max %.1f cm ; %d sommets encore à moins de %.1f cm (max %.1f cm)" % (it + 1, 100 * max(total), len(need), MARGE * 100, 100 * max(need.values(), default=0)))
shots("apres")

act("Jira_idle", 1)
for o in (sun, cam): bpy.data.objects.remove(o)
bpy.ops.wm.save_as_mainfile(filepath=OUT + "Jira_rig_v3.blend")
arm.animation_data.action = None
for pb in arm.pose.bones: pb.matrix_basis = mathutils.Matrix()
bpy.ops.object.select_all(action='DESELECT'); arm.select_set(True)
for m in MESH.values(): m.select_set(True)
bpy.ops.export_scene.fbx(filepath=OUT + "Jira_skin_v3.fbx", use_selection=True, object_types={'ARMATURE', 'MESH'},
                         add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False, mesh_smooth_type='FACE')
log("export ok")
open(OUT + "cheveux_v3_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
