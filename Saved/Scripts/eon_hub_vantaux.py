# EON — L_HUB, sur place : les 4 portes (porte.fbx, « cadre » dans Unity) debout et dans le plan du mur.
# 15/11, Vincent : « les modules de porte sont mis à 90, en dessous de leur niveau ». Comme walldooreno, la racine
# Blender de porte.fbx n'est pas la racine standard (-90, 0, 0) : posée telle quelle, elle était à l'envers
# (Z 197..469 pour un sol à 464) et tournée de 90° (2,20 m en travers de l'ouverture). Retournement sur X, puis 90°
# autour de la verticale : debout, au centre de l'ouverture, largeur le long du mur.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_vantaux.py
import unreal
MAP = "/Game/EON/Maps/L_HUB"
TAG = "EON_porte_redressee"
MML = unreal.MathLibrary
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
n = 0
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    sm = a.static_mesh_component.static_mesh
    if sm is None or sm.get_name() != "porte" or TAG in [str(t) for t in a.tags]:
        continue
    r = MML.compose_rotators(unreal.Rotator(roll=180.0, pitch=0.0, yaw=0.0), a.get_actor_rotation())
    r = MML.compose_rotators(unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), r)
    a.set_actor_rotation(r, False)
    a.tags = list(a.tags) + [unreal.Name(TAG)]
    c, e = a.get_actor_bounds(False)
    unreal.log("EON_VANTAUX X %.0f..%.0f  Y %.0f..%.0f  Z %.0f..%.0f" % (c.x - e.x, c.x + e.x, c.y - e.y, c.y + e.y, c.z - e.z, c.z + e.z))
    n += 1
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), MAP)
unreal.log("EON_VANTAUX %d portes redressées" % n)
