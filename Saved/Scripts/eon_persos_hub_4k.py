# EON — tous les personnages dans le vrai décor de L_HUB (Vincent, 23/11 : « propose-les-moi dans le vrai décor, là où tu
# le proposais avant » ; décor et éclairage du niveau inchangés). Sur la passerelle Grille01 (5) / (7) : x 780..1180,
# y -47..153, dessus à z 465. Tailles de jeu, trait v6 (réglages par défaut de M_PP_EON_Trait), film F4.
# Textures pleine résolution. Rien n'est enregistré. Sortie : Saved/Shots/Persos/EON_persos_HUB_4K.png (3840×2160)
#   UnrealEditor.exe "D:/_PROJETS/briscard/EON_UE/EON.uproject" -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_persos_hub_4k.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Persos"
os.makedirs(OUT, exist_ok=True)
for c in ("t.IdleWhenNotForeground 0", "r.ScreenPercentage 100", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0",
          "r.HighResScreenshotDelay 32", "r.Editor.Viewport.ScreenPercentage 100"):
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

SOL = 465.0
# (squelette, anim, x, y, taille cm, yaw) — 3 rangs décalés, les petits devant (la caméra est du côté +Y)
PERSOS = [("/Game/EON/Ennemis/Kama/SK_Kama", "/Game/EON/Ennemis/Kama/Anims/A_Kama_Run", 810.0, -5.0, 180.0, 15),
          ("/Game/EON/Characters/Jira/SK_Jira_UEFN", None, 990.0, -5.0, 181.25, 0),
          ("/Game/EON/Ennemis/Ubume/SK_Ubume", "/Game/EON/Ennemis/Ubume/Anims/A_Ubume_Walk", 1150.0, -5.0, 200.0, -15),
          ("/Game/EON/Ennemis/Ikiryo/SK_Ikiryo", None, 880.0, 125.0, 180.0, 10),
          ("/Game/EON/Ennemis/Nuke/SK_Nuke", "/Game/EON/Ennemis/Nuke/Anims/A_Nuke_Walk", 1040.0, 60.0, 40.0, 20),
          ("/Game/EON/Ennemis/Onibi/SK_Onibi", "/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run", 1110.0, 120.0, 150.0, -25)]
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
        c.override_animation_data(seq, False, False, seq.get_play_length() * 0.25, 0.0)
    unreal.EONStatics.mark_cel(a)


def camera(target, dist, pitch, fov):
    P = math.radians(pitch)
    cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(target.x, target.y + dist * math.cos(P), target.z + dist * math.sin(P)),
                                     unreal.Rotator(0, -pitch, -90))
    cam.camera_component.set_editor_property("field_of_view", fov)
    cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
    return cam


# v2 : caméra plus plongeante (50°) pour voir par-dessus les rambardes (Nuke, 40 cm, était caché)
SHOTS = [("EON_persos_HUB_4K", camera(unreal.Vector(980.0, 55.0, SOL + 70.0), 1450.0, 50.0, 25.0)),
         ("EON_persos_HUB_4K_large", camera(unreal.Vector(980.0, 55.0, SOL + 90.0), 2700.0, 50.0, 25.0))]
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
