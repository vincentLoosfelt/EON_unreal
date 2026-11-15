# EON — L_HUB, sur place : les 4 portes statiques (mesh « porte ») remplacées par des AEONSlidingDoor (porte Sas :
# cadre fixe + 2 vantaux qui glissent de 1 m, ouverte sans ennemis), même position, rotation et échelle.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_portes_sas.py
import unreal
MAP = "/Game/EON/Maps/L_HUB"
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    sm = a.static_mesh_component.static_mesh
    if sm is None or sm.get_name() != "porte":
        continue
    d = eas.spawn_actor_from_class(unreal.EONSlidingDoor, a.get_actor_location(), a.get_actor_rotation())
    d.set_actor_scale3d(a.get_actor_scale3d())
    d.set_actor_label("PorteSas_%d" % n)
    d.set_folder_path(a.get_folder_path())
    eas.destroy_actor(a)
    n += 1
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), MAP)
unreal.log("EON_SAS %d portes remplacées" % n)
