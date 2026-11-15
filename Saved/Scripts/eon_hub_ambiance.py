# EON — L_HUB, sur place : ambiance sombre et contrastée des références Unity (15/11, Vincent : « assombrir, on est loin
# du vrai »), et point de départ de Jira sur le C.O.M à mi-rayon du centre (le centre = ordinateurs, pas de passage).
# Unity : ambiance dégradée sombre (ciel 0,21/0,23/0,26, horizon 0,11, sol 0,05), lumière principale 0,6, lightmaps.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_ambiance.py
import unreal
MAP = "/Game/EON/Maps/L_HUB"
CIEL = 0.2            # SkyLight (était 1)
SOLEILS = {"Soleil_principal": 0.6, "Soleil_2": 0.2}   # lux (étaient 1,88 et 1,76)
DEPART = unreal.Vector(330.0, 53.0, 700.0)   # sur le C.O.M, hors de la zone des ordinateurs (r 5,7 m du centre (-240, 53)), à droite de l écran (Vincent, 15/11)
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
start = None
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(a, unreal.SkyLight):
        a.light_component.set_editor_property("intensity", CIEL)
    elif isinstance(a, unreal.DirectionalLight) and a.get_actor_label() in SOLEILS:
        a.light_component.set_editor_property("intensity", SOLEILS[a.get_actor_label()])
    elif isinstance(a, unreal.PlayerStart):
        start = a
if start is None:
    start = eas.spawn_actor_from_class(unreal.PlayerStart, DEPART, unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0))
    start.set_actor_label("Depart_Jira")
start.set_actor_location(DEPART, False, False)
start.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), False)
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), MAP)
unreal.log("EON_AMB ciel %.2f soleils %s départ %s" % (CIEL, SOLEILS, DEPART))
