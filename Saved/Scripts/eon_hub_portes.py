# EON — L_HUB, sur place : retournement (180° autour de son axe X local) des 4 cadres de porte walldooreno.
# 15/11, Vincent : « des sortes de trous dans les murs, le passage des portes est inversé ». walldooreno est importé
# à part (/Game/EON/Unity/import/, pas fbx/enviro/) avec un autre repère local que les murs : posé comme eux, il était
# à l'envers (Z 154..474 pour un sol à 464 : enfoncé, il dépassait à peine) et 15 à 120 cm derrière la ligne des murs,
# face décorée vers l'extérieur -> passage ouvert sur le noir. Retourné sur X : debout, face vers la salle, dans la
# ligne des murs (ouest : Y -1482..-1325 pour des murs à -1427..-1329).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_portes.py
import unreal
MAP = "/Game/EON/Maps/L_HUB"
TAG = "EON_porte_retournee_X"
ESSAI = "EON_porte_retournee"   # 1er essai (180° sur Z seulement) : il reste le 180° sur Y à ajouter
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
n = 0
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    sm = a.static_mesh_component.static_mesh
    tags = [str(t) for t in a.tags]
    if sm is None or sm.get_name() != "walldooreno" or TAG in tags:
        continue
    loc = unreal.Rotator(roll=0.0, pitch=180.0, yaw=0.0) if ESSAI in tags else unreal.Rotator(roll=180.0, pitch=0.0, yaw=0.0)
    a.set_actor_rotation(unreal.MathLibrary.compose_rotators(loc, a.get_actor_rotation()), False)
    a.tags = [unreal.Name(t) for t in tags if t != ESSAI] + [unreal.Name(TAG)]
    c, e = a.get_actor_bounds(False)
    unreal.log("EON_PORTES X %.0f..%.0f  Y %.0f..%.0f  Z %.0f..%.0f" % (c.x - e.x, c.x + e.x, c.y - e.y, c.y + e.y, c.z - e.z, c.z + e.z))
    n += 1
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), MAP)
unreal.log("EON_PORTES %d cadres retournés" % n)
