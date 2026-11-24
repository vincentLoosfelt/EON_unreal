# EON — lecture seule : lumières de L_Etage et du HUB (type, intensité, unités, couleur) et exposition des volumes.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_etage_lumieres.py
import unreal
LOG = []
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for carte in ("/Game/EON/Maps/L_Etage", "/Game/EON/Maps/L_HUB"):
    les.load_level(carte)
    LOG.append("=== " + carte)
    for a in eas.get_all_level_actors():
        for c in a.get_components_by_class(unreal.LightComponent):
            u = c.get_editor_property("intensity_units") if hasattr(c, "intensity_units") or isinstance(c, unreal.LocalLightComponent) else ""
            LOG.append("%s %s %s int=%.2f %s coul=%s visible=%s" % (a.get_actor_label(), c.get_class().get_name(), c.get_name(), c.intensity, u,
                       c.get_editor_property("light_color"), c.is_visible()))
        if isinstance(a, unreal.SkyLight):
            LOG.append("SkyLight %s int=%.2f" % (a.get_actor_label(), a.light_component.intensity))
    for v in [x for x in eas.get_all_level_actors() if isinstance(x, unreal.PostProcessVolume)]:
        s = v.settings
        LOG.append("PPV %s expo methode=%s biais=%.3f (override %s) min=%.2f max=%.2f" % (v.get_actor_label(), s.auto_exposure_method, s.auto_exposure_bias,
                   s.override_auto_exposure_bias, s.auto_exposure_min_brightness, s.auto_exposure_max_brightness))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_etage_lumieres.txt", "w", encoding="utf-8").write("\n".join(LOG))
