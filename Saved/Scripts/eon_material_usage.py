# EON — usages du maître M_UnityLit (Skeletal Mesh : Jira, ennemis). Ne touche pas au graphe.
import unreal
m = unreal.load_asset("/Game/EON/Unity/Materials/M_UnityLit")
m.set_editor_property("used_with_skeletal_mesh", True)
unreal.MaterialEditingLibrary.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_material_usage.txt", "w").write("skeletal %s" % m.get_editor_property("used_with_skeletal_mesh"))
