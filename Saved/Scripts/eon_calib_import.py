# EON — calibration : importe un FBX (argument 1) comme maillage squelettique neuf dans /Game/EON/Temp et compare son
# squelette à uefn_skel.json (écarts de position en cm, d'angle en degrés). Résultat : Saved/Logs/calib.txt
import unreal, json, sys, math
fbx = sys.argv[-1] if sys.argv[-1].endswith(".fbx") else "D:/_PROJETS/briscard/EON_UE/Saved/Jira/uefn_calib.fbx"
name = fbx.split("/")[-1][:-4]
t = unreal.AssetImportTask()
t.filename = fbx; t.destination_path = "/Game/EON/Temp"; t.destination_name = name
t.automated = True; t.replace_existing = True; t.save = True
ui = unreal.FbxImportUI()
ui.import_mesh = True; ui.import_as_skeletal = True; ui.import_animations = False; ui.import_materials = False; ui.import_textures = False
ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
t.options = ui
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
sk = unreal.load_asset("/Game/EON/Temp/" + name)
ref = {b["name"]: b for b in json.load(open("D:/_PROJETS/briscard/EON_UE/Saved/Jira/uefn_skel.json"))["bones"]}
m = unreal.SkeletonModifier(); m.set_skeletal_mesh(sk)
L = []; worst = (0, 0)
for n in m.get_all_bone_names():
    n = str(n); tr = m.get_bone_transform(n, True); q = tr.rotation; p = tr.translation
    r = ref.get(n)
    if not r: L.append("EN TROP %s" % n); continue
    dp = math.dist((p.x, p.y, p.z), r["t"])
    dot = abs(q.x * r["q"][0] + q.y * r["q"][1] + q.z * r["q"][2] + q.w * r["q"][3])
    ang = math.degrees(2 * math.acos(min(1, dot)))
    worst = (max(worst[0], dp), max(worst[1], ang))
    L.append("%s parent=%s dp=%.3f ang=%.2f  q=(%.3f %.3f %.3f %.3f) ref=(%.3f %.3f %.3f %.3f) s=%s" % (
        n, m.get_parent_name(n), dp, ang, q.x, q.y, q.z, q.w, *r["q"], tr.scale3d))
L.insert(0, "PIRE dp=%.3f cm ang=%.2f deg ; %d os" % (worst[0], worst[1], len(L)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/calib.txt", "w").write("\n".join(L))
