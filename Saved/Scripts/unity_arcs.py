# EON — extrait les 8 arcs électriques du HUB Unity (ElectricArcVFX dans Room_elevator) : extrémités (startPoint,
# endPoint, en monde Unity) et rayons de repositionnement. Sortie : hub_arcs.json (lu par eon_hub_arcs.py).
#   py -3 unity_arcs.py
import re, json, sys, os
sys.argv = [sys.argv[0], "x"]
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eon_unity_flatten as F
PREFAB = "prefab/Environment/Room/Room Special/Room_elevator.prefab"
nodes, owner = F.build(PREFAB)
memo = {}
txt = open(os.path.join(F.U, PREFAB), encoding="utf-8", errors="ignore").read()
arcs = []
for m in re.finditer(r"--- !u!1001 &(\d+)\n(.*?)(?=\n--- !u!|\Z)", txt, re.S):
    body = m.group(2)
    if "ElectricArcVFX" not in body:
        continue
    d = {"name": (re.search(r"value: (ElectricArcVFX[^\n]*)", body) or [None, "?"])[1]}
    for key in ("startPoint", "endPoint"):
        r = re.search(r"propertyPath: %s\n\s*value: ?\n\s*objectReference: \{fileID: (-?\d+)" % key, body)
        fid = int(r.group(1)) if r else 0
        k = owner.get(fid, fid)
        if k in nodes:
            p, _, _, a = F.world(nodes, k, memo)
            d[key] = p; d[key + "_name"] = nodes[k].name; d[key + "_actif"] = a
        else:
            d[key] = None; d[key + "_fid"] = fid
    for key in ("startPointRadius", "endPointRadius"):
        r = re.search(r"propertyPath: %s\n\s*value: ([-\d.e]+)" % key, body)
        d[key] = float(r.group(1)) if r else 2.0
    arcs.append(d)
json.dump(arcs, open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "hub_arcs.json"), "w", encoding="utf-8"), indent=1, ensure_ascii=False)
for a in arcs:
    print(a["name"], [round(c, 2) for c in a["startPoint"]] if a["startPoint"] else a.get("startPoint_fid"), a.get("startPoint_name"), "->",
          [round(c, 2) for c in a["endPoint"]] if a["endPoint"] else a.get("endPoint_fid"), a.get("endPoint_name"), a["startPointRadius"], a["endPointRadius"])
