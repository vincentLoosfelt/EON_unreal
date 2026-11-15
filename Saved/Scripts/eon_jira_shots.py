# EON — captures de Jira dans le HUB depuis l'éditeur (-ExecutePythonScript) : poses idle / course / marche / tir / hacking
# sur la passerelle est, caméra rapprochée. Rien n'est enregistré dans la carte. Sortie : Saved/Shots/Jira.
import os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Jira"
os.makedirs(OUT, exist_ok=True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sk = unreal.load_asset("/Game/EON/Characters/Jira/SK_Jira")
scale = 181.0 / 198.0
POSES = [("idle", 0.0), ("run", 0.3), ("walk", 0.4), ("shoot_front", 0.5), ("hacking", 0.5)]
acts = []
for i, (anim, t) in enumerate(POSES):
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(820 + i * 90, 55, 464), unreal.Rotator(0, 0, 180 if i % 2 else 90))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(sk)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    seq = unreal.load_asset("/Game/EON/Characters/Jira/Anims/Jira/Jira_" + anim)
    c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    c.override_animation_data(seq, False, False, seq.get_play_length() * t, 0.0)
    acts.append(a)
cams = []
for name, loc, rot in (("jira_groupe", unreal.Vector(1000, 520, 700), unreal.Rotator(0, -22, -110)),
                       ("jira_face", unreal.Vector(830, 260, 580), unreal.Rotator(0, -8, -95))):
    cam = eas.spawn_actor_from_class(unreal.CameraActor, loc, rot)
    cam.camera_component.set_editor_property("field_of_view", 50.0)
    cams.append((name, cam))
state = {"i": 0, "wait": 0}


def tick(dt):
    state["wait"] += 1
    if state["wait"] < 150:
        return
    if state["i"] < len(cams):
        name, cam = cams[state["i"]]
        unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, "%s/%s.png" % (OUT, name), cam)
        state["i"] += 1; state["wait"] = 60
    elif state["wait"] > 220:
        unreal.unregister_slate_post_tick_callback(h)
        open(OUT + "/fini.txt", "w").write("ok")
        unreal.SystemLibrary.quit_editor()


h = unreal.register_slate_post_tick_callback(tick)
