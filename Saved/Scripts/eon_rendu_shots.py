# EON — capture d'état des lieux du rendu (lecture seule, rien n'est enregistré) : L_HUB vu par la caméra de jeu
# (plongée 55°, FOV 25°, 30 m), Jira + Nuke + Onibi au milieu du décor.
# EON_PP=jeu : comme en jeu aujourd'hui (volume actif, peinture à 0) ; peinture : blendable des cartes à 1 ;
# brut : volume coupé ; trait : blendable des cartes (M_PP_EON_Trait) + celluloïd (MarkCel) sur les personnages. Une capture par lancement :
#   UnrealEditor.exe EON.uproject -ExecCmds="py Saved/Scripts/eon_rendu_shots.py"
import os, unreal
MODE = os.environ.get("EON_PP", "jeu")
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Rendu"
os.makedirs(OUT, exist_ok=True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for v in eas.get_all_level_actors():
    if isinstance(v, unreal.PostProcessVolume):
        v.set_editor_property("enabled", MODE != "brut")
        s = v.get_editor_property("settings")
        arr = s.get_editor_property("weighted_blendables").get_editor_property("array")
        for b in arr:
            b.set_editor_property("weight", 1.0 if MODE in ("peinture", "trait") else 0.0)
        s.set_editor_property("weighted_blendables", unreal.WeightedBlendables(arr))
        v.set_editor_property("settings", s)
X0, Y0, Z0 = 900.0, 50.0, 464.0
PERSOS = [("/Game/EON/Characters/Jira/SK_Jira_UEFN", None, 0.0, 170.0),
          ("/Game/EON/Ennemis/Nuke/SK_Nuke", "/Game/EON/Ennemis/Nuke/Anims/A_Nuke_Walk", -220.0, 170.0),
          ("/Game/EON/Ennemis/Onibi/SK_Onibi", "/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run", 220.0, 150.0)]
for path, anim, dx, taille in PERSOS:
    sk = unreal.load_asset(path)
    k = taille / max(sk.get_bounds().box_extent.z * 2, 1.0)
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(X0 + dx, Y0, Z0), unreal.Rotator(0, 0, 0))
    a.set_actor_scale3d(unreal.Vector(k, k, k))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(sk)
    if anim:
        seq = unreal.load_asset(anim)
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        c.override_animation_data(seq, False, False, seq.get_play_length() * 0.25, 0.0)
    if MODE == "trait":
        unreal.EONStatics.mark_cel(a)   # comme en jeu : stencil celluloïd
import math
P = math.radians(55.0)
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(X0, Y0 + 3000 * math.cos(P), Z0 + 90 + 3000 * math.sin(P)), unreal.Rotator(0, -55, -90))
cam.camera_component.set_editor_property("field_of_view", 25.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
state = {"n": 0, "done": False}
F = "%s/HUB_%s.png" % (OUT, MODE)


def tick(dt):
    state["n"] += 1
    if state["n"] == 700:
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, F, cam)
    elif state["n"] > 760 and os.path.exists(F) and not state["done"]:
        state["done"] = True
        unreal.unregister_slate_post_tick_callback(h)
        open(OUT + "/fini_%s.txt" % MODE, "w").write("ok")
        unreal.SystemLibrary.quit_editor()


h = unreal.register_slate_post_tick_callback(tick)
