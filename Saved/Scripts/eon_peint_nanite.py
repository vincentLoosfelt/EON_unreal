# EON — drapeaux d'usage de M_EON_DecorPeint : Nanite et instances (murs et décor des salles en ISM Nanite). Sans eux,
# le jeu empaqueté remplace les MI_Peint_* par le matériau par défaut (« missing usage flag Nanite »).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_peint_nanite.py
import unreal
m = unreal.load_asset("/Game/EON/Materials/Peint/M_EON_DecorPeint")
LOG = []
for u in (unreal.MaterialUsage.MATUSAGE_NANITE, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES):
    LOG.append("%s : %s" % (u, unreal.MaterialEditingLibrary.set_material_usage(m, u)))
unreal.MaterialEditingLibrary.recompile_material(m)
LOG.append("nanite=%s ism=%s sauvegarde=%s" % (m.get_editor_property("used_with_nanite"), m.get_editor_property("used_with_instanced_static_meshes"),
                                              unreal.EditorAssetLibrary.save_loaded_asset(m, False)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_peint_nanite.txt", "w", encoding="utf-8").write("\n".join(LOG))
