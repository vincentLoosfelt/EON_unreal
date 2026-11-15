# EON — Jira : bras gauche (Vincent, 15/11 : « la main est défoncée, une partie flotte » ; « pas de trucs rigides »).
# Sur Saved/Jira/Jira_rig_v3.blend. Au repos, les os du bras gauche étaient pliés vers l'avant, 20 cm au-dessus de la
# main du maillage : les poids partaient sur le pouce et la main tournait autour d'un point hors d'elle.
# 1. Os recalés DANS le maillage (articulations mesurées sur le maillage : coude z 1,265, poignet z 1,135, jointure
#    des doigts z 0,99) ; doigts et pouce suivent la main en bloc.
# 2. Actions recuites : les poses restent identiques en monde (comme dans Unreal, où les animations stockent des
#    transformations locales absolues) ; seule la pose de repos du bras change.
# 3. Skin doux du bras gauche (pièce bras + pièce du pouce) : poids en 1/d^4 vers upper_arm.L, forearm.L, hand.L,
#    2 os max par sommet -> fondu au coude et au poignet. Doigts sur la main.
#   blender -b --factory-startup -P blender_bras_jira_v3.py -- DOSSIER_RENDUS/
import bpy, bmesh, mathutils, collections, math, sys
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
SHOT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else OUT
bpy.ops.wm.open_mainfile(filepath=OUT + "Jira_rig_v3.blend")
sc = bpy.context.scene
B = bpy.data.objects["Armature"]
LOG = []
def log(*a):
    s = " ".join(str(x) for x in a); print("BRAS", s); LOG.append(s)
def mode(m, obj):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT': bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True); bpy.context.view_layer.objects.active = obj
    if m != 'OBJECT': bpy.ops.object.mode_set(mode=m)
def act(arm, ac):
    arm.animation_data_create(); arm.animation_data.action = ac
    if hasattr(arm.animation_data, "action_slot") and ac.slots: arm.animation_data.action_slot = ac.slots[0]

# --- 0. Copie de l'armature avec l'ancienne pose de repos (source de la recuisson)
A = B.copy(); A.data = B.data.copy(); A.name = "OLD"; sc.collection.objects.link(A)
CHAIN = ["upper_arm.L", "forearm.L", "hand.L"]
SUB = [b.name for b in B.data.bones if b.name in CHAIN or any(p.name == "upper_arm.L" for p in b.parent_recursive)]
log("os recalés :", SUB)

# --- 1. Nouvelle pose de repos du bras gauche
EPAULE = B.data.bones["upper_arm.L"].head_local.copy()
COUDE = mathutils.Vector((0.32, 0.025, 1.265))
POIGNET = mathutils.Vector((0.385, 0.025, 1.135))
JOINT = mathutils.Vector((0.40, 0.02, 0.99))
mode('EDIT', B)
eb = B.data.edit_bones
old = {n: (eb[n].matrix.copy(), eb[n].length) for n in SUB}
eb["forearm.L"].use_connect = False; eb["hand.L"].use_connect = False
def place(n, h, t):
    e = eb[n]; roll_ref = e.z_axis.copy()
    e.head = h; e.tail = t; e.align_roll(roll_ref)
place("upper_arm.L", EPAULE, COUDE)
place("forearm.L", COUDE, POIGNET)
place("hand.L", POIGNET, JOINT)
# doigts : même déplacement en bloc que la main
M = eb["hand.L"].matrix @ old["hand.L"][0].inverted()
for n in SUB:
    if n in CHAIN: continue
    e = eb[n]; mat = M @ old[n][0]
    e.matrix = mat; e.length = old[n][1]
eb["forearm.L"].use_connect = True; eb["hand.L"].use_connect = True
mode('OBJECT', B)
for n in CHAIN:
    b = B.data.bones[n]; log("%s : tête (%.2f, %.2f, %.2f) queue (%.2f, %.2f, %.2f)" % (n, *b.head_local, *b.tail_local))

# --- 2. Recuisson : B copie A en monde sur les os recalés, pour chaque action
mode('POSE', B)
for n in SUB:
    c = B.pose.bones[n].constraints.new('COPY_TRANSFORMS'); c.name = "EON_recale"; c.target = A; c.subtarget = n
for pb in B.pose.bones: pb.bone.select = pb.name in SUB
actions = [a for a in bpy.data.actions if a.name.startswith("Jira_")]
for ac in actions:
    src = ac.copy(); src.name = "SRC_" + ac.name
    act(A, src); act(B, ac)
    f0, f1 = int(ac.frame_range[0]), int(ac.frame_range[1])
    bpy.ops.nla.bake(frame_start=f0, frame_end=max(f1, f0 + 1), only_selected=True, visual_keying=True,
                     clear_constraints=False, use_current_action=True, bake_types={'POSE'})
    bpy.data.actions.remove(src)
for n in SUB:
    pb = B.pose.bones[n]; pb.constraints.remove(pb.constraints["EON_recale"])
mode('OBJECT', B)
bpy.data.objects.remove(A)
log("actions recuites :", len(actions))

# --- 3. Skin doux du bras gauche
body = bpy.data.objects["Body"]; me = body.data
bm = bmesh.new(); bm.from_mesh(me); bm.verts.ensure_lookup_table(); seen = set(); isls = []
for v in bm.verts:
    if v.index in seen: continue
    st, isl = [v], []; seen.add(v.index)
    while st:
        x = st.pop(); isl.append(x.index)
        for e in x.link_edges:
            y = e.other_vert(x)
            if y.index not in seen: seen.add(y.index); st.append(y)
    isls.append(isl)
bm.free()
def centre(isl): return sum((me.vertices[i].co for i in isl), mathutils.Vector()) / len(isl)
bras = [i for i in isls if centre(i).x > 0.2 and 600 < len(i) < 1500 and centre(i).z < 1.3]
pouce = [i for i in isls if centre(i).x > 0.35 and len(i) < 300 and centre(i).z < 1.2]
assert len(bras) == 1 and len(pouce) == 1
ids = bras[0] + pouce[0]
seg = {n: (B.data.bones[n].head_local.copy(), B.data.bones[n].tail_local.copy()) for n in CHAIN}
def dist(p, n):
    h, t = seg[n]; q, f = mathutils.geometry.intersect_point_line(p, h, t); f = min(max(f, 0), 1)
    return max((p - (h + (t - h) * f)).length, 0.005)
for g in body.vertex_groups: g.remove(ids)
grp = {n: body.vertex_groups.get(n) or body.vertex_groups.new(name=n) for n in CHAIN}
cnt = collections.Counter()
for i in ids:
    p = me.vertices[i].co
    w = sorted(((1.0 / dist(p, n) ** 4, n) for n in CHAIN), reverse=True)[:2]
    t = sum(x for x, _ in w)
    for x, n in w:
        if x / t > 0.02: grp[n].add([i], x / t, 'REPLACE')
    cnt[w[0][1]] += 1
log("skin doux bras gauche : %d sommets, os principal %s" % (len(ids), dict(cnt)))

# --- 4. Rendus : os visibles au repos, puis poses texturées (bras gauche en gros plan)
for m in [o for o in bpy.data.objects if o.type == 'MESH']: m["_mats"] = [x.name for x in m.data.materials]
sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
sc.render.resolution_x, sc.render.resolution_y = 600, 800
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True; w.node_tree.nodes['Background'].inputs[0].default_value = (.6, .62, .66, 1)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sc.collection.objects.link(sun)
sun.data.energy = 3; sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam; cam.data.lens = 50
tmp = [sun, cam]
# os en bâtons (repos)
B.data.pose_position = 'REST'
ghost = bpy.data.materials.new("TMP_ghost"); ghost.use_nodes = True
gb = ghost.node_tree.nodes["Principled BSDF"]; gb.inputs["Base Color"].default_value = (.5, .5, .55, 1); gb.inputs["Alpha"].default_value = 0.4
if hasattr(ghost, "surface_render_method"): ghost.surface_render_method = 'BLENDED'
for o in [o for o in bpy.data.objects if o.type == 'MESH']:
    o.data.materials.clear(); o.data.materials.append(ghost); o.hide_render = o.name == "Hair"
for n, c in (("upper_arm.L", (0, .9, 1, 1)), ("forearm.L", (.2, 1, .3, 1)), ("hand.L", (.75, .3, 1, 1))):
    h, t = seg[n]
    bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=(t - h).length, location=(h + t) / 2)
    cy = bpy.context.object; cy.rotation_euler = (t - h).to_track_quat('Z', 'Y').to_euler(); tmp.append(cy)
    m = bpy.data.materials.new("TMP_" + n); m.use_nodes = True
    bb = m.node_tree.nodes["Principled BSDF"]; bb.inputs["Base Color"].default_value = c; bb.inputs["Emission Color"].default_value = c; bb.inputs["Emission Strength"].default_value = 2
    cy.data.materials.append(m)
for tag, yaw in (("face", 0), ("profil", 90)):
    r = math.radians(yaw); cam.location = (2.6 * math.sin(r), -2.6 * math.cos(r), 1.3); cam.rotation_euler = (math.radians(90), 0, r)
    sc.render.filepath = SHOT + "bras2_os_%s.png" % tag; bpy.ops.render.render(write_still=True)
for o in tmp[2:]: bpy.data.objects.remove(o)
for o in [o for o in bpy.data.objects if o.type == 'MESH']:
    o.data.materials.clear()
    for n in o["_mats"]: o.data.materials.append(bpy.data.materials[n])
    del o["_mats"]; o.hide_render = False
B.data.pose_position = 'POSE'
T = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/Textures/Textures_Chara/T_Cyborg_V2/"
added = []
for mat, f in (("Material", "T_Cyborg_BaseColor.png"), ("Material.001", "T_Cyborg_BaseColor_2.png")):
    mat = bpy.data.materials.get(mat)
    if not mat: continue
    mat.use_nodes = True; nt = mat.node_tree
    bsdf = [n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'][0]
    im = nt.nodes.new('ShaderNodeTexImage'); im.image = bpy.data.images.load(T + f); added.append((nt, im))
    nt.links.new(im.outputs['Color'], bsdf.inputs['Base Color']); nt.links.new(im.outputs['Alpha'], bsdf.inputs['Alpha'])
for a, fr in (("Jira_shoot_front", 10), ("Jira_run", 6), ("Jira_walk", 30), ("Jira_idle", 1)):
    act(B, bpy.data.actions[a]); sc.frame_set(fr)
    hp = B.matrix_world @ B.pose.bones["forearm.L"].head
    loc = hp + mathutils.Vector((1.0, -1.5, 0.2)); cam.location = loc; cam.rotation_euler = (hp - loc).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = SHOT + "bras2_%s.png" % a[5:]; bpy.ops.render.render(write_still=True)
for nt, im in added: nt.nodes.remove(im)
for m in [m for m in bpy.data.materials if m.name.startswith("TMP_")]: bpy.data.materials.remove(m)
for o in (sun, cam): bpy.data.objects.remove(o)

# --- 5. Sauvegarde + export (maillage + armature, sans animations)
act(B, bpy.data.actions["Jira_idle"]); sc.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=OUT + "Jira_rig_v3.blend")
B.animation_data.action = None
for pb in B.pose.bones: pb.matrix_basis = mathutils.Matrix()
bpy.ops.object.select_all(action='DESELECT'); B.select_set(True)
for m in [o for o in bpy.data.objects if o.type == 'MESH']: m.select_set(True)
bpy.ops.export_scene.fbx(filepath=OUT + "Jira_skin_v3.fbx", use_selection=True, object_types={'ARMATURE', 'MESH'},
                         add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False, mesh_smooth_type='FACE')
log("export ok")
open(OUT + "bras_v3_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
