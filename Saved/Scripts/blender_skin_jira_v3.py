# EON — Jira : corrections du skin dictées par Vincent (15/11), à partir du rig d'origine Saved/Jira/Jira_rig.blend
# (non modifié). Seules ces pièces changent, tout le reste garde ses poids :
#  - ceinture des hanches (riggée sur canon.R) -> CTRL_pelvis, rigide ;
#  - pièce du haut du torse (riggée sur upper_arm.R / upper_arm.L) -> CTRL_chest, rigide ;
#  - épaulettes : pièces rigides, chaque côté entier sur l'os de son épaule (os dominant du côté), cylindres
#    compris (ils suivent leur épaulette).
#   blender -b --factory-startup -P blender_skin_jira_v3.py -- DOSSIER_RENDUS/
import bpy, bmesh, mathutils, collections, math, sys
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
SHOT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else OUT
bpy.ops.wm.open_mainfile(filepath=OUT + "Jira_rig.blend")
arm = bpy.data.objects["Armature"]
sc = bpy.context.scene
LOG = []
def log(*a):
    s = " ".join(str(x) for x in a); print("SKIN", s); LOG.append(s)
MESH = {o.name: o for o in bpy.data.objects if o.type == 'MESH'}
DEFORM = {b.name for b in arm.data.bones if b.use_deform}

def islands(me):
    bm = bmesh.new(); bm.from_mesh(me); bm.verts.ensure_lookup_table()
    seen, out = set(), []
    for v in bm.verts:
        if v.index in seen: continue
        st, isl = [v], []; seen.add(v.index)
        while st:
            x = st.pop(); isl.append(x.index)
            for e in x.link_edges:
                o = e.other_vert(x)
                if o.index not in seen: seen.add(o.index); st.append(o)
        out.append(isl)
    bm.free(); return out
def info(m, isl):
    names = {g.index: g.name for g in m.vertex_groups}
    pts = [m.data.vertices[i].co for i in isl]
    c = sum(pts, mathutils.Vector()) / len(pts)
    w = collections.Counter()
    for i in isl:
        for g in m.data.vertices[i].groups:
            if names[g.group] in DEFORM: w[names[g.group]] += g.weight
    return c, w
def rigid(m, ids, bone):
    for g in m.vertex_groups: g.remove(ids)
    (m.vertex_groups.get(bone) or m.vertex_groups.new(name=bone)).add(ids, 1.0, 'REPLACE')

# --- Ceinture et pièce du haut (maillage Body, pièces au milieu du corps)
body = MESH["Body"]
for k, isl in enumerate(islands(body.data)):
    c, w = info(body, isl)
    tot = sum(w.values()) or 1
    if abs(c.x) > 0.08 or len(isl) > 600:
        continue
    arms = sum(x for n, x in w.items() if n.startswith(("canon", "upper_arm", "forearm", "hand", "thumb", "clavicle")))
    if arms / tot < 0.5:
        continue
    bone = "CTRL_pelvis" if c.z < 1.4 else "CTRL_chest"
    rigid(body, isl, bone)
    log("Body #%d (%d sommets, centre z=%.2f) : %s -> %s" % (k, len(isl), c.z, {n: round(x / tot, 2) for n, x in w.most_common(3)}, bone))

# --- Épaulettes : un côté = un os
sh = MESH["shoulders_armor"]
side_w = {"R": collections.Counter(), "L": collections.Counter()}
names = {g.index: g.name for g in sh.vertex_groups}
for v in sh.data.vertices:
    s = "R" if v.co.x < 0 else "L"
    for g in v.groups:
        n = names[g.group]
        if n.endswith("." + s): side_w[s][n] += g.weight
bone = {s: side_w[s].most_common(1)[0][0] for s in side_w}
log("os des épaulettes : %s (poids par côté : %s)" % (bone, {s: dict(side_w[s].most_common(3)) for s in side_w}))
for k, isl in enumerate(islands(sh.data)):
    c, w = info(sh, isl)
    if len(isl) < 200:   # cylindre : suit l'épaulette de son côté
        rigid(sh, isl, bone["R" if c.x < 0 else "L"]); continue
    R = [i for i in isl if sh.data.vertices[i].co.x < 0]; L = [i for i in isl if sh.data.vertices[i].co.x >= 0]
    rigid(sh, R, bone["R"]); rigid(sh, L, bone["L"])
    log("shoulders_armor #%d : %d sommets -> %s, %d -> %s" % (k, len(R), bone["R"], len(L), bone["L"]))

# --- Rendus : pose de repos colorée par os, puis pose de tir texturée
COL = {"clavicle.R": (1, .1, .1), "upper_arm.R": (1, .55, 0), "canon.R": (1, .95, 0), "clavicle.L": (0, .45, 1),
       "upper_arm.L": (0, .9, 1), "forearm.L": (.2, 1, .3), "hand.L": (.75, .3, 1), "CTRL_chest": (.45, .45, .45),
       "CTRL_spine": (.3, .3, .3), "CTRL_pelvis": (.6, .2, .4)}
def colorize():
    for o in MESH.values():
        nm = {g.index: g.name for g in o.vertex_groups}
        a = o.data.color_attributes.new("Os", 'FLOAT_COLOR', 'POINT')
        for i, v in enumerate(o.data.vertices):
            c = [0, 0, 0]; t = 0
            for g in v.groups:
                col = COL.get(nm[g.group], (1, .4, .8) if nm[g.group].split("_")[0] in ("thumb", "index", "middle", "ring", "pinky") else (.12, .12, .12))
                for j in range(3): c[j] += col[j] * g.weight
                t += g.weight
            a.data[i].color = (*[x / t for x in c], 1) if t else (1, 1, 1, 1)
        m = bpy.data.materials.new("Os_" + o.name); m.use_nodes = True
        nt = m.node_tree; b = nt.nodes["Principled BSDF"]
        at = nt.nodes.new("ShaderNodeVertexColor"); at.layer_name = "Os"
        nt.links.new(at.outputs["Color"], b.inputs["Base Color"])
        o["mats"] = [x.name for x in o.data.materials]
        o.data.materials.clear(); o.data.materials.append(m)
        o.hide_render = o.name == "Hair"
def restore_mats():
    for o in MESH.values():
        o.data.materials.clear()
        for n in o["mats"]: o.data.materials.append(bpy.data.materials[n])
        o.hide_render = False
        o.data.color_attributes.remove(o.data.color_attributes["Os"])
T = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/Textures/Textures_Chara/T_Cyborg_V2/"
for mat, f in (("Material", "T_Cyborg_BaseColor.png"), ("Material.001", "T_Cyborg_BaseColor_2.png")):
    mat = bpy.data.materials.get(mat)
    if not mat: continue
    mat.use_nodes = True; nt = mat.node_tree
    bsdf = [n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'][0]
    im = nt.nodes.new('ShaderNodeTexImage'); im.image = bpy.data.images.load(T + f)
    nt.links.new(im.outputs['Color'], bsdf.inputs['Base Color']); nt.links.new(im.outputs['Alpha'], bsdf.inputs['Alpha'])
    bsdf.inputs['Metallic'].default_value = 0; bsdf.inputs['Roughness'].default_value = 0.6
sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
sc.render.resolution_x, sc.render.resolution_y = 600, 800
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes['Background'].inputs[0].default_value = (0.6, 0.62, 0.66, 1)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sc.collection.objects.link(sun)
sun.data.energy = 3; sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.lens = 50
def shot(name, yaw, z=1.4, d=2.4):
    r = math.radians(yaw)
    cam.location = (d * math.sin(r), -d * math.cos(r), z); cam.rotation_euler = (math.radians(90), 0, r)
    sc.render.filepath = SHOT + name; bpy.ops.render.render(write_still=True)
colorize()
arm.data.pose_position = 'REST'
for yaw in (0, 180): shot("v3_os_%d.png" % yaw, yaw, 1.35, 3.0)
restore_mats()
arm.data.pose_position = 'POSE'
ac = bpy.data.actions["Jira_shoot_front"]; arm.animation_data.action = ac
if hasattr(arm.animation_data, "action_slot") and ac.slots: arm.animation_data.action_slot = ac.slots[0]
sc.frame_set(10)
for yaw in (0, -60, 180): shot("v3_tir_%d.png" % yaw, yaw)
ac = bpy.data.actions["Jira_run"]; arm.animation_data.action = ac
if hasattr(arm.animation_data, "action_slot") and ac.slots: arm.animation_data.action_slot = ac.slots[0]
sc.frame_set(6); shot("v3_course_150.png", 150)

# --- Sauvegarde et export (maillage + armature, sans animations)
ac = bpy.data.actions["Jira_idle"]; arm.animation_data.action = ac
if hasattr(arm.animation_data, "action_slot") and ac.slots: arm.animation_data.action_slot = ac.slots[0]
sc.frame_set(1)
for o in (sun, cam): bpy.data.objects.remove(o)
bpy.ops.wm.save_as_mainfile(filepath=OUT + "Jira_rig_v3.blend")
arm.animation_data.action = None
for pb in arm.pose.bones: pb.matrix_basis = mathutils.Matrix()
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
for m in MESH.values(): m.select_set(True)
bpy.ops.export_scene.fbx(filepath=OUT + "Jira_skin_v3.fbx", use_selection=True, object_types={'ARMATURE', 'MESH'},
                         add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False, mesh_smooth_type='FACE')
log("export ok")
open(OUT + "skin_v3_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
