# EON — pose AEONChampForce au centre de la sphère du dôme du C.O.M (mesuré le 15/11 : (-240, 50, 97), rayon 1046 cm).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_champ_acteur.py
import unreal
unreal.EditorLoadingAndSavingUtils.load_map("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(a, unreal.EONChampForce):
        eas.destroy_actor(a)
c = eas.spawn_actor_from_class(unreal.EONChampForce, unreal.Vector(-240.0, 50.0, 97.0), unreal.Rotator(0, 0, 0))
c.set_editor_property("radius", 1046.0)
c.set_actor_label("ChampForce_COM")
c.set_folder_path("HUB")
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), "/Game/EON/Maps/L_HUB")
unreal.log("EON_CHAMPA posé, collection %s" % c.get_editor_property("collection"))
