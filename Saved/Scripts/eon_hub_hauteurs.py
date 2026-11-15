# EON — L_HUB, sur place : les 2 rangées basses de murs (échelle Unity (1, 2, 1)) doublées en HAUTEUR, pas en épaisseur.
# 15/11, Vincent : « entre le premier tour de mur et juste en dessous, un énorme trou ». Rangées à 4,64 / -1,46 / -7,36 m
# (6,1 m d'écart) pour des murs de 3,05 m : doublés en épaisseur, il restait 3 m de vide entre chaque rangée ; doublés
# en hauteur (6,1 m), les rangées se touchent exactement.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_hauteurs.py
import unreal
MAP = "/Game/EON/Maps/L_HUB"
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
n = 0
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    sm = a.static_mesh_component.static_mesh
    s = a.get_actor_scale3d()
    if sm and sm.get_name().startswith("WallA") and abs(abs(s.y) - 2.0) < 0.01 and abs(abs(s.z) - 1.0) < 0.01:
        a.set_actor_scale3d(unreal.Vector(s.x, s.y / 2.0, s.z * 2.0))
        n += 1
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), MAP)
unreal.log("EON_HAUT %d murs doublés en hauteur" % n)
