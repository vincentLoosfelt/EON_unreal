# EON — lit le projet Unity (lecture seule) et écrit Saved/Scripts/eon_unity_materials.json :
#   fbx  -> matériau Unity par emplacement (d'après les prefabs et scènes)
#   .mat -> textures (couleur, normale, métal, émission) et couleurs, parent compris.
# Python 3 hors Unreal :  py -I eon_unity_materials_scan.py
import os, re, json, collections

U = "D:/_PROJETS/briscard/GIT/EON_Project/Assets"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "eon_unity_materials.json")

# --- guid -> chemin relatif
guid = {}
for root, _, files in os.walk(U):
    for f in files:
        if f.endswith(".meta"):
            p = os.path.join(root, f)
            with open(p, encoding="utf-8", errors="ignore") as h:
                m = re.search(r"^guid: ([0-9a-f]{32})", h.read(), re.M)
            if m:
                guid[m.group(1)] = os.path.relpath(p[:-5], U).replace("\\", "/")

ROLES = {
    "base": ["_BaseMap", "_MainTex", "_BC", "Base_Map", "_Texture2D", "_BaseColorMap", "_Albedo"],
    "normal": ["_BumpMap", "_Normal", "_NormalMap"],
    "metal": ["_MetallicGlossMap", "_MaskMap"],
    "emis": ["_EmissionMap", "_Emission"],
    "ao": ["_OcclusionMap"],
}


def parse_mat(rel, depth=0):
    path = os.path.join(U, rel)
    with open(path, encoding="utf-8", errors="ignore") as h:
        txt = h.read()
    tex = dict(re.findall(r"- (\w+):\s*\n\s*m_Texture: \{fileID: \d+, guid: ([0-9a-f]{32})", txt))
    cols = {k: [float(x) for x in v] for k, *v in re.findall(
        r"- (\w+): \{r: ([-\d.e]+), g: ([-\d.e]+), b: ([-\d.e]+), a: ([-\d.e]+)\}", txt)}
    floats = {k: float(v) for k, v in re.findall(r"- (\w+): ([-\d.e]+)\s*$", txt, re.M)}
    out = {}
    par = re.search(r"m_Parent: \{fileID: \d+, guid: ([0-9a-f]{32})", txt)
    if par and par.group(1) in guid and depth < 5 and guid[par.group(1)].endswith(".mat"):
        out = parse_mat(guid[par.group(1)], depth + 1)
        out["parent"] = guid[par.group(1)]
    for role, names in ROLES.items():
        for n in names:
            if n in tex and tex[n] in guid:
                out[role] = guid[tex[n]]
                break
    for n in ("_BaseColor", "_Color", "_Tint"):
        if n in cols:
            out["color"] = cols[n]; break
    if "_EmissionColor" in cols:
        out["emis_color"] = cols["_EmissionColor"]
    for n in ("_Metallic", "_Smoothness", "_Glossiness"):
        if n in floats:
            out[n.strip("_").lower()] = floats[n]
    sh = re.search(r"m_Shader: \{fileID: (-?\d+), guid: ([0-9a-f]{32})", txt)
    if sh:
        out["shader"] = guid.get(sh.group(2), sh.group(2))
    if "_Surface" in floats:
        out["transparent"] = floats["_Surface"] > 0.5
    return out


mats = {}
for g, rel in guid.items():
    if rel.endswith(".mat"):
        try:
            mats[rel] = parse_mat(rel)
        except Exception as e:
            mats[rel] = {"error": str(e)}

# --- fbx -> matériaux, depuis prefabs et scènes
votes = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
DOC = re.compile(r"^--- !u!(\d+) &(-?\d+)", re.M)
for root, _, files in os.walk(U):
    if "_Recovery" in root:
        continue
    for f in files:
        if not (f.endswith(".prefab") or f.endswith(".unity")):
            continue
        with open(os.path.join(root, f), encoding="utf-8", errors="ignore") as h:
            txt = h.read()
        parts = DOC.split(txt)
        docs = {}
        for i in range(1, len(parts) - 2, 3):
            docs[parts[i + 1]] = (parts[i], parts[i + 2])
        go_mesh, go_mats = {}, {}
        for fid, (cls, body) in docs.items():
            gom = re.search(r"m_GameObject: \{fileID: (-?\d+)", body)
            if cls in ("33", "137"):
                mm = re.search(r"m_Mesh: \{fileID: (-?\d+), guid: ([0-9a-f]{32})", body)
                if mm and gom and mm.group(2) in guid:
                    go_mesh[gom.group(1)] = guid[mm.group(2)]
            if cls in ("23", "137"):
                ml = re.search(r"m_Materials:\s*\n((?:\s*- \{[^}]*\}\s*\n)+)", body)
                if ml and gom:
                    go_mats[gom.group(1)] = re.findall(r"guid: ([0-9a-f]{32})", ml.group(1))
            if cls == "1001":  # PrefabInstance : surcharges de matériaux d'un FBX glissé dans la scène
                src = re.search(r"m_SourcePrefab: \{fileID: \d+, guid: ([0-9a-f]{32})", body)
                if src and guid.get(src.group(1), "").lower().endswith(".fbx"):
                    for idx, mg in re.findall(
                            r"propertyPath: m_Materials\.Array\.data\[(\d+)\]\s*\n\s*value:.*\n\s*objectReference: \{fileID: \d+, guid: ([0-9a-f]{32})", body):
                        if mg in guid:
                            votes[guid[src.group(1)]][int(idx)][guid[mg]] += 1
        for go, mesh in go_mesh.items():
            if mesh.lower().endswith(".fbx"):
                for i, mg in enumerate(go_mats.get(go, [])):
                    if mg in guid:
                        votes[mesh][i][guid[mg]] += 1

fbx = {}
for mesh, slots in votes.items():
    fbx[mesh] = {str(i): c.most_common(1)[0][0] for i, c in sorted(slots.items())}

used = {m for s in fbx.values() for m in s.values()}
json.dump({"fbx": fbx, "mats": {k: v for k, v in mats.items() if k in used or k.startswith("materials/")}},
          open(OUT, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
all_fbx = [r for r in guid.values() if r.lower().endswith(".fbx")]
print("fbx avec matériaux trouvés : %d / %d" % (len(fbx), len(all_fbx)))
print("sans :", sorted(set(all_fbx) - set(fbx)))
print("matériaux utilisés :", len(used), " avec texture couleur :", sum(1 for m in used if "base" in mats.get(m, {})))
