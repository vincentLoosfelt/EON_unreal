# EON — contrôle des jambes de Jira (SK_Jira_UEFN) : repos + 5 instants de marche avant, de face, puis la même chose
# sur le mannequin UEFN pour comparer. Au-dessus du HUB, rien n'est enregistré. Sorties : Saved/Shots/Anims/
# Jira_poses_<vue>.png et Mannequin_poses_<vue>.png, puis l'éditeur se ferme. EON_VUE = face (défaut) ou profil.
#   UnrealEditor.exe EON.uproject -ExecCmds="py Saved/Scripts/eon_jira_poses_shots.py"
import os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Anims"
VUE = os.environ.get("EON_VUE", "face")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for v in eas.get_all_level_actors():
    if isinstance(v, unreal.PostProcessVolume):
        v.set_editor_property("enabled", False)
for cmd in ("r.TextureStreaming 0", "r.DefaultFeature.AutoExposure 0", "r.DefaultFeature.MotionBlur 0", "r.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, cmd)
X0, Y0, Z0, PAS = 0.0, 0.0, 6000.0, 130.0
sol = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(X0, Y0, Z0 - 1), unreal.Rotator(0, 0, 0))
sol.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
sol.set_actor_scale3d(unreal.Vector(14, 8, 1))
mur = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(X0, Y0 + 350, Z0 + 250), unreal.Rotator(0, 0, 0))
mur.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
mur.set_actor_scale3d(unreal.Vector(14, 0.2, 5))
A = "/Game/Characters/UEFN_Mannequin/Animations/"
idle = unreal.load_asset(A + "Idle/M_Neutral_Stand_Idle_Loop")
walk = unreal.load_asset(A + "Walk/M_Neutral_Walk_Loop_F")
POSES = [(idle, 0.0)] + [(walk, walk.get_play_length() * i / 5) for i in range(5)]
MESH = [("Jira", unreal.load_asset("/Game/EON/Characters/Jira/SK_Jira_UEFN")),
        ("Mannequin", unreal.load_asset("/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin"))]
# Le mesh UEFN regarde +Y : lacet 180 -> il regarde -Y, vers la caméra (de face) ; lacet 90 -> de profil.
YAW = 180 if VUE == "face" else 90
COMPS = []
for i, (seq, t) in enumerate(POSES):
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(X0 + (i - 2.5) * PAS, Y0, Z0), unreal.Rotator(0, 0, YAW))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(MESH[0][1])
    c.set_update_animation_in_editor(True)
    c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    c.override_animation_data(seq, False, False, t, 0.0)
    COMPS.append((c, seq, t))
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(X0, Y0 - 950, Z0 + 100), unreal.Rotator(0, -3, 90))
cam.camera_component.set_editor_property("field_of_view", 50.0)
st = {"wait": 0, "k": 0, "pris": False}
def fichier(k): return "%s/%s_poses_%s.png" % (OUT, MESH[k][0], VUE)
for k in range(len(MESH)):
    if os.path.exists(fichier(k)): os.remove(fichier(k))
def tick(dt):
    st["wait"] += 1
    if not st["pris"] and st["wait"] in (60, 100, 140):
        for c, seq, t in COMPS:
            c.play_animation(seq, False); c.set_position(t, False); c.set_play_rate(0.0)
    if not st["pris"] and st["wait"] > 150:
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, fichier(st["k"]), cam)
        st["pris"] = True; st["wait"] = 0
    elif st["pris"] and st["wait"] > 60 and os.path.exists(fichier(st["k"])):
        st["k"] += 1
        if st["k"] >= len(MESH):
            unreal.unregister_slate_post_tick_callback(handle); unreal.SystemLibrary.quit_editor(); return
        for c, seq, t in COMPS:
            c.set_skinned_asset_and_update(MESH[st["k"]][1])
        st["pris"] = False; st["wait"] = 0
handle = unreal.register_slate_post_tick_callback(tick)
