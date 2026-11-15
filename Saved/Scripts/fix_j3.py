# EON — correctifs J3 : drapeau ISM sur M_EON_BG (salles grises sinon), priorité de la lumière principale, ciel de L_Etage.
import unreal
LOG = []
EAL = unreal.EditorAssetLibrary
m = unreal.load_asset("/Game/EON/Materials/M_EON_BG")
for u in ("used_with_instanced_static_meshes", "used_with_skeletal_mesh"):
    m.set_editor_property(u, True)
unreal.MaterialEditingLibrary.recompile_material(m)
EAL.save_loaded_asset(m)
LOG.append("M_EON_BG : ISM ok")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for MAP in ("/Game/EON/Maps/L_Etage", "/Game/EON/Maps/L_HUB"):
    les.load_level(MAP)
    for a in eas.get_all_level_actors():
        lab = a.get_actor_label()
        if isinstance(a, unreal.DirectionalLight):
            lc = a.light_component
            try:
                lc.set_editor_property("forward_shading_priority", 1 if lab == "Soleil_principal" else 0)
                LOG.append("%s %s priorité ok" % (MAP, lab))
            except Exception as e:
                LOG.append("%s %s : %s" % (MAP, lab, e))
        if isinstance(a, unreal.SkyLight) and MAP.endswith("L_Etage"):
            eas.destroy_actor(a)
            LOG.append("SkyLight retirée de L_Etage")
    LOG.append("%s enregistrée : %s" % (MAP, les.save_current_level()))
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/fix_j3.txt", "w", encoding="utf-8").write("\n".join(LOG))
