# EON — importe l'icosphère dense de la fumée toon (SourceAssets/VFX/SM_EON_FumeeSphere.obj, rayon 50 cm).
import unreal, os
t = unreal.AssetImportTask()
t.set_editor_property("filename", os.path.join(unreal.Paths.project_dir(), "SourceAssets/VFX/SM_EON_FumeeSphere.obj"))
t.set_editor_property("destination_path", "/Game/EON/VFX")
t.set_editor_property("automated", True); t.set_editor_property("replace_existing", True); t.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
m = unreal.load_asset("/Game/EON/VFX/SM_EON_FumeeSphere")
# Pas de Nanite (Vincent, 24/11 : « toujours trop sphérique et sans displacement ») : en Nanite, Niagara et le dôme
# affichaient le maillage de repli, trop pauvre en sommets pour que le displacement des bulles (WPO) se voie.
ns = m.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", False); m.set_editor_property("nanite_settings", ns)
unreal.EditorAssetLibrary.save_loaded_asset(m)
open(os.path.join(unreal.Paths.project_saved_dir(), "sphere_import.txt"), "w").write("%s r=%.1f" % (m, m.get_bounds().sphere_radius if m else -1))
