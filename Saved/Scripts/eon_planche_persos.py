# EON — planche 4K de tous les personnages du jeu (Vincent, 24/11 : « refaire une planche avec tous les personnages et leurs noms »).
# Banc du HUB à Z=6000, rendu validé du niveau (décor, trait v7, film) ; lumière relevée comme la planche du 23/11.
# Tailles = celles du jeu (ModelHCm de EONEnemy.cpp, sinon HH du proto × 6,25 cm). Rien n'est enregistré.
# Sortie : Saved/Shots/Persos/PLANCHE_BRUTE.png (3840×2160) puis noms posés par eon_planche_noms.py.
#   UnrealEditor.exe "D:/_PROJETS/briscard/EON_UE/EON.uproject" -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_planche_persos.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Persos"
os.makedirs(OUT, exist_ok=True)
for c in ("t.IdleWhenNotForeground 0", "r.ScreenPercentage 100", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0",
          "r.HighResScreenshotDelay 32", "r.Editor.Viewport.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, c)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
LOG = []
# Post-process : blendables du niveau gardés ; exposure compensation.
EV = 0.6   # carte : manuel, 0,26
ppv = [v for v in eas.get_all_level_actors() if isinstance(v, unreal.PostProcessVolume)][0]
ppv.set_editor_property("enabled", True)
s = ppv.get_editor_property("settings")
for k in ("auto_exposure_method", "auto_exposure_bias", "auto_exposure_min_brightness", "auto_exposure_max_brightness"):
    try:
        LOG.append("PPV avant %s = %s (override %s)" % (k, s.get_editor_property(k), s.get_editor_property("override_" + k)))
    except Exception as ex:
        LOG.append("PPV %s ? %s" % (k, ex))
ppv.set_editor_property("settings", s)

CX = float(os.environ.get("EON_PL_CX", "0"))
E = "/Game/EON/Ennemis/"
U = "/Game/EON/Unity/import/fbx/"
# (nom, mesh, anim, x, hauteur cm, lacet, flotte cm)
PERSOS = [("Jira", "/Game/EON/Characters/Jira/SK_Jira_UEFN", None, -345.0, 181.25, 0, 0),
          ("Nuke", E + "Nuke/SK_Nuke", E + "Nuke/Anims/A_Nuke_Walk", -215.0, 40.0, 20, 0),
          ("Onibi", E + "Onibi/SK_Onibi", E + "Onibi/Anims/A_Onibi_Run", -95.0, 150.0, -25, 0),
          ("Kama", E + "Kama/SK_Kama", E + "Kama/Anims/A_Kama_Run", 45.0, 180.0, 15, 0),
          ("Ubume", E + "Ubume/SK_Ubume", E + "Ubume/Anims/A_Ubume_Walk", 195.0, 200.0, -15, 0),
          ("Ikiryo", E + "Ikiryo/SK_Ikiryo", None, 340.0, 180.0, 0, 0)]
# 24/11 (2e planche) : Petite Nuke, Brainless et Méduse retirées ; écart de l'aberration chromatique divisé par 2.
mi_film = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Film")
chroma = MEL.get_material_instance_scalar_parameter_value(mi_film, "ChromaPx")
LOG.append("ChromaPx avant = %s" % chroma)
if chroma > 2.0:
    MEL.set_material_instance_scalar_parameter_value(mi_film, "ChromaPx", chroma * 0.5)
    MEL.update_material_instance(mi_film)
    EAL.save_loaded_asset(mi_film)
LOG.append("ChromaPx après = %s" % MEL.get_material_instance_scalar_parameter_value(mi_film, "ChromaPx"))
BX, BY, BZ = 0.0, 0.0, 6000.0
for nom, path, anim, x, taille, yaw, flotte in PERSOS:
    m = unreal.load_asset(path)
    bd = m.get_bounds()
    k = taille / max(bd.box_extent.z * 2, 1.0)
    z = BZ - (bd.origin.z - bd.box_extent.z) * k + flotte
    if isinstance(m, unreal.SkeletalMesh):
        a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(BX + x, BY, z), unreal.Rotator(0, 0, yaw))
        c = a.skeletal_mesh_component
        c.set_skinned_asset_and_update(m)
        if anim:
            seq = unreal.load_asset(anim)
            c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
            c.override_animation_data(seq, False, False, seq.get_play_length() * 0.25, 0.0)
    else:
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(BX + x, BY, z), unreal.Rotator(0, 0, yaw))
        a.static_mesh_component.set_static_mesh(m)
    a.set_actor_scale3d(unreal.Vector(k, k, k))
    unreal.EONStatics.mark_cel(a)
    LOG.append("%s k=%.3f bornes %s" % (nom, k, bd.box_extent))
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for loc, sc in ((unreal.Vector(BX, BY, BZ - 50), unreal.Vector(40, 16, 1)), (unreal.Vector(BX, BY - 500, BZ + 250), unreal.Vector(40, 0.2, 7))):
    m = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0, 0, 0))
    m.static_mesh_component.set_static_mesh(cube)
    m.set_actor_scale3d(sc)

# Lumière : soleil de L_HUB réorienté et monté ; SkyLight de remplissage (cubemap de jour du moteur).
SOLEIL_LUX = 5.0
soleils = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.DirectionalLight)]
soleil = max(soleils, key=lambda a: a.light_component.get_editor_property("intensity")) if soleils else \
    eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(BX, BY, BZ + 500), unreal.Rotator(0, 0, 0))
soleil.set_actor_rotation(unreal.Rotator(0.0, -45.0, -60.0), False)
LOG.append("soleil gardé à %s lux" % soleil.light_component.get_editor_property("intensity"))
LOG.append("SkyLights existants : %s" % [a.get_actor_label() for a in eas.get_all_level_actors() if isinstance(a, unreal.SkyLight)])

P = math.radians(15.0)
D = float(os.environ.get("EON_PL_D", "2250"))
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(BX + CX, BY + D * math.cos(P), BZ + 95.0 + D * math.sin(P)),
                                 unreal.Rotator(0, -15.0, -90))
cam.camera_component.set_editor_property("field_of_view", 25.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
open(OUT + "/reglages.txt", "w", encoding="utf-8").write(chr(10).join(LOG))
F = OUT + "/PLANCHE_BRUTE.png"
if os.path.exists(F):
    os.remove(F)
state = {"n": 0}


def tick(dt):
    state["n"] += 1
    n = state["n"]
    if n == 600:
        unreal.AutomationLibrary.take_high_res_screenshot(3840, 2160, F, cam)
    elif n > 660 and os.path.exists(F):
        unreal.unregister_slate_post_tick_callback(h)
        open(OUT + "/fini.txt", "w").write("ok")
        unreal.SystemLibrary.quit_editor()


h = unreal.register_slate_post_tick_callback(tick)
