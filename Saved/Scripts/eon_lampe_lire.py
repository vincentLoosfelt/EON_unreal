# EON — lecture seule : composants de BPA_Lampe_sol (tags, matériaux, paramètres d'émission) et réglages de la lampe.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_lampe_lire.py
import unreal
LOG = []
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_Etage")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
bp = unreal.EditorAssetLibrary.load_blueprint_class("/Game/EON/RoomAssets/Dynamiques/Lumieres/BPA_Lampe_sol")
a = eas.spawn_actor_from_class(bp, unreal.Vector(0, 0, -100000))
for p in ("intensite", "intensite_unreal", "emission_neon", "couleur"):
    try:
        LOG.append("%s = %s" % (p, a.get_editor_property(p)))
    except Exception as e:
        LOG.append("%s ? %s" % (p, e))
for c in a.get_components_by_class(unreal.ActorComponent):
    l = "%s %s tags=%s" % (c.get_class().get_name(), c.get_name(), [str(t) for t in c.component_tags])
    if isinstance(c, unreal.MeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            l += " | mat %s" % (m.get_path_name() if m else None)
            if isinstance(m, unreal.MaterialInstance):
                for sp in m.get_editor_property("scalar_parameter_values"):
                    l += " %s=%.2f" % (sp.parameter_info.name, sp.parameter_value)
                l += " parent=%s" % m.get_editor_property("parent").get_path_name()
    if isinstance(c, unreal.LightComponent):
        l += " int=%.1f rayon=%.1f" % (c.intensity, c.attenuation_radius)
    LOG.append(l)
eas.destroy_actor(a)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_lampe_lire.txt", "w", encoding="utf-8").write("\n".join(LOG))
