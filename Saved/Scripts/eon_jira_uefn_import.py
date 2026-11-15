# EON — import de Jira riggée sur le squelette UEFN (Saved/Jira/Jira_UEFN.fbx, blender_jira_uefn.py) :
# SK_Jira_UEFN sur SK_UEFN_Mannequin ; matériaux de SK_Jira par slot ; contrôle de la pose de référence
# (orientations identiques au mannequin) ; modes de retarget de translation lus. Journal : Saved/Logs/eon_jira_uefn.txt
import unreal, json, math
EAL = unreal.EditorAssetLibrary
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DIR = "/Game/EON/Characters/Jira"
LOG = []
def log(s):
    LOG.append(str(s)); open(PROJ + "Saved/Logs/eon_jira_uefn.txt", "w", encoding="utf-8").write("\n".join(LOG))
uefn = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
skel = uefn.get_editor_property("skeleton")
old = unreal.load_asset(DIR + "/SK_Jira")
avant = {str(m.get_editor_property("material_slot_name")): m.get_editor_property("material_interface") for m in old.get_editor_property("materials")}
o = unreal.FbxImportUI()
o.automated_import_should_detect_type = False
o.import_mesh = True; o.import_as_skeletal = True
o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
o.skeleton = skel
o.import_animations = False; o.import_materials = False; o.import_textures = False
o.create_physics_asset = False
o.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
t = unreal.AssetImportTask()
t.filename = PROJ + "Saved/Jira/Jira_UEFN.fbx"; t.destination_path = DIR; t.destination_name = "SK_Jira_UEFN"
t.automated = True; t.replace_existing = True; t.save = False; t.options = o
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
log("importés : %s" % list(t.imported_object_paths))
mesh = unreal.load_asset(DIR + "/SK_Jira_UEFN")
log("squelette : %s" % mesh.get_editor_property("skeleton").get_path_name())
mats = list(mesh.get_editor_property("materials"))
for m in mats:
    n = str(m.get_editor_property("material_slot_name"))
    if n in avant and avant[n]: m.set_editor_property("material_interface", avant[n])
mesh.set_editor_property("materials", mats)
log("matériaux : %s" % [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name() if m.get_editor_property("material_interface") else None) for m in mesh.get_editor_property("materials")])
# physique du mannequin (capsules) pour les collisions de mesh éventuelles
pa = uefn.get_editor_property("physics_asset")
if pa: mesh.set_editor_property("physics_asset", pa)
EAL.save_loaded_asset(mesh, False)
# contrôle : orientations de la pose de référence = celles du mannequin
ref = {b["name"]: b for b in json.load(open(PROJ + "Saved/Jira/uefn_skel.json"))["bones"]}
m = unreal.SkeletonModifier(); m.set_skeletal_mesh(mesh)
worst, n_b = 0, 0
for n in m.get_all_bone_names():
    n = str(n); q = m.get_bone_transform(n, True).rotation; r = ref[n]["q"]
    dot = abs(q.x * r[0] + q.y * r[1] + q.z * r[2] + q.w * r[3])
    worst = max(worst, math.degrees(2 * math.acos(min(1, dot)))); n_b += 1
log("%d os ; écart d'orientation max avec le mannequin : %.3f°" % (n_b, worst))
for n in ("root", "pelvis", "head", "hand_l", "foot_r"):
    p = m.get_bone_transform(n, True).translation; log("  %s : (%.1f %.1f %.1f) cm" % (n, p.x, p.y, p.z))
log("API skeleton : %s" % [x for x in dir(skel) if "retarget" in x.lower() or "translation" in x.lower()])
log("fin")
