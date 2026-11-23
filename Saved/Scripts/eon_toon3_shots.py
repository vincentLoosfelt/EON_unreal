# EON — planche des 3 tons des celluloïds dans le Substrate Toon BSDF (Vincent, 23/11 : utiliser le toon shader de la
# 5.8, raccord avec les vraies lumières). Variantes de la rampe du Toon Profile TP_EON_Anime (UEONStatics::
# ConfigureToonProfile3), trait E1 + film F4 actifs. Profil NON enregistré : l'éditeur quitte sans sauvegarder, l'asset
# sur disque reste la rampe 2 tons d'Unity. Vues « banc » (point lumineux) et « hub » (soleil de la carte).
# Sortie : Saved/Shots/Toon3/<variante>_<vue>.png
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_toon3_shots.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Toon3"
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
tp = unreal.load_asset("/Game/EON/Materials/TP_EON_Anime")
E = unreal.EONStatics
TINT = (1.0, 0.80146426, 0.50628924)
SH = unreal.LinearColor(*[t / (1 + t) for t in TINT], 1.0)            # ombre chaude d'Unity (rampe actuelle)
MID = unreal.LinearColor(*[(1 + t / (1 + t)) / 2 for t in TINT], 1.0)
SHB = unreal.LinearColor(0.40, 0.40, 0.56, 1.0)                      # ombre bleu-violet
MIDB = unreal.LinearColor(0.74, 0.73, 0.84, 1.0)
VARIANTES = [
    ("T0_actuel_2_tons_Unity", lambda: E.configure_toon_profile(tp, 0.5, 0.01, SH, 0.35, True)),
    ("T1_3_tons_ombre_chaude", lambda: E.configure_toon_profile3(tp, 0.45, 0.62, 0.02, SH, MID, 0.5, 0.0, 0.35, True)),
    ("T2_3_tons_ombre_bleutee", lambda: E.configure_toon_profile3(tp, 0.45, 0.62, 0.02, SHB, MIDB, 0.5, 0.0, 0.35, True)),
    ("T3_T1_reflet", lambda: E.configure_toon_profile3(tp, 0.45, 0.62, 0.02, SH, MID, 0.5, 0.6, 0.35, True)),
    ("T4_T2_reflet", lambda: E.configure_toon_profile3(tp, 0.45, 0.62, 0.02, SHB, MIDB, 0.5, 0.6, 0.35, True)),
]
VUES = [("banc", cam_banc), ("hub", cam_hub)]
SHOTS = [(v, nv, cam) for v in VARIANTES for nv, cam in VUES]
state = {"i": -1, "n": 0, "cur": None}


def fichier(sh):
    return "%s/%s_%s.png" % (OUT, sh[0][0], sh[1])


def tick(dt):
    state["n"] += 1
    n = state["n"]
    if state["i"] < 0:
        if n >= 600:
            state["i"] = 0; state["n"] = 0; SHOTS[0][0][1](); state["cur"] = SHOTS[0][0][0]
        return
    i = state["i"]
    if i >= len(SHOTS):
        if n > 90:
            unreal.unregister_slate_post_tick_callback(h)
            VARIANTES[0][1]()
            open(OUT + "/fini.txt", "w").write("ok")
            unreal.SystemLibrary.quit_editor()
        return
    if n == 150:
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, fichier(SHOTS[i]), SHOTS[i][2])
    elif n > 210 and os.path.exists(fichier(SHOTS[i])):
        state["i"] = i + 1; state["n"] = 0
        if i + 1 < len(SHOTS) and SHOTS[i + 1][0][0] != state["cur"]:
            SHOTS[i + 1][0][1](); state["cur"] = SHOTS[i + 1][0][0]


h = unreal.register_slate_post_tick_callback(tick)
