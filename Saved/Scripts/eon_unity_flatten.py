# EON — aplatit un prefab Unity (prefabs imbriqués + surcharges) en liste de FBX posés dans le monde.
# Lecture seule du projet Unity. Python 3 hors Unreal :
#   py -I eon_unity_flatten.py "prefab/Environment/Room/Room Special/Room_elevator.prefab" sortie.json
# Sortie : [{name, fbx, pos[x,y,z] (m, repère Unity), rot[x,y,z,w], scale[x,y,z], mats[...], active}]
import os, re, sys, json, copy, math

U = "D:/_PROJETS/briscard/GIT/EON_Project/Assets"
MASK = 0x7FFFFFFFFFFFFFFF

guid = {}
for r, _, fs in os.walk(U):
    for x in fs:
        if x.endswith(".meta"):
            with open(os.path.join(r, x), encoding="utf-8", errors="ignore") as h:
                m = re.search(r"^guid: (\w{32})", h.read(600), re.M)
            if m:
                guid[m.group(1)] = os.path.relpath(os.path.join(r, x[:-5]), U).replace("\\", "/")
rguid = {v: k for k, v in guid.items()}

DOC = re.compile(r"^--- !u!(\d+) &(-?\d+)( stripped)?\s*$", re.M)


def parse(path):
    txt = open(os.path.join(U, path), encoding="utf-8", errors="ignore").read()
    out = {}
    ms = list(DOC.finditer(txt))
    for i, m in enumerate(ms):
        body = txt[m.end(): ms[i + 1].start() if i + 1 < len(ms) else len(txt)]
        out[int(m.group(2))] = (int(m.group(1)), body, bool(m.group(3)))
    return out


def ref(body, key):
    m = re.search(key + r": \{fileID: (-?\d+)(?:, guid: (\w{32}))?", body)
    return (int(m.group(1)), m.group(2)) if m else (0, None)


def vec(body, key, n):
    m = re.search(key + r": \{([^}]*)\}", body)
    if not m:
        return None
    d = dict(re.findall(r"(\w): (-?[\d.eE+-]+)", m.group(1)))
    return [float(d.get(c, 0)) for c in "xyzw"[:n]]


sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fbx_inspect
_fbxinfo = {}


def fbx_root(rel):
    """(nb de racines, Lcl racine en repère Unity) : rot quaternion, échelle. Multi-racine -> identité."""
    if rel not in _fbxinfo:
        roots = [m for m in fbx_inspect.scan(os.path.join(U, rel)) if m["root"]]
        if len(roots) == 1:
            rx, ry, rz = [math.radians(a) for a in roots[0]["r"]]
            # Euler XYZ du FBX (repère droit) -> quaternion, puis passage en repère Unity (x, -y, -z, w)
            cx, sx, cy, sy, cz, sz = math.cos(rx/2), math.sin(rx/2), math.cos(ry/2), math.sin(ry/2), math.cos(rz/2), math.sin(rz/2)
            qx = [sx, 0, 0, cx]; qy = [0, sy, 0, cy]; qz = [0, 0, sz, cz]
            q = qmul(qz, qmul(qy, qx))
            _fbxinfo[rel] = (1, [q[0], -q[1], -q[2], q[3]], [abs(v) for v in roots[0]["s"]], roots[0]["s"])
        else:
            _fbxinfo[rel] = (len(roots), [0, 0, 0, 1], [1, 1, 1], [1, 1, 1])
        # Rotation commune des racines : Unreal fusionne un FBX à plusieurs racines en cuisant leurs Lcl
        # (le mesh fusionné est « debout »), alors qu'Unity garde un nœud modèle neutre au-dessus.
        if len(roots) > 1:
            rx, ry, rz = [math.radians(a) for a in roots[0]["r"]]
            cx, sx, cy, sy, cz, sz = math.cos(rx/2), math.sin(rx/2), math.cos(ry/2), math.sin(ry/2), math.cos(rz/2), math.sin(rz/2)
            q = qmul([0, 0, sz, cz], qmul([0, sy, 0, cy], [sx, 0, 0, cx]))
            _post[rel] = [q[0], -q[1], -q[2], q[3]]
        else:
            _post[rel] = [0, 0, 0, 1]
    return _fbxinfo[rel]


_post = {}


def fbx_post(rel):
    fbx_root(rel)
    return _post[rel]


class Node:
    def __init__(self):
        self.parent = None; self.pos = [0, 0, 0]; self.rot = [0, 0, 0, 1]; self.scale = [1, 1, 1]
        self.name = ""; self.fbx = None; self.mats = {}; self.active = True; self.go = None; self.vfx = None
        self.div = [1, 1, 1]   # échelle racine du FBX déjà appliquée au mesh Unreal
        self.direct = False    # MeshFilter posé à la main (sous-mesh d'un FBX) plutôt qu'instance du modèle


_cache = {}


def build(path):
    """Nœuds (clé = fileID de la Transform dans l'espace de cet asset) + index objet -> nœud."""
    if path in _cache:
        return copy.deepcopy(_cache[path])
    docs = parse(path)
    nodes, owner = {}, {}   # owner : fileID d'objet (GO, renderer…) -> clé de nœud
    go_of_tf = {}
    # Transforms propres
    for fid, (cls, body, stripped) in docs.items():
        if cls in (4, 224) and not stripped:
            n = Node()
            n.parent = ref(body, "m_Father")[0] or None
            n.pos = vec(body, "m_LocalPosition", 3) or n.pos
            n.rot = vec(body, "m_LocalRotation", 4) or n.rot
            n.scale = vec(body, "m_LocalScale", 3) or n.scale
            go = ref(body, "m_GameObject")[0]
            n.go = go
            nodes[fid] = n; owner[fid] = fid; owner[go] = fid; go_of_tf[go] = fid
    for fid, (cls, body, stripped) in docs.items():
        if stripped:
            continue
        if cls == 1:
            k = go_of_tf.get(fid)
            if k is not None:
                m = re.search(r"m_Name: (.*)", body); nodes[k].name = m.group(1).strip() if m else ""
                m = re.search(r"m_IsActive: (\d)", body); nodes[k].active = (m is None or m.group(1) == "1")
        elif cls in (33, 23, 137, 114):
            go = ref(body, "m_GameObject")[0]
            k = go_of_tf.get(go)
            if k is None:
                continue
            owner[fid] = k
            if cls in (33, 137):
                mf, mg = ref(body, "m_Mesh")
                if mg in guid and guid[mg].lower().endswith(".fbx"):
                    nodes[k].fbx = guid[mg]; nodes[k].direct = True
                    nr, _, rs, _ = fbx_root(guid[mg])
                    nodes[k].div = rs if nr == 1 else [1, 1, 1]
            if cls in (23, 137):
                ml = re.search(r"m_Materials:\s*\n((?:\s*- \{[^}]*\}\s*\n)+)", body)
                if ml:
                    for i, g in enumerate(re.findall(r"guid: (\w{32})", ml.group(1))):
                        nodes[k].mats[i] = guid.get(g, g)
            if cls == 114 and "arcColor" in body:
                nodes[k].vfx = "ElectricArc"
    # Instances de prefabs / modèles
    for fid, (cls, body, stripped) in docs.items():
        if cls != 1001:
            continue
        sp, sg = ref(body, "m_SourcePrefab")
        src = guid.get(sg)
        if not src:
            continue
        parent = ref(body, "m_TransformParent")[0] or None
        mods = []
        for mm in re.finditer(r"- target: \{fileID: (-?\d+), guid: (\w{32}), type: \d\}\s*\n\s*propertyPath: (.*)\n\s*value: ?(.*)\n\s*objectReference: \{fileID: (-?\d+)(?:, guid: (\w{32}))?", body):
            mods.append((int(mm.group(1)), mm.group(2), mm.group(3).strip().strip("'"), mm.group(4).strip(), mm.group(6)))
        if src.lower().endswith(".prefab"):
            sub, subowner = build(src)
            roots = [k for k, n in sub.items() if n.parent is None or n.parent not in sub]
        else:  # modèle FBX : un seul nœud racine
            sub, subowner = {}, {}
            haspos = {t for t, g, p, v, og in mods if g == sg and p.startswith("m_LocalPosition")}
            hasrot = {t for t, g, p, v, og in mods if g == sg and p.startswith("m_LocalRotation")}
            cand = [t for t, g, p, v, og in mods if t in haspos and t in hasrot]
            root = cand[0] if cand else (sorted(haspos)[0] if haspos else 1)
            nr, q0, s0, s0signed = fbx_root(src)
            n = Node(); n.fbx = src; n.rot = list(q0); n.scale = list(s0signed); n.div = s0
            sub[root] = n; subowner[root] = root
            for t, g, p, v, og in mods:
                if g == sg and p.startswith("m_Materials"):
                    subowner.setdefault(t, root)
            roots = [root]
        for t, g, p, v, og in mods:
            if g != sg:
                continue
            k = subowner.get(t)
            if k is None or k not in sub:
                continue
            n = sub[k]
            m = re.match(r"m_Local(Position|Rotation|Scale)\.([xyzw])", p)
            if m:
                arr = {"Position": n.pos, "Rotation": n.rot, "Scale": n.scale}[m.group(1)]
                arr["xyzw".index(m.group(2))] = float(v)
            elif p == "m_Name":
                n.name = v
            elif p == "m_IsActive":
                n.active = v == "1"
            else:
                mm = re.match(r"m_Materials\.Array\.data\[(\d+)\]", p)
                if mm and og:
                    n.mats[int(mm.group(1))] = guid.get(og, og)
        for k, n in sub.items():
            nk = (fid ^ k) & MASK
            if n.parent is not None and n.parent in sub:
                n.parent = (fid ^ n.parent) & MASK
            elif k in roots:
                n.parent = parent
            nodes[nk] = n
        for o, k in subowner.items():
            owner[(fid ^ o) & MASK] = (fid ^ k) & MASK
    # Objets ajoutés sous des transforms « stripped » : leur parent est déjà dans l'espace de cet asset.
    _cache[path] = (nodes, owner)
    return copy.deepcopy((nodes, owner))


def qmul(a, b):
    ax, ay, az, aw = a; bx, by, bz, bw = b
    return [aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz]


def qrot(q, v):
    x, y, z, w = q
    p = qmul(qmul(q, [v[0], v[1], v[2], 0]), [-x, -y, -z, w])
    return p[:3]


def world(nodes, k, memo):
    if k in memo:
        return memo[k]
    n = nodes[k]
    if n.parent is None or n.parent not in nodes:
        r = (n.pos, n.rot, n.scale, n.active)
    else:
        pp, pr, ps, pa = world(nodes, n.parent, memo)
        sp = [n.pos[i] * ps[i] for i in range(3)]
        rp = qrot(pr, sp)
        r = ([pp[i] + rp[i] for i in range(3)], qmul(pr, n.rot), [ps[i] * n.scale[i] for i in range(3)], pa and n.active)
    memo[k] = r
    return r


if __name__ == "__main__":
    nodes, _ = build(sys.argv[1])
    memo, out, seen = {}, [], set()
    for k, n in nodes.items():
        if not (n.fbx or n.vfx):
            continue
        key = k
        if n.fbx and n.direct and fbx_root(n.fbx)[0] > 1 and n.parent in nodes:
            # Sous-mesh d'un FBX à plusieurs racines : Unreal a fusionné les morceaux -> un seul mesh,
            # posé à la place du modèle (le parent), une fois.
            if (n.parent, n.fbx) in seen:
                continue
            seen.add((n.parent, n.fbx)); key = n.parent
        p, r, sc, a = world(nodes, key, memo)
        if key != k:
            a = a and n.active
        sc = [sc[i] / (n.div[i] or 1) for i in range(3)]
        if n.fbx:
            r = qmul(r, fbx_post(n.fbx))
        out.append(dict(name=n.name, fbx=n.fbx, vfx=n.vfx, pos=p, rot=r, scale=sc, mats=[n.mats[i] for i in sorted(n.mats)], active=a))
    json.dump(out, open(sys.argv[2], "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    import collections
    print(len(out), "objets ;", sum(1 for o in out if o["active"]), "actifs")
    for f, c in collections.Counter((o["fbx"] or o["vfx"]) for o in out).most_common():
        print(" ", c, f)
    xs = [o["pos"] for o in out if o["active"]]
    print("étendue (m) x %.1f..%.1f  y %.1f..%.1f  z %.1f..%.1f" % (min(p[0] for p in xs), max(p[0] for p in xs), min(p[1] for p in xs), max(p[1] for p in xs), min(p[2] for p in xs), max(p[2] for p in xs)))
