# EON — construit L_HUB à partir du prefab Unity Room_elevator (aplati par eon_unity_flatten.py -> hub_elevator.json).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_build_hub.py
# Conversion de repère (vérifiée sur les 92 murs : longueur = X local, hauteur = Z local des deux côtés) :
#   monde : Unreal (X, Y, Z) = Unity (z, x, y) × 100
#   local : Unreal = Unity tourné de 180° autour de Z (Unity inverse X à l'import FBX, Unreal inverse Y)
# Éclairage dynamique, Lumen (GI + reflets) comme le reste du projet.
import json, os, re, unreal

HERE = os.path.dirname(os.path.abspath(__file__)) if "__file__" in dir() else unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Scripts"
DATA = json.load(open(os.path.join(HERE, "hub_elevator.json"), encoding="utf-8"))
MAP = "/Game/EON/Maps/L_HUB"
EAL = unreal.EditorAssetLibrary
REPORT = []


def log(m):
    REPORT.append(m)
    unreal.log("EON_HUB " + m)


def ue_asset_for(fbx_rel):
    base = os.path.splitext(fbx_rel)[0]
    parts = [re.sub(r"[^A-Za-z0-9_]", "_", p) for p in base.split("/")]
    folder = "/Game/EON/Unity/" + "/".join(parts[:-1])
    name = parts[-1]
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    ads = list(reg.get_assets_by_path(folder, recursive=False))
    for exact in (True, False):
        for ad in ads:
            n = str(ad.asset_name)
            if (n == name) if exact else n.startswith(name):
                a = ad.get_asset()
                if isinstance(a, (unreal.StaticMesh, unreal.SkeletalMesh)):
                    return a
    return None


def qrot(q, v):
    x, y, z, w = q

    def m(a, b):
        ax, ay, az, aw = a; bx, by, bz, bw = b
        return [aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz]
    return m(m(q, [v[0], v[1], v[2], 0]), [-x, -y, -z, w])[:3]


def to_ue(v):  # vecteur monde Unity -> Unreal
    return unreal.Vector(v[2], v[0], v[1])


def transform_of(o):
    q = o["rot"]
    # Axes locaux Unreal X, Z exprimés en local Unity (rotation 180° autour de Z) : X_ue = -x_u, Z_ue = z_u
    ax = to_ue(qrot(q, [-1, 0, 0]))
    az = to_ue(qrot(q, [0, 0, 1]))
    rot = unreal.MathLibrary.make_rot_from_xz(ax, az)
    s = o["scale"]
    loc = to_ue(o["pos"]) * 100.0
    if o.get("fbx", "").endswith("walldooreno.fbx"):
        # importé à part (/Game/EON/Unity/import/) avec un autre repère : sans ce retournement sur X il était à
        # l'envers, enfoncé et derrière les murs (trous, passage inversé ; Vincent 15/11, voir eon_hub_portes.py)
        rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=180.0, pitch=0.0, yaw=0.0), rot)
    if o.get("fbx", "").endswith("enviro/porte.fbx"):
        # porte : retournée sur X puis 90° sur la verticale (debout, dans le plan du mur ; 15/11, eon_hub_vantaux.py)
        rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=180.0, pitch=0.0, yaw=0.0), rot)
        rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), rot)
    if o.get("fbx", "").endswith("WallA160CornerIn.fbx"):
        # coin à 45° : -90° autour du pivot pour que sa diagonale rejoigne le second mur (15/11, eon_hub_coins.py)
        rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), rot)
    if o.get("fbx", "").split("/")[-1].startswith("WallA") and abs(s[1] - 2.0) < 0.01 and abs(s[2] - 1.0) < 0.01:
        s = [s[0], 1.0, 2.0]   # rangées basses : le 2 est la hauteur (6,1 m entre rangées), sinon 3 m de trou (15/11)
    return loc, rot, unreal.Vector(s[0], s[1], s[2])   # signe gardé : pièces en miroir


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
# L'ancien .umap est effacé sur disque avant le lancement (new_level refuse d'écraser).
les.new_level(MAP)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

cache, placed, missing = {}, 0, set()
xs, ys = [], []
for o in DATA:
    if o.get("fbx") and o["fbx"].endswith("COMv1.fbx"):
        o["active"] = True   # présente dans le HUB Unity ; le drapeau inactif vient d'un parent mal résolu
    if not o["active"]:
        continue
    loc, rot, scl = transform_of(o)
    if o.get("vfx"):
        # Arc électrique Unity (VFX) : en attendant le Niagara, une lumière bleue au même endroit.
        L = eas.spawn_actor_from_class(unreal.PointLight, loc, rot)
        L.point_light_component.set_editor_property("light_color", unreal.Color(120, 190, 255, 255))
        L.point_light_component.set_editor_property("intensity", 4000.0)
        L.point_light_component.set_editor_property("attenuation_radius", 600.0)
        L.set_actor_label("Arc_" + o["name"]); L.set_folder_path("HUB/Arcs")
        continue
    a = cache.get(o["fbx"]) if o["fbx"] in cache else cache.setdefault(o["fbx"], ue_asset_for(o["fbx"]))
    if a is None:
        missing.add(o["fbx"]); continue
    if o["fbx"].endswith("enviro/porte.fbx"):
        # porte Sas d'Unity : cadre + 2 vantaux coulissants, ouverte sans ennemis (AEONSlidingDoor, 15/11)
        act = eas.spawn_actor_from_class(unreal.EONSlidingDoor, loc, rot)
    elif isinstance(a, unreal.StaticMesh):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
        act.static_mesh_component.set_static_mesh(a)
    else:
        act = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, rot)
        act.skeletal_mesh_component.set_skinned_asset_and_update(a)
    act.set_actor_scale3d(scl)
    act.set_actor_label(o["name"] or a.get_name())
    act.set_folder_path("HUB/" + os.path.basename(os.path.dirname(o["fbx"])))
    xs.append(loc.x); ys.append(loc.y)
    placed += 1
log("posés : %d ; FBX introuvables : %s" % (placed, sorted(missing)))

cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
w_tiles = int((max(xs) - min(xs)) / 100) + 2
h_tiles = int((max(ys) - min(ys)) / 100) + 2
log("emprise %.0f..%.0f x %.0f..%.0f cm, centre (%.0f, %.0f)" % (min(xs), max(xs), min(ys), max(ys), cx, cy))

# Pas de sol : le HUB est un puits (anneaux de murs sur la hauteur) où la COM, centre d'opération mobile,
# peut descendre. Niveau jouable = y Unity 4,64 m (grilles-passerelles, portes, rampes).
FLOOR_Z = 464.0
# Portes fermées : rien derrière tant que les salles voisines n'existent pas (Unity : portes closes hors transition).
# Unity (x, z) des 4 portes -> Unreal (X = z, Y = x) ; bloc fin dans l'axe de la passerelle.
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for ux, uz, across_x in ((0.53, -16.9, True), (0.53, 12.1, True), (15.03, -2.4, False), (-13.97, -2.4, False)):
    b = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(uz * 100, ux * 100, FLOOR_Z + 200))
    b.static_mesh_component.set_static_mesh(cube)
    b.set_actor_scale3d(unreal.Vector(0.5, 5.0, 4.0) if across_x else unreal.Vector(5.0, 0.5, 4.0))
    b.set_actor_hidden_in_game(True)
    b.static_mesh_component.set_collision_profile_name("BlockAll")
    b.set_actor_label("Porte_fermee"); b.set_folder_path("HUB/Portes_fermees")

room = eas.spawn_actor_from_class(unreal.EONRoomBounds, unreal.Vector(cx, cy, FLOOR_Z))
room.set_editor_property("size_tiles", unreal.IntPoint(w_tiles, h_tiles))
room.set_actor_label("Salle_HUB")
# Départ sur la passerelle grillagée entre la COM et la porte est : Unity (x 0,5 ; z 9) -> Unreal (900, 50).
eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(900, 50, FLOOR_Z + 120)).set_actor_label("Depart_Jira")

sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -50, -40))
sun.light_component.set_editor_property("intensity", 4.68)   # 1,49 π : côté éclairé de la rampe toon = couleur × 1,49 (Unity)
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800))
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("intensity", 1.0)
eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
ppv.set_editor_property("unbound", True)
s = ppv.get_editor_property("settings")
s.set_editor_property("override_auto_exposure_method", True)
s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
s.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
s.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
s.set_editor_property("override_auto_exposure_bias", True)
s.set_editor_property("auto_exposure_bias", 0.0)
# Lumen : qualité maximale de la salle (final gather, éclairage de scène, reflets en hit lighting)
for k, v in (("lumen_final_gather_quality", 2.0), ("lumen_scene_lighting_quality", 2.0), ("lumen_scene_detail", 2.0),
             ("lumen_reflection_quality", 2.0), ("lumen_final_gather_lighting_update_speed", 2.0)):
    try:
        s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
    except Exception as e:
        log("PPV %s : %s" % (k, e))
try:
    s.set_editor_property("override_lumen_ray_lighting_mode", True)
    s.set_editor_property("lumen_ray_lighting_mode", unreal.LumenRayLightingModeOverride.HIT_LIGHTING_FOR_REFLECTIONS)
except Exception as e:
    log("PPV ray lighting : %s" % e)
# Volume Unity : Bloom seuil 1 / intensité 1 ; aberration chromatique légère ; passe peinture (PaintingEffect).
for k, v in (("bloom_threshold", 1.0), ("bloom_intensity", 1.0), ("scene_fringe_intensity", 0.5)):
    s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
# Post-process validé le 24/11 (Vincent) : décor aquarelle B, trait des celluloïds, film ; bloom 6 / 0,8 / 8, sans motion blur.
# (L'ancien filtre M_PP_EON_Peinture est supprimé.)
for k, v in (("bloom_intensity", 6.0), ("bloom_threshold", 0.8), ("bloom_size_scale", 8.0), ("motion_blur_amount", 0.0)):
    s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
s.set_editor_property("weighted_blendables", unreal.WeightedBlendables([unreal.WeightedBlendable(1.0, unreal.load_asset(p))
    for p in ("/Game/EON/Materials/Peint/MI_PP_EON_Decor_B", "/Game/EON/Materials/MI_PP_EON_Trait", "/Game/EON/Materials/MI_PP_EON_Film")]))
ppv.set_editor_property("settings", s)
ppv.set_actor_label("PP_HUB")

ws = unreal.EditorLevelLibrary.get_editor_world().get_world_settings()
ws.set_editor_property("default_game_mode", unreal.load_asset("/Game/EON/Blueprints/BP_EONGameMode").generated_class())
ok = les.save_current_level()
log("carte enregistrée : %s" % ok)
out = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_build_hub.txt"
open(out, "w", encoding="utf-8").write("\n".join(REPORT))
