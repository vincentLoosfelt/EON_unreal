# EON — collisions du HUB comme dans Unity : MeshCollider sur les rambardes (Barrière01 -> Barrier_V3_1),
# et collision exacte sur murs, portes et grilles (bords de la salle et passerelles). Dôme de la COM : toujours sans collision.
import unreal
EAL = unreal.EditorAssetLibrary
BASE = "/Game/EON/Unity/import/"
NOMS = ["fbx/Barriere/Barrier_V3_1", "fbx/enviro/Grille01", "fbx/enviro/porte", "fbx/enviro/wall/WallA320",
        "fbx/enviro/wall/WallA160CornerIn", "walldooreno"]
log = []
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for n in NOMS:
    folder, name = (BASE + n).rsplit("/", 1)
    for ad in reg.get_assets_by_path(folder, recursive=False):
        a = ad.get_asset()
        if str(ad.asset_name).startswith(name) and isinstance(a, unreal.StaticMesh):
            bs = a.get_editor_property("body_setup")
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            a.set_editor_property("body_setup", bs)
            EAL.save_loaded_asset(a)
            log.append("%s : complexe = simple" % ad.asset_name)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_hub_collisions.txt", "w", encoding="utf-8").write("\n".join(log))
