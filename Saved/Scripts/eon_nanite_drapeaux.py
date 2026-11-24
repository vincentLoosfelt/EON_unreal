# EON — drapeaux d'usage Nanite + instances manquants (journal du jeu, 24/11 : « missing usage flag Nanite! Default Material
# will be used in game » sur MI_BG_Room_light, MI_BG_pipe, M_EON_Neon). Le drapeau se pose sur le matériau de base.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_nanite_drapeaux.py
import unreal
MEL, EAL = unreal.MaterialEditingLibrary, unreal.EditorAssetLibrary
LOG = []
bases = {}
for p in ("/Game/EON/Materials/BG/MI_BG_Room_light", "/Game/EON/Materials/BG/MI_BG_pipe", "/Game/EON/Materials/M_EON_Neon"):
    a = unreal.load_asset(p)
    b = a.get_base_material() if isinstance(a, unreal.MaterialInstance) else a
    LOG.append("%s -> base %s" % (p, b.get_path_name()))
    bases[b.get_path_name()] = b
for nm, m in bases.items():
    for u in (unreal.MaterialUsage.MATUSAGE_NANITE, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES):
        MEL.set_material_usage(m, u)
    MEL.recompile_material(m)
    LOG.append("%s : nanite=%s ism=%s sauvegarde=%s" % (nm, m.get_editor_property("used_with_nanite"),
               m.get_editor_property("used_with_instanced_static_meshes"), EAL.save_loaded_asset(m, False)))
for p in ("/Game/EON/Materials/BG/MI_BG_Room_light", "/Game/EON/Materials/BG/MI_BG_pipe"):
    LOG.append("resave %s %s" % (p, EAL.save_asset(p, False)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_nanite_drapeaux.txt", "w", encoding="utf-8").write("\n".join(LOG))
