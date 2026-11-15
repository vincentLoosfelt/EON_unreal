# EON — remplace Jira_walk et Jira_run par les versions adoucies (blender_jira_anims.py : course moins ample,
# cadence recalée sur 2,8 et 4,8 m/s). Les blendspaces et l'ABP de locomotion gardent leurs références.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_jira_anims_import.py
import unreal
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
SRC = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/Anims/"
DIR = "/Game/EON/Characters/Jira/Anims/Jira"
skel = unreal.load_asset("/Game/EON/Characters/Jira/SKEL_Jira")
LOG = []
for name in ("Jira_walk", "Jira_run"):
    o = unreal.FbxImportUI()
    o.automated_import_should_detect_type = False
    o.import_mesh = False
    o.import_animations = True
    o.import_materials = False
    o.import_textures = False
    o.import_as_skeletal = True
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    o.skeleton = skel
    o.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
    o.anim_sequence_import_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    # Import par-dessus l'asset existant (même objet) : les blendspaces et l'ABP gardent leurs références.
    t = unreal.AssetImportTask()
    t.filename = SRC + name + ".fbx"; t.destination_path = DIR; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True; t.options = o
    AT.import_asset_tasks([t])
    a = unreal.load_asset(DIR + "/" + name)
    if not isinstance(a, unreal.AnimSequence):
        LOG.append("%s : import ÉCHOUÉ (%s)" % (name, list(t.imported_object_paths))); continue
    EAL.save_loaded_asset(a)
    LOG.append("%s : %.2f s (%s)" % (name, a.get_play_length(), list(t.imported_object_paths)))
EAL.save_directory("/Game/EON/Characters/Jira", False, True)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_jira_anims_import.txt", "w", encoding="utf-8").write("\n".join(LOG))
