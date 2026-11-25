# EON — Jira riggée sur le squelette du mannequin UEFN (Vincent, 15/11 : « même squelette et même locomotion que le
# mannequin d'Unreal »). Part de Saved/Jira/Jira_rig_v3.blend (skin v3 validé), produit Jira_UEFN.blend / Jira_UEFN.fbx.
#  1. pose : membres de Jira tournés dans la direction des os du mannequin (jambes avec le pied vers l'avant), puis
#     cette pose devient la pose de repos des maillages ;
#  2. squelette : les 91 os UEFN, orientations exactes du mannequin, positions calées sur les articulations de Jira ;
#  3. poids : os Jira -> os UEFN (pièces rigides gardées rigides ; colonne CTRL_spine répartie sur spine_01..04 ;
#     os des cheveux -> leur ancêtre) ;
#  4. export FBX en cm (même réglage que la calibration blender_uefn_calib.py, vérifiée à 0,001 cm / 0,01°).
#   blender -b --factory-startup -P blender_jira_uefn.py -- DOSSIER_RENDUS/
import bpy, json, mathutils, math, sys, collections, importlib.util
V = mathutils.Vector
J = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
SHOT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else J
spec = importlib.util.spec_from_file_location("calib", "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/blender_uefn_calib.py")
calib = importlib.util.module_from_spec(spec); spec.loader.exec_module(calib)
LOG = []
def log(*a):
    s = " ".join(str(x) for x in a); print("JU", s); LOG.append(s)

U = json.load(open(J + "uefn_skel.json"))["bones"]
UF = {b["name"]: calib.ue_frame(b) for b in U}            # repères UEFN, Blender, cm
UP = {n: M.to_translation() for n, M in UF.items()}
UPAR = {b["name"]: b["parent"] for b in U}

bpy.ops.wm.open_mainfile(filepath=J + "Jira_rig_v3.blend")
arm = bpy.data.objects["Armature"]; arm.name = "JiraOld"; arm.data.name = "JiraOld"
sc = bpy.context.scene
MESH = [o for o in bpy.data.objects if o.type == 'MESH']
arm.animation_data_clear() if arm.animation_data else None
for pb in arm.pose.bones: pb.matrix_basis = mathutils.Matrix()
arm.data.pose_position = 'POSE'
bpy.context.view_layer.update()

# ---------------------------------------------------------------- 1. pose du mannequin
def frame(p, h):
    x = p.normalized(); z = x.cross(h).normalized(); y = z.cross(x)
    return mathutils.Matrix((x, y, z)).transposed()
def flat(v):
    return V((v.x, v.y, 0)).normalized()
def cur(n):
    pb = arm.pose.bones[n]; return pb.head.copy(), pb.tail.copy()
def udir(a, b):
    return (UP[b] - UP[a]).normalized()
def rotate(n, R):
    pb = arm.pose.bones[n]; M = pb.matrix.copy()
    t = M.to_translation()
    pb.matrix = mathutils.Matrix.Translation(t) @ (R @ M.to_3x3()).to_4x4()
    bpy.context.view_layer.update()
def align(n, target, hint_now=None, hint_target=None):
    h, t = cur(n); d = t - h
    if hint_now is None:
        R = d.normalized().rotation_difference(target.normalized()).to_matrix()
    else:
        R = frame(target, hint_target) @ frame(d, hint_now).inverted()
    rotate(n, R)
    h2, t2 = cur(n)
    log("%-12s écart après alignement : %.2f°" % (n, math.degrees((t2 - h2).angle(target))))
# Jambes (25/11) : les os des jambes du rig d'origine ne sont pas dans le maillage (os du pied G tourné de 53° vers
# l'extérieur, hanche D décalée de 9 cm) et leurs poids sont asymétriques. Se fier aux os tordait la jambe gauche vers
# l'intérieur. On part donc du maillage, symétrique, jambes droites et pieds vers l'avant :
#  - articulations (hanche, genou, cheville, ball) mesurées sur les sections du maillage, puis symétrisées ;
#  - poids des pièces rigides legs_up / legs_down refaits par la géométrie (mêmes des deux côtés) ;
#  - chaque segment tourné autour de ses vraies articulations vers les directions du mannequin (skinning linéaire à la main),
#    CONF des os de jambe = cette transformation appliquée à leur repos (transfert exact des anims de corps).
OB = arm.data.bones
LEGM = {o.name: o for o in MESH if o.name in ("legs_up", "legs_down")}
def section(objs, s, z0, z1):
    vs = [v.co for o in objs for v in o.data.vertices if z0 <= v.co.z <= z1 and (v.co.x > 0) == (s == "L")]
    return sum(vs, V()) / len(vs)
def zavg(n): return (OB[n + ".L"].head_local.z + OB[n + ".R"].head_local.z) / 2
JM = {}
for s in "LR":
    up, dn = LEGM["legs_up"], LEGM["legs_down"]
    ztop = max(v.co.z for v in up.data.vertices)
    zh, zk, za, zt = zavg("leg_up"), zavg("leg_down"), zavg("foot"), zavg("toes")
    hs = section([up], s, min(zh, ztop) - 0.08, min(zh, ztop) - 0.02); H = V((hs.x, hs.y, zh))
    ks = section([up, dn], s, zk - 0.02, zk + 0.02); K = V((ks.x, ks.y, zk))
    as_ = section([dn], s, za + 0.02, za + 0.06); A = V((as_.x, as_.y, za))
    foot = [v.co for v in dn.data.vertices if v.co.z < za and (v.co.x > 0) == (s == "L")]
    tip = min(foot, key=lambda c: c.y)
    B = A.lerp(V((tip.x, tip.y, A.z)), 0.70); B.z = zt
    JM[s] = {"H": H, "K": K, "A": A, "B": B, "T": V((tip.x, tip.y, zt))}
for k in ("H", "K", "A", "B", "T"):   # symétrie gauche/droite (le maillage est symétrique en x)
    l, r = JM["L"][k], JM["R"][k]
    m = (l + V((-r.x, r.y, r.z))) / 2
    JM["L"][k], JM["R"][k] = m, V((-m.x, m.y, m.z))
def sm(t):
    t = min(max(t, 0.0), 1.0); return t * t * (3 - 2 * t)
for o in LEGM.values():   # poids géométriques des pièces de jambe
    for s in "LR":
        for n in ("leg_up", "leg_down", "foot", "toes"):
            if o.vertex_groups.get(n + "." + s) is None: o.vertex_groups.new(name=n + "." + s)
    G = {g.name: g for g in o.vertex_groups}
    for v in o.data.vertices:
        s = "L" if v.co.x > 0 else "R"; J_ = JM[s]; c = v.co
        wk = sm((J_["K"].z + 0.04 - c.z) / 0.08)                 # 0 au-dessus du genou -> 1 en dessous
        wa = sm((J_["A"].z + 0.05 - c.z) / 0.07)                 # pied sous la cheville
        wb = sm((J_["B"].y + 0.02 - c.y) / 0.04) * sm((0.10 - c.z) / 0.04)   # avant du pied (vers -y) -> ball
        w = {"leg_up": 1 - wk, "leg_down": wk * (1 - wa), "foot": wk * wa * (1 - wb), "toes": wk * wa * wb}
        for g in [g.group for g in v.groups]:
            o.vertex_groups[g].remove([v.index])
        for n, x in w.items():
            if x > 1e-4: G[n + "." + s].add([v.index], x, 'REPLACE')
def tr(Xold, Xnew, R):
    return mathutils.Matrix.Translation(Xnew) @ R.to_4x4() @ mathutils.Matrix.Translation(-Xold)
LEGT = {}
side = V((0, 0, 1))
for s, u in (("L", "l"), ("R", "r")):
    J_ = JM[s]; fwd_u = flat(UP["ball_" + u] - UP["foot_" + u]); fwd_m = flat(J_["B"] - J_["A"])
    bx = (UF["ball_" + u].to_3x3() @ V((1, 0, 0)))          # axe X UE de ball = sa longueur
    Rt = frame(udir("thigh_" + u, "calf_" + u), fwd_u) @ frame(J_["K"] - J_["H"], fwd_m).inverted()
    Rc = frame(udir("calf_" + u, "foot_" + u), fwd_u) @ frame(J_["A"] - J_["K"], fwd_m).inverted()
    Rf = frame(udir("foot_" + u, "ball_" + u), side.cross(fwd_u)) @ frame(J_["B"] - J_["A"], side.cross(fwd_m)).inverted()
    Rb = frame(bx, side.cross(fwd_u)) @ frame(J_["T"] - J_["B"], side.cross(fwd_m)).inverted()
    H2 = J_["H"]; K2 = H2 + Rt @ (J_["K"] - J_["H"]); A2 = K2 + Rc @ (J_["A"] - J_["K"]); B2 = A2 + Rf @ (J_["B"] - J_["A"])
    LEGT["leg_up." + s] = tr(J_["H"], H2, Rt); LEGT["leg_down." + s] = tr(J_["K"], K2, Rc)
    LEGT["foot." + s] = tr(J_["A"], A2, Rf); LEGT["toes." + s] = tr(J_["B"], B2, Rb)
    J_.update({"H2": H2, "K2": K2, "A2": A2, "B2": B2})
    log("jambe %s : hanche %s genou %s cheville %s ball %s (m, maillage)" % (s, *(tuple(round(x, 3) for x in J_[k]) for k in ("H", "K", "A", "B"))))
    for n, R, d in (("cuisse", Rt, J_["K"] - J_["H"]), ("tibia", Rc, J_["A"] - J_["K"]), ("pied", Rf, J_["B"] - J_["A"])):
        log("   %s %s tourné de %.1f°" % (s, n, math.degrees(R.to_quaternion().angle)))
def leg_of(n):   # os de jambe d'origine dont dépend le groupe n (ou None)
    b = OB.get(n)
    while b is not None and b.name not in LEGT: b = b.parent
    return b.name if b is not None else None
for o in MESH:   # skinning linéaire des jambes autour des articulations du maillage
    nm = {g.index: g.name for g in o.vertex_groups}
    for v in o.data.vertices:
        acc = V(); tot = 0.0
        for g in v.groups:
            n = nm[g.group]
            if g.weight <= 0 or OB.get(n) is None or not OB[n].use_deform: continue
            L_ = leg_of(n); tot += g.weight
            acc += g.weight * ((LEGT[L_] @ v.co) if L_ else v.co)
        if tot > 0 and any(leg_of(nm[g.group]) for g in v.groups if g.weight > 0): v.co = acc / tot
    o.data.update()
align("clavicle.L", udir("clavicle_l", "upperarm_l"))
align("upper_arm.L", udir("upperarm_l", "lowerarm_l"))
align("forearm.L", udir("lowerarm_l", "hand_l"))
align("hand.L", udir("hand_l", "middle_01_l"))
align("clavicle.R", udir("clavicle_r", "upperarm_r"))
align("upper_arm.R", udir("upperarm_r", "lowerarm_r"))
align("canon.R", udir("lowerarm_r", "hand_r"))
CONF = {pb.name: pb.matrix.copy() for pb in arm.pose.bones}   # pose « mannequin » des os de Jira (m)
for n, T_ in LEGT.items(): CONF[n] = T_ @ OB[n].matrix_local   # jambes : transformation du maillage, pas la pose des os

# articulations de Jira dans la pose du mannequin (cm)
JP = {}
for b in arm.pose.bones:
    JP[b.name] = (arm.matrix_world @ b.head) * 100
    JP[b.name + "@tail"] = (arm.matrix_world @ b.tail) * 100

# maillages : pose appliquée, coordonnées monde en cm, plus de parent
for o in MESH:
    for m in [m for m in o.modifiers if m.type != 'ARMATURE']: log("modificateur gardé", o.name, m.type)
    if o.data.shape_keys: log("ATTENTION clés de forme", o.name)
    with bpy.context.temp_override(object=o, active_object=o, selected_objects=[o], selected_editable_objects=[o]):
        bpy.ops.object.modifier_apply(modifier=[m for m in o.modifiers if m.type == 'ARMATURE'][0].name)
    mw = o.matrix_world.copy()
    o.parent = None; o.matrix_world = mathutils.Matrix()
    o.data.transform(mathutils.Matrix.Scale(100, 4) @ mw)
    o.data.update()

# ---------------------------------------------------------------- 2. squelette UEFN calé sur Jira
P = {}
P["root"] = V((0, 0, 0)); P["attach"] = V((0, 0, 0))
P["pelvis"] = JP["CTRL_pelvis"]
for s, u in (("L", "l"), ("R", "r")):
    P["thigh_" + u] = JP["leg_up." + s]; P["calf_" + u] = JP["leg_down." + s]
    P["foot_" + u] = JP["foot." + s]; P["ball_" + u] = JP["toes." + s]
    P["thigh_" + u], P["calf_" + u] = JM[s]["H2"] * 100, JM[s]["K2"] * 100       # jambes : articulations du maillage
    P["foot_" + u], P["ball_" + u] = JM[s]["A2"] * 100, JM[s]["B2"] * 100
P["clavicle_l"] = JP["clavicle.L"]; P["upperarm_l"] = JP["upper_arm.L"]; P["lowerarm_l"] = JP["forearm.L"]; P["hand_l"] = JP["hand.L"]
P["clavicle_r"] = JP["clavicle.R"]; P["upperarm_r"] = JP["upper_arm.R"]; P["lowerarm_r"] = JP["canon.R"]; P["hand_r"] = JP["canon.R@tail"]
P["neck_01"] = JP["CTRL_neck"]; P["head"] = JP["CTRL_head"]
# colonne et cou : mêmes proportions que le mannequin entre pelvis et neck_01, neck_01 et head
def chain(names, a, b):
    L = [0.0]
    for i in range(1, len(names)): L.append(L[-1] + (UP[names[i]] - UP[names[i - 1]]).length)
    for n, l in zip(names[1:-1], L[1:-1]): P[n] = P[a].lerp(P[b], l / L[-1])
chain(["pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01"], "pelvis", "neck_01")
chain(["neck_01", "neck_02", "head"], "neck_01", "head")
for n in ("pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01", "neck_02", "head"):
    P[n] = V((0.0, P[n].y, P[n].z))   # axe du corps : le maillage est symétrique en x, les os CTRL d'origine sont décalés
# facteur d'échelle par segment (os placé -> son enfant principal)
MAIN = {"thigh": "calf", "calf": "foot", "foot": "ball", "upperarm": "lowerarm", "lowerarm": "hand", "clavicle": "upperarm"}
def seg_scale(n):
    base, _, side = n.rpartition("_")
    if base in MAIN and n in P and (MAIN[base] + "_" + side) in P:
        c = MAIN[base] + "_" + side
        return (P[c] - P[n]).length / max((UP[c] - UP[n]).length, 1e-6)
    if base == "hand" and n in P:   # main : longueur jusqu'aux phalanges
        return 1.0 if side == "r" else (JP["hand.L@tail"] - P[n]).length / (UP["middle_01_l"] - UP[n]).length
    return None
order = [b["name"] for b in U]
SC = {}
for n in order:
    p = UPAR[n]
    if n in P:
        SC[n] = seg_scale(n) or (SC.get(p, 1.0) if p != "None" else 1.0); continue
    if n.startswith("ik_") or n in ("ik_foot_root", "ik_hand_root"):
        continue
    s = SC.get(p, 1.0)
    P[n] = P[p] + (UP[n] - UP[p]) * s; SC[n] = s
P["ik_foot_root"] = V((0, 0, 0)); P["ik_hand_root"] = V((0, 0, 0))
P["ik_foot_l"] = P["foot_l"]; P["ik_foot_r"] = P["foot_r"]
P["ik_hand_gun"] = P["hand_r"]; P["ik_hand_l"] = P["hand_l"]; P["ik_hand_r"] = P["hand_r"]
P["props_root"] = V((0, 0, 0)); P["prop_01"] = V((0, 0, 0))
P["poi"] = V((0, UP["poi"].y, P["head"].z))
log("hauteurs (cm) : pelvis %.1f (UEFN %.1f), tête %.1f (UEFN %.1f), main G z %.1f" % (P["pelvis"].z, UP["pelvis"].z, P["head"].z, UP["head"].z, P["hand_l"].z))
frames = {n: mathutils.Matrix.Translation(P[n]) @ UF[n].to_3x3().to_4x4() for n in order}
new = calib.build.__wrapped__(U, frames) if hasattr(calib.build, "__wrapped__") else None
# calib.build efface toute la scène : on construit l'armature à la main ici
ad = bpy.data.armatures.new("Armature"); ao = bpy.data.objects.new("Armature", ad); sc.collection.objects.link(ao)
sc.unit_settings.system = 'METRIC'; sc.unit_settings.scale_length = 0.01
bpy.context.view_layer.objects.active = ao; bpy.ops.object.mode_set(mode='EDIT')
eb = {}
for b in U:
    e = ad.edit_bones.new(b["name"]); e.head = (0, 0, 0); e.tail = (0, 3, 0); e.roll = 0
    e.matrix = frames[b["name"]]
    if b["parent"] != "None": e.parent = eb[b["parent"]]
    eb[b["name"]] = e
bpy.ops.object.mode_set(mode='OBJECT')

# ---------------------------------------------------------------- 3. poids
MAP = {"CTRL_pelvis": "pelvis", "CTRL_chest": "spine_05", "CTRL_neck": "neck_01", "CTRL_head": "head",
       "clavicle.L": "clavicle_l", "upper_arm.L": "upperarm_l", "forearm.L": "lowerarm_l", "hand.L": "hand_l",
       "clavicle.R": "clavicle_r", "upper_arm.R": "upperarm_r", "canon.R": "lowerarm_r",
       "leg_up.L": "thigh_l", "leg_down.L": "calf_l", "foot.L": "foot_l", "toes.L": "ball_l",
       "leg_up.R": "thigh_r", "leg_down.R": "calf_r", "foot.R": "foot_r", "toes.R": "ball_r"}
old = arm.data.bones
def target(n):
    if n in MAP: return MAP[n]
    if n.startswith(("thumb", "index", "middle", "ring", "pinky", "f_")) and n.endswith(".L"): return "hand_l"
    b = old.get(n)
    while b is not None and b.name not in MAP and b.name != "CTRL_spine": b = b.parent
    return MAP.get(b.name) if b is not None and b.name != "CTRL_spine" else ("SPINE" if b is not None else None)
SPN = ["spine_01", "spine_02", "spine_03", "spine_04"]
def spine_split(z):
    zs = [P[n].z for n in SPN]
    if z <= zs[0]: return {SPN[0]: 1.0}
    if z >= zs[-1]: return {SPN[-1]: 1.0}
    for i in range(len(zs) - 1):
        if zs[i] <= z <= zs[i + 1]:
            t = (z - zs[i]) / (zs[i + 1] - zs[i]); return {SPN[i]: 1 - t, SPN[i + 1]: t}
stats = collections.Counter(); lost = collections.Counter()
for o in MESH:
    nm = {g.index: g.name for g in o.vertex_groups}
    neww = []
    for v in o.data.vertices:
        w = collections.defaultdict(float)
        for g in v.groups:
            if g.weight <= 0: continue
            n = nm[g.group]
            if old.get(n) is None or not old[n].use_deform: continue
            t = target(n)
            if t is None: lost[n] += 1; continue
            if t == "SPINE":
                for k, x in spine_split(v.co.z).items(): w[k] += g.weight * x
            else:
                w[t] += g.weight
            if n not in MAP: stats["%s->%s" % (n, t)] += 1
        tot = sum(w.values())
        neww.append({k: x / tot for k, x in w.items()} if tot > 0 else {"pelvis": 1.0})
        if tot <= 0: lost["(aucun poids)"] += 1
    if o.name == "Hair":
        # mèche longue jusqu'aux genoux : la lier toute à la tête la fait balancer dès que la tête bouge.
        # Haut -> head, omoplates -> spine_05, sous la taille -> pelvis, transitions douces.
        def sm(t):
            t = min(max(t, 0.0), 1.0); return t * t * (3 - 2 * t)
        Zn, Z3, Zp = P["neck_01"].z, P["spine_03"].z, P["pelvis"].z
        neww = []
        for v in o.data.vertices:
            z = v.co.z
            wh = sm((z - (Zn - 8)) / 16)
            wp = sm((Z3 - z) / max(Z3 - (Zp - 15), 1))
            ws = max(0.0, 1 - wh - wp)
            neww.append({k: x for k, x in (("head", wh), ("spine_05", ws), ("pelvis", wp)) if x > 1e-4})
    for g in list(o.vertex_groups): o.vertex_groups.remove(g)
    groups = {}
    for i, w in enumerate(neww):
        for k, x in w.items():
            if k not in groups: groups[k] = o.vertex_groups.new(name=k)
            groups[k].add([i], x, 'REPLACE')
    for m in list(o.modifiers):
        if m.type == 'ARMATURE': o.modifiers.remove(m)
    o.parent = ao
    o.modifiers.new("Armature", 'ARMATURE').object = ao
    log("maillage %s : %d sommets, os %s" % (o.name, len(o.data.vertices), sorted(groups)))
log("os secondaires renvoyés :", dict(stats.most_common(40)))
log("perdus :", dict(lost))

# ---------------------------------------------------------------- 3b. animations de Jira transférées sur le squelette UEFN
# Exact par os : M_neuf(pose) = M_jira(pose) . M_jira(pose mannequin)^-1 . M_neuf(repos)  (cm). Os sans équivalent : suivent
# leur parent. Locomotion exclue (celle d'Epic la remplace).
OLDOF = {"pelvis": "CTRL_pelvis", "spine_01": "CTRL_spine", "spine_02": "CTRL_spine", "spine_03": "CTRL_spine", "spine_04": "CTRL_spine",
         "spine_05": "CTRL_chest", "neck_01": "CTRL_neck", "neck_02": "CTRL_neck", "head": "CTRL_head",
         "clavicle_l": "clavicle.L", "upperarm_l": "upper_arm.L", "lowerarm_l": "forearm.L", "hand_l": "hand.L",
         "clavicle_r": "clavicle.R", "upperarm_r": "upper_arm.R", "lowerarm_r": "canon.R", "hand_r": "canon.R"}
for s_, u_ in (("L", "l"), ("R", "r")):
    OLDOF.update({"thigh_" + u_: "leg_up." + s_, "calf_" + u_: "leg_down." + s_, "foot_" + u_: "foot." + s_, "ball_" + u_: "toes." + s_})
def cm(M):
    M = M.copy(); M.translation = M.translation * 100; return M
NREST = {b.name: b.matrix_local.copy() for b in ao.data.bones}
CONFI = {n: cm(M).inverted() for n, M in CONF.items()}
BAKE = [a.name for a in bpy.data.actions if a.name.startswith("Jira_") and a.name not in ("Jira_run", "Jira_walk", "Jira_idle", "Jira_run_origine")]
if not arm.animation_data: arm.animation_data_create()
if not ao.animation_data: ao.animation_data_create()
for pb in ao.pose.bones: pb.rotation_mode = 'QUATERNION'
for an in BAKE:
    src = bpy.data.actions[an]
    for pb in arm.pose.bones: pb.matrix_basis = mathutils.Matrix()
    arm.animation_data.action = src
    if hasattr(arm.animation_data, "action_slot") and src.slots: arm.animation_data.action_slot = src.slots[0]
    dst = bpy.data.actions.new(an + "_UEFN"); ao.animation_data.action = dst
    f0, f1 = int(src.frame_range[0]), int(src.frame_range[1])
    for fr in range(f0, f1 + 1):
        sc.frame_set(fr)
        OLDM = {pb.name: pb.matrix.copy() for pb in arm.pose.bones}
        for b in U:
            pb = ao.pose.bones[b["name"]]
            o_ = OLDOF.get(b["name"])
            if o_:
                pb.matrix = cm(OLDM[o_]) @ CONFI[o_] @ NREST[b["name"]]
            else:
                pb.matrix_basis = mathutils.Matrix()
            bpy.context.view_layer.update()
        for pb in ao.pose.bones:
            pb.keyframe_insert("rotation_quaternion", frame=fr); pb.keyframe_insert("location", frame=fr)
    log("animation %s : images %d-%d" % (dst.name, f0, f1))
ao.animation_data.action = None
for pb in ao.pose.bones: pb.matrix_basis = mathutils.Matrix()
bpy.data.objects.remove(arm)

# ---------------------------------------------------------------- rendu de contrôle et export
for o in [o for o in bpy.data.objects if o.type in ('CAMERA', 'LIGHT')]: bpy.data.objects.remove(o)
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
cam.data.lens = 50; cam.data.clip_end = 10000
ao.show_in_front = True; ad.display_type = 'STICK'
for yaw in (0, 90, 180):
    r = math.radians(yaw); d = 330
    cam.location = (d * math.sin(r), -d * math.cos(r), 100); cam.rotation_euler = (math.radians(90), 0, r)
    sc.render.filepath = SHOT + "uefn_repos_%d.png" % yaw; bpy.ops.render.render(write_still=True)

act_t = bpy.data.actions["Jira_shoot_front_UEFN"]; ao.animation_data.action = act_t
for fr_, yaw in ((10, 0), (10, -60), (10, 180)):
    sc.frame_set(fr_); r = math.radians(yaw); d = 330
    cam.location = (d * math.sin(r), -d * math.cos(r), 120); cam.rotation_euler = (math.radians(90), 0, r)
    sc.render.filepath = SHOT + "uefn_tir_%d.png" % yaw; bpy.ops.render.render(write_still=True)
ao.animation_data.action = None
for pb in ao.pose.bones: pb.matrix_basis = mathutils.Matrix()
# pose test : bras et jambe pliés avec les orientations UEFN (doit déformer proprement)
for n, ax, a in (("calf_l", (0, 0, 1), -60), ("lowerarm_l", (0, 0, 1), 70), ("spine_03", (0, 0, 1), 20), ("head", (0, 0, 1), 25)):
    ao.pose.bones[n].rotation_mode = 'XYZ'
    ao.pose.bones[n].rotation_euler = mathutils.Euler([math.radians(a) * c for c in ax])
for yaw in (0, 90):
    r = math.radians(yaw); d = 330
    cam.location = (d * math.sin(r), -d * math.cos(r), 100); cam.rotation_euler = (math.radians(90), 0, r)
    sc.render.filepath = SHOT + "uefn_plie_%d.png" % yaw; bpy.ops.render.render(write_still=True)
for pb in ao.pose.bones: pb.matrix_basis = mathutils.Matrix(); pb.rotation_mode = 'QUATERNION'
for o in (sun, cam): bpy.data.objects.remove(o)
bpy.ops.wm.save_as_mainfile(filepath=J + "Jira_UEFN.blend")
calib.export(J + "Jira_UEFN.fbx", [ao] + MESH)
log("export ok")

import os
os.makedirs(J + "Anims_UEFN", exist_ok=True)
for an in BAKE:
    ac = bpy.data.actions[an + "_UEFN"]; ao.animation_data.action = ac
    sc.frame_start, sc.frame_end = int(ac.frame_range[0]), int(ac.frame_range[1])
    bpy.ops.object.select_all(action='DESELECT'); ao.select_set(True); bpy.context.view_layer.objects.active = ao
    bpy.ops.export_scene.fbx(filepath=J + "Anims_UEFN/" + an + ".fbx", use_selection=True, object_types={'ARMATURE'}, add_leaf_bones=False,
                             bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True,
                             bake_anim_simplify_factor=0.0, use_armature_deform_only=False, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_NONE', global_scale=1.0)
ao.animation_data.action = None
log("animations exportées : %d" % len(BAKE))
open(J + "jira_uefn_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
