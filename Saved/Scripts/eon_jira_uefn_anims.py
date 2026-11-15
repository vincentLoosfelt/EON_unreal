# EON — import des animations de Jira transférées sur le squelette UEFN (Saved/Jira/Anims_UEFN/*.fbx, blender_jira_uefn.py)
# dans /Game/EON/Characters/Jira/Anims/UEFN (A_<nom>). Journal : Saved/Logs/eon_jira_uefn_anims.txt
import unreal, os
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = PROJ + "Saved/Jira/Anims_UEFN/"
DST = "/Game/EON/Characters/Jira/Anims/UEFN"
skel = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin").get_editor_property("skeleton")
L = []
tasks = []
for f in sorted(os.listdir(SRC)):
    if not f.endswith(".fbx"): continue
    o = unreal.FbxImportUI()
    o.automated_import_should_detect_type = False
    o.import_mesh = False; o.import_animations = True; o.import_as_skeletal = True
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    o.skeleton = skel; o.import_materials = False; o.import_textures = False
    ad = o.anim_sequence_import_data
    ad.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    ad.set_editor_property("import_bone_tracks", True)
    ad.set_editor_property("remove_redundant_keys", False)
    t = unreal.AssetImportTask()
    t.filename = SRC + f; t.destination_path = DST; t.destination_name = "A_" + f[:-4]
    t.automated = True; t.replace_existing = True; t.save = True; t.options = o
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    L.append("%s -> %s" % (t.destination_name, list(t.imported_object_paths)))
    for p in t.imported_object_paths:
        a = unreal.load_asset(p)
        if isinstance(a, unreal.AnimSequence):
            L.append("   durée %.2f s, squelette %s" % (a.get_play_length(), a.get_editor_property("skeleton").get_name()))
open(PROJ + "Saved/Logs/eon_jira_uefn_anims.txt", "w", encoding="utf-8").write("\n".join(L))
