# EON — Jira skinné : Cyborg_mode.fbx (maillages statiques) + armature de Jira_all_anims.fbx.
#   blender -b --factory-startup -P blender_rig_jira.py
# 1. La pose de l'image 1 d'« idle » colle au maillage : elle devient la pose de repos (armature B).
# 2. Skin automatique (bone heat) sur les os de déformation ; casque rigide sur la tête ; sommets orphelins → os le plus proche.
# 3. Chaque action de l'armature d'origine (A) est recuite sur B (Copy Transforms monde + bake visuel).
# 4. Export FBX (armature « Armature » + maillages + toutes les actions) et .blend de contrôle.
import bpy, mathutils, os
R = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/FBX/"
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
LOG = []
def log(*a):
    s = " ".join(str(x) for x in a); print("JIRA", s); LOG.append(s)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=R + "Chara/Cyborg/Cyborg_mode.fbx")
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
bpy.ops.import_scene.fbx(filepath=R + "Animation/Cyborg/Jira_all_anims.fbx")
A = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
A.name = "SRC"
src_actions = [a for a in bpy.data.actions]
for o in list(bpy.data.objects):
    if o.type not in ('MESH', 'ARMATURE'):
        bpy.data.objects.remove(o)

def mode(m, obj):
    bpy.ops.object.mode_set(mode='OBJECT') if bpy.context.object and bpy.context.object.mode != 'OBJECT' else None
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True); bpy.context.view_layer.objects.active = obj
    if m != 'OBJECT':
        bpy.ops.object.mode_set(mode=m)

# Maillages : transformations appliquées
for m in meshes:
    m.parent = None
    mode('OBJECT', m); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# B = copie de A, transformation appliquée
B = A.copy(); B.data = A.data.copy(); B.animation_data_clear()
bpy.context.scene.collection.objects.link(B)
B.name = "Armature"; B.data.name = "Armature"
mode('OBJECT', B); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

def set_action(obj, act):
    # Blender 4.4+ : une action a des « slots » ; sans slot assigné, rien n'est évalué.
    obj.animation_data.action = act
    if act is not None and hasattr(act, "slots") and len(act.slots):
        obj.animation_data.action_slot = act.slots[0]


def constrain(on):
    for pb in B.pose.bones:
        for c in list(pb.constraints): pb.constraints.remove(c)
        if on:
            c = pb.constraints.new('COPY_TRANSFORMS'); c.target = A; c.subtarget = pb.name

# Pose idle image 1 → repos
set_action(A, bpy.data.actions[[a.name for a in src_actions if a.name.endswith("|idle")][0]])
bpy.context.scene.frame_set(1)
constrain(True)
mode('POSE', B); bpy.ops.pose.select_all(action='SELECT')
bpy.ops.pose.visual_transform_apply()
constrain(False)
bpy.ops.pose.armature_apply(selected=False)
mode('OBJECT', B)

# Os de déformation : pas master ni contrôleurs d'IK
for b in B.data.bones:
    b.use_deform = not (b.name == "master" or b.name.startswith("CTRL_IK"))
deform = [b.name for b in B.data.bones if b.use_deform]
log("os de déformation", len(deform))

# Skin
for m in meshes:
    bpy.ops.object.select_all(action='DESELECT')
    m.select_set(True); B.select_set(True); bpy.context.view_layer.objects.active = B
    bpy.ops.object.parent_set(type='ARMATURE_AUTO')
helmet = bpy.data.objects.get("helmet")
if helmet:
    for vg in list(helmet.vertex_groups): helmet.vertex_groups.remove(vg)
    g = helmet.vertex_groups.new(name="CTRL_head"); g.add(range(len(helmet.data.vertices)), 1.0, 'REPLACE')

# Os autorisés par pièce : cheveux = tête/cou/buste/chaîne CTRL_head.* ; jambes = os des jambes + bassin ;
# corps et épaulières = tout sauf la chaîne des cheveux.
HAIRB = lambda n: n.startswith("CTRL_head") or n in ("CTRL_neck", "CTRL_chest")
LEGB = lambda n: n.startswith(("leg_", "foot", "toes")) or n == "CTRL_pelvis"
ALLOW = {"Hair": HAIRB, "legs_up": LEGB, "legs_down": LEGB}
import bmesh
def islands(me):
    bm = bmesh.new(); bm.from_mesh(me); bm.verts.ensure_lookup_table()
    seen, out = set(), []
    for v in bm.verts:
        if v.index in seen: continue
        stack, isl = [v], []
        seen.add(v.index)
        while stack:
            x = stack.pop(); isl.append(x.index)
            for e in x.link_edges:
                o = e.other_vert(x)
                if o.index not in seen: seen.add(o.index); stack.append(o)
        out.append(isl)
    bm.free(); return out
for m in meshes:
    ok = ALLOW.get(m.name, lambda n: not n.startswith("CTRL_head."))
    for g in list(m.vertex_groups):
        if g.name in [b.name for b in B.data.bones] and not ok(g.name):
            m.vertex_groups.remove(g)
    if m.name in ("Hair", "helmet"):
        continue
    rig = 0
    for isl in islands(m.data):
        pts = [m.data.vertices[i].co for i in isl]
        lo = mathutils.Vector([min(p[k] for p in pts) for k in range(3)]); hi = mathutils.Vector([max(p[k] for p in pts) for k in range(3)])
        if (hi - lo).length > 0.3: continue
        c = sum(pts, mathutils.Vector()) / len(pts)
        best, bd = None, 1e9
        for b in B.data.bones:
            if not (b.use_deform and ok(b.name)): continue
            q, f = mathutils.geometry.intersect_point_line(c, b.head_local, b.tail_local); f = min(max(f, 0), 1)
            d = (c - (b.head_local + (b.tail_local - b.head_local) * f)).length
            if d < bd: best, bd = b.name, d
        for g in m.vertex_groups: g.remove(isl)
        (m.vertex_groups.get(best) or m.vertex_groups.new(name=best)).add(isl, 1.0, 'REPLACE'); rig += 1
    log(m.name, "pièces rigides", rig)

# Sommets sans poids → os le plus proche
segs = {b.name: (b.head_local.copy(), b.tail_local.copy()) for b in B.data.bones if b.use_deform}
def nearest(p, ok=lambda n: True):
    best, bd = None, 1e9
    for n, (h, t) in segs.items():
        if not ok(n): continue
        q, f = mathutils.geometry.intersect_point_line(p, h, t); f = min(max(f, 0), 1)
        d = (p - (h + (t - h) * f)).length
        if d < bd: best, bd = n, d
    return best
for m in meshes:
    names = {g.index: g.name for g in m.vertex_groups}
    fixed = 0
    for v in m.data.vertices:
        if sum(g.weight for g in v.groups if names[g.group] in segs) < 1e-4:
            n = nearest(v.co, ALLOW.get(m.name, lambda n: True))
            g = m.vertex_groups.get(n) or m.vertex_groups.new(name=n)
            g.add([v.index], 1.0, 'REPLACE'); fixed += 1
    log(m.name, "sommets", len(m.data.vertices), "sans poids corrigés", fixed)

# Recuisson des actions
constrain(True)
mode('POSE', B); bpy.ops.pose.select_all(action='SELECT')
baked = []
seen = set()
for act in src_actions:
    short = act.name.split("|")[-1]
    if short in seen or short.endswith(".001"):
        continue
    seen.add(short)
    set_action(A, act)
    bpy.context.scene.frame_set(int(act.frame_range[0]))
    f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
    B.animation_data_create(); B.animation_data.action = None
    bpy.ops.nla.bake(frame_start=f0, frame_end=max(f1, f0 + 1), only_selected=True, visual_keying=True,
                     clear_constraints=False, use_current_action=False, bake_types={'POSE'})
    na = B.animation_data.action
    na.name = "Jira_" + short; na.use_fake_user = True
    baked.append((na.name, f0, f1))
    B.animation_data.action = None
log("actions recuites", baked)
constrain(False)
mode('OBJECT', B)

# Ménage : A et ses actions
for act in src_actions: bpy.data.actions.remove(act)
bpy.data.objects.remove(A)
set_action(B, bpy.data.actions["Jira_idle"])
bpy.context.scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=OUT + "Jira_rig.blend")

bpy.ops.object.select_all(action='DESELECT')
B.select_set(True)
for m in meshes: m.select_set(True)
bpy.ops.export_scene.fbx(filepath=OUT + "Jira_rigged.fbx", use_selection=True, object_types={'ARMATURE', 'MESH'},
                         add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=True,
                         bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
                         use_armature_deform_only=False, mesh_smooth_type='FACE')
log("export ok")
open(OUT + "rig_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
