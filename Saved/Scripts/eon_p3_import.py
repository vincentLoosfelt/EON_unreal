# EON — réimport de la seule planche P3 (traînées de sang au sol, 2 × 4 images de 1024 × 512), sans toucher au matériau.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os
EAL = unreal.EditorAssetLibrary
SRC = os.path.join(unreal.Paths.project_dir(), "SourceAssets", "VFX_Planches", "EON_VFX_P3_TachesSol.png")
DST = "/Game/EON/VFX/Planches"
t = unreal.AssetImportTask()
t.set_editor_property("filename", SRC)
t.set_editor_property("destination_path", DST)
t.set_editor_property("destination_name", "T_EON_VFX_P3_TachesSol")
t.set_editor_property("automated", True)
t.set_editor_property("replace_existing", True)
t.set_editor_property("save", False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
tx = unreal.load_asset(DST + "/T_EON_VFX_P3_TachesSol")
tx.set_editor_property("srgb", True)
tx.set_editor_property("never_stream", True)
tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
unreal.log("EON_P3 %s %dx%d" % (EAL.save_loaded_asset(tx), tx.blueprint_get_size_x(), tx.blueprint_get_size_y()))
