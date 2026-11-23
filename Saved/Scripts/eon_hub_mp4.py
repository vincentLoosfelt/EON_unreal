# EON — petit plan animé du HUB (Vincent, 23/11 : « un mp4 avec une petite rotation de caméra et un léger travelling
# avant sur les persos »). L_HUB tel qu'enregistré (décor peint, P5, bloom, grain, aberration), personnages placés comme
# dans eon_decor_peint_4k.py. Caméra : orbite de AZ0 à AZ1 degrés autour des personnages et rapprochement de D0 à D1,
# mouvement adouci (ease in/out). Animations des personnages en 12 images/s, en paliers (comme Orbitals) ; caméra en
# 30 images/s. Images 4K dans Saved/Shots/Film/images, puis encodage mp4 par eon_hub_mp4_encode.py.
#   UnrealEditor.exe "D:/_PROJETS/briscard/EON_UE/EON.uproject" -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_hub_mp4.py"
import math, os, unreal
OUT = os.environ.get("EON_MP4_OUT", "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Film/images")
os.makedirs(OUT, exist_ok=True)
FPS, DUREE = int(os.environ.get("EON_MP4_FPS", "30")), 6.0
PAS = float(os.environ.get("EON_MP4_PAS", "12"))              # 0 = animation fluide à la cadence de la vidéo
N = int(FPS * DUREE)
AZ0, AZ1, D0, D1, P0, P1 = 115.0, 145.0, 1550.0, 1150.0, 38.0, 32.0
W, H = int(os.environ.get("EON_MP4_W", "3840")), int(os.environ.get("EON_MP4_H", "2160"))
for c in ("t.IdleWhenNotForeground 0", "r.ScreenPercentage 100", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0",
          "r.HighResScreenshotDelay 32", "r.Editor.Viewport.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, c)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
SOL = 465.0
PERSOS = [("/Game/EON/Ennemis/Kama/SK_Kama", "/Game/EON/Ennemis/Kama/Anims/A_Kama_Run", 810.0, -5.0, 180.0, 15),
          ("/Game/EON/Characters/Jira/SK_Jira_UEFN", None, 990.0, -5.0, 181.25, 0),
          ("/Game/EON/Ennemis/Ubume/SK_Ubume", "/Game/EON/Ennemis/Ubume/Anims/A_Ubume_Walk", 1150.0, -5.0, 200.0, -15),
          ("/Game/EON/Ennemis/Ikiryo/SK_Ikiryo", None, 880.0, 125.0, 180.0, 10),
          ("/Game/EON/Ennemis/Nuke/SK_Nuke", "/Game/EON/Ennemis/Nuke/Anims/A_Nuke_Walk", 1040.0, 60.0, 40.0, 20),
          ("/Game/EON/Ennemis/Onibi/SK_Onibi", "/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run", 1110.0, 120.0, 150.0, -25)]
ANIMS = []
for path, anim, x, y, taille, yaw in PERSOS:
    sk = unreal.load_asset(path)
    k = taille / max(sk.get_bounds().box_extent.z * 2, 1.0)
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x, y, SOL), unreal.Rotator(0, 0, yaw))
    a.set_actor_scale3d(unreal.Vector(k, k, k))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(sk)
    if anim:
        seq = unreal.load_asset(anim)
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        c.override_animation_data(seq, True, False, 0.0, 0.0)
        ANIMS.append((c, seq.get_play_length()))
    unreal.EONStatics.mark_cel(a)
CIBLE = unreal.Vector(980.0, 55.0, SOL + 85.0)
cam = eas.spawn_actor_from_class(unreal.CameraActor, CIBLE, unreal.Rotator(0, 0, 0))
cam.camera_component.set_editor_property("field_of_view", 30.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)


def poser(i):
    u = i / max(N - 1, 1)
    e = u * u * (3 - 2 * u)                                    # adouci au départ et à l'arrivée
    az, d, p = AZ0 + (AZ1 - AZ0) * e, D0 + (D1 - D0) * e, P0 + (P1 - P0) * e
    P, A = math.radians(p), math.radians(az)
    pos = unreal.Vector(CIBLE.x + d * math.cos(P) * math.cos(A), CIBLE.y + d * math.cos(P) * math.sin(A), CIBLE.z + d * math.sin(P))
    cam.set_actor_location_and_rotation(pos, unreal.Rotator(0, -p, az + 180.0), False, False)
    t = math.floor(i / FPS * PAS) / PAS if PAS > 0 else i / FPS   # paliers de PAS images/s (12 = comme Orbitals)
    for c, L in ANIMS:
        c.set_position(t % L, False)


state = {"i": 0, "n": 0}
poser(0)


def tick(dt):
    state["n"] += 1
    i, n = state["i"], state["n"]
    if i >= N:
        if n > 30:
            unreal.unregister_slate_post_tick_callback(h)
            open(OUT + "/fini.txt", "w").write("ok %d" % N)
            unreal.SystemLibrary.quit_editor()
        return
    f = "%s/f%04d.png" % (OUT, i)
    if n == (900 if i == 0 else 4):
        unreal.AutomationLibrary.take_high_res_screenshot(W, H, f, cam)
    elif n > (900 if i == 0 else 4) and os.path.exists(f) and os.path.getsize(f) > 0:
        state["i"] = i + 1; state["n"] = 0
        if i + 1 < N:
            poser(i + 1)


h = unreal.register_slate_post_tick_callback(tick)
