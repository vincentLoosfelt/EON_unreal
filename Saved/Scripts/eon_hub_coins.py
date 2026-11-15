# EON — L_HUB, sur place : rotation des 12 coins à 45° (WallA160CornerIn) de -90° autour de leur pivot.
# 15/11, Vincent : « les coins sont mal orientés ». Chaque coin relie un bout de mur (au pivot) à un autre bout situé
# à 160 cm dans chaque direction ; posé tel quel, le mesh (X 0..200, Y -40..167 en local) partait du mauvais côté.
# Tourné de -90° (X local -> -Y, Y -> X), sa diagonale rejoint le second mur, pour les 4 coins de chaque rangée.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_coins.py
import unreal
MAP = "/Game/EON/Maps/L_HUB"
TAG = "EON_coin_tourne"
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
n = 0
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    sm = a.static_mesh_component.static_mesh
    if sm is None or sm.get_name() != "WallA160CornerIn" or TAG in [str(t) for t in a.tags]:
        continue
    a.set_actor_rotation(unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), a.get_actor_rotation()), False)
    a.tags = list(a.tags) + [unreal.Name(TAG)]
    n += 1
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), MAP)
unreal.log("EON_COINS %d coins tournés" % n)
