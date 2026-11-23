# EON — rendus Full HD (1920×1080) des personnages avec tout le traitement retenu le 23/11 : trait celluloïd
# (M_PP_EON_Trait, cumul + ombre E1) et passe film F4 (M_PP_EON_Film), aux valeurs par défaut des matériaux.
# Deux plans : « banc » (fond neutre, caméra 7 m) et « hub » (dans L_HUB, caméra 10 m). Rien n'est enregistré.
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_beauty_shots.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Beauty"
os.makedirs(OUT, exist_ok=True)
unreal.SystemLibrary.execute_console_command(None, "t.IdleWhenNotForeground 0")
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

PERSOS = [("/Game/EON/Ennemis/Nuke/SK_Nuke", "/Game/EON/Ennemis/Nuke/Anims/A_Nuke_Walk", -1, 170.0, 20),
          ("/Game/EON/Characters/Jira/SK_Jira_UEFN", None, 0, 170.0, 0),
          ("/Game/EON/Ennemis/Onibi/SK_Onibi", "/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run", 1, 42.5, -25)]  # Onibi : Vincent 23/11, 4 fois plus petit (1/4 de Jira)


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


spawn_group(900.0, 50.0, 464.0, 150.0)
cam_hub = camera(unreal.Vector(900.0, 50.0, 554.0), 1000.0, 40.0)
BX, BY, BZ = 0.0, 0.0, 6000.0
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for loc, sc in ((unreal.Vector(BX, BY, BZ - 50), unreal.Vector(14, 14, 1)), (unreal.Vector(BX, BY - 500, BZ + 250), unreal.Vector(14, 0.2, 6))):
    m = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0, 0, 0))
    m.static_mesh_component.set_static_mesh(cube)
    m.set_actor_scale3d(sc)
lum = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(BX + 200, BY + 250, BZ + 450), unreal.Rotator(0, 0, 0))
lum.point_light_component.set_editor_property("intensity", 60.0)
lum.point_light_component.set_editor_property("attenuation_radius", 2000.0)
spawn_group(BX, BY, BZ, 120.0)
cam_banc = camera(unreal.Vector(BX, BY, BZ + 90.0), 700.0, 25.0)
SHOTS = [("EON_persos_banc_FullHD", cam_banc), ("EON_persos_hub_FullHD", cam_hub)]
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
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, f, SHOTS[i][1])
    elif n > (660 if i == 0 else 180) and os.path.exists(f):
        state["i"] = i + 1; state["n"] = 0


h = unreal.register_slate_post_tick_callback(tick)
