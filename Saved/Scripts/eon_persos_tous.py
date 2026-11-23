# EON — tous les personnages côte à côte, à leur taille de jeu (Vincent, 23/11 : Jira 181, Nuke 40, Onibi 150, Kama 180,
# Ubume 200, Ikiryo 180 cm ; AEONEnemy FLook::ModelHCm), avec tout le traitement : Toon BSDF (TP_EON_Anime tel qu'enregistré), trait E1 + ombre,
# film F4. Plan 4K (3840×2160) : « banc » (fond neutre) et « hub » (L_HUB). Rien n'est enregistré.
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_persos_tous.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Persos"
os.makedirs(OUT, exist_ok=True)
unreal.SystemLibrary.execute_console_command(None, "t.IdleWhenNotForeground 0")
# Netteté 4K (Vincent : « ce n'est pas du 4K ») : rendu à 100 %, textures toujours en pleine résolution, et 32 images
# de chauffe avant la prise pour que le TSR accumule son historique à la taille de la capture.
for c in ("r.ScreenPercentage 100", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0", "r.HighResScreenshotDelay 32",
          "r.Editor.Viewport.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, c)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ppv = [v for v in eas.get_all_level_actors() if isinstance(v, unreal.PostProcessVolume)][0]
ppv.set_editor_property("enabled", True)
s = ppv.get_editor_property("settings")
s.set_editor_property("weighted_blendables", unreal.WeightedBlendables([
    unreal.WeightedBlendable(1.0, unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")),
    unreal.WeightedBlendable(1.0, unreal.load_asset("/Game/EON/Materials/M_PP_EON_Film"))]))
ppv.set_editor_property("settings", s)

PERSOS = [("/Game/EON/Characters/Jira/SK_Jira_UEFN", None, -380.0, 181.25, 0),
          ("/Game/EON/Ennemis/Nuke/SK_Nuke", "/Game/EON/Ennemis/Nuke/Anims/A_Nuke_Walk", -240.0, 40.0, 20),
          ("/Game/EON/Ennemis/Onibi/SK_Onibi", "/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run", -115.0, 150.0, -25),
          ("/Game/EON/Ennemis/Kama/SK_Kama", "/Game/EON/Ennemis/Kama/Anims/A_Kama_Run", 35.0, 180.0, 15),
          ("/Game/EON/Ennemis/Ubume/SK_Ubume", "/Game/EON/Ennemis/Ubume/Anims/A_Ubume_Walk", 195.0, 200.0, -15),
          ("/Game/EON/Ennemis/Ikiryo/SK_Ikiryo", None, 350.0, 180.0, 0)]


def spawn_group(x0, y0, z0, ecart):
    for path, anim, i, taille, yaw in PERSOS:
        sk = unreal.load_asset(path)
        k = taille / max(sk.get_bounds().box_extent.z * 2, 1.0)
        a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x0 + i * ecart, y0, z0), unreal.Rotator(0, 0, yaw))
        a.set_actor_scale3d(unreal.Vector(k, k, k))
        c = a.skeletal_mesh_component
        c.set_skinned_asset_and_update(sk)
        if anim:
            seq = unreal.load_asset(anim)
            c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
            c.override_animation_data(seq, False, False, seq.get_play_length() * 0.25, 0.0)
        unreal.EONStatics.mark_cel(a)


def camera(target, dist, pitch):
    P = math.radians(pitch)
    cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(target.x, target.y + dist * math.cos(P), target.z + dist * math.sin(P)),
                                     unreal.Rotator(0, -pitch, -90))
    cam.camera_component.set_editor_property("field_of_view", 25.0)
    cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
    return cam


spawn_group(1050.0, 50.0, 464.0, 1.0)   # x = décalage en cm (colonne 3 de PERSOS)
cam_hub = camera(unreal.Vector(1050.0, 50.0, 534.0), 2900.0, 40.0)
BX, BY, BZ = 0.0, 0.0, 6000.0
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for loc, sc in ((unreal.Vector(BX, BY, BZ - 50), unreal.Vector(30, 16, 1)), (unreal.Vector(BX, BY - 500, BZ + 250), unreal.Vector(30, 0.2, 6))):
    m = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0, 0, 0))
    m.static_mesh_component.set_static_mesh(cube)
    m.set_actor_scale3d(sc)
# Lumière directionnelle (Vincent : « une lumière directionnelle, pas une point light ») : le soleil de L_HUB,
# monté à 4 lux et réorienté pour la prise, venant d'en haut à gauche côté caméra. Rien n'est enregistré.
soleils = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.DirectionalLight)]
if soleils:
    soleil = max(soleils, key=lambda a: a.light_component.get_editor_property("intensity"))
else:
    soleil = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(BX, BY, BZ + 500), unreal.Rotator(0, 0, 0))
    soleil.light_component.set_editor_property("intensity", 5.0)
unreal.log("EON_PERSOS soleil %s intensité %s" % (soleil.get_actor_label(), soleil.light_component.get_editor_property("intensity")))
soleil.set_actor_rotation(unreal.Rotator(0.0, -45.0, -60.0), False)   # roll, pitch, yaw : descend à 45°, vers +X / -Y
soleil.light_component.set_editor_property("intensity", 4.0)   # 0,6 lux en jeu : trop sombre au banc (exposition de la carte)
spawn_group(BX, BY, BZ, 1.0)
cam_banc = camera(unreal.Vector(BX - 15.0, BY, BZ + 95.0), 2050.0, 15.0)
SHOTS = [("EON_tous_persos_banc_4K", cam_banc)]
state = {"i": 0, "n": 0}


def tick(dt):
    state["n"] += 1
    i, n = state["i"], state["n"]
    if i >= len(SHOTS):
        if n > 60:
            unreal.unregister_slate_post_tick_callback(h)
            open(OUT + "/fini.txt", "w").write("ok")
            unreal.SystemLibrary.quit_editor()
        return
    f = "%s/%s.png" % (OUT, SHOTS[i][0])
    if n == (600 if i == 0 else 120):
        unreal.AutomationLibrary.take_high_res_screenshot(3840, 2160, f, SHOTS[i][1])
    elif n > (660 if i == 0 else 180) and os.path.exists(f):
        state["i"] = i + 1; state["n"] = 0


h = unreal.register_slate_post_tick_callback(tick)
