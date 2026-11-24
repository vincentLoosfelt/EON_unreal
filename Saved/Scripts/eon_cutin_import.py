# EON — JIR-V23 (24/11) : importe Saved/Shots/CutIn/T_Jira_CutIn.png en texture d'interface /Game/EON/UI/T_Jira_CutIn.
import unreal, os
t = unreal.AssetImportTask()
t.filename = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/CutIn/T_Jira_CutIn.png"
t.destination_path = "/Game/EON/UI"
t.destination_name = "T_Jira_CutIn"
t.replace_existing = True
t.automated = True
t.save = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
tex = unreal.load_asset("/Game/EON/UI/T_Jira_CutIn")
tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
tex.set_editor_property("never_stream", True)
ok = unreal.EditorAssetLibrary.save_loaded_asset(tex)
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_cutin_import.txt"), "w").write("import %s, taille %dx%d, sauve %s" % (tex is not None, tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), ok))
