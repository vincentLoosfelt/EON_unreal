# EON — capture de contrôle de la course de l'Onibi (A_Onibi_Run) : 6 instants de la boucle côte à côte, de profil,
# au-dessus du HUB (rien n'est enregistré). Sortie : Saved/Shots/Anims/Onibi_course.png puis l'éditeur se ferme.
#   UnrealEditor.exe EON.uproject -ExecutePythonScript=Saved/Scripts/eon_onibi_course_shots.py
import os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Anims"
os.makedirs(OUT, exist_ok=True)
ANIM = os.environ.get("EON_ANIM", "A_Onibi_Run")
NOM = os.environ.get("EON_NOM", "Onibi")
FICHIER = "%s/%s.png" % (OUT, os.environ.get("EON_SORTIE", "Onibi_course"))
if os.path.exists(FICHIER):
    os.remove(FICHIER)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for v in eas.get_all_level_actors():
    if isinstance(v, unreal.PostProcessVolume):
        v.set_editor_property("enabled", False)
for cmd in ("r.TextureStreaming 0", "r.DefaultFeature.AutoExposure 0", "r.DefaultFeature.MotionBlur 0", "r.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, cmd)
X0, Y0, Z0, N, PAS = 0.0, 0.0, 6000.0, 6, 150.0
sol = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(X0, Y0, Z0 - 1), unreal.Rotator(0, 0, 0))
sol.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
sol.set_actor_scale3d(unreal.Vector(14, 8, 1))
mur = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(X0, Y0 + 300, Z0 + 250), unreal.Rotator(0, 0, 0))
mur.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
mur.set_actor_scale3d(unreal.Vector(14, 0.2, 5))
seq = unreal.load_asset("/Game/EON/Ennemis/%s/Anims/%s" % (NOM, ANIM))
sk = unreal.load_asset("/Game/EON/Ennemis/%s/SK_%s" % (NOM, NOM))
L = seq.get_play_length()
COMPS = []
for i in range(N):
    # Acteur face +X (modèle à lacet -90, comme AEONEnemy) : de profil pour une caméra qui regarde +Y.
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(X0 + (i - (N - 1) / 2.0) * PAS, Y0, Z0), unreal.Rotator(0, 0, -90))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(sk)
    # Sans « update_animation_in_editor », le monde de l'éditeur n'évalue pas l'anim : toutes les poses sont pareilles.
    c.set_update_animation_in_editor(True)
    c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    c.override_animation_data(seq, False, False, L * i / N, 0.0)
    COMPS.append((c, L * i / N))
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(X0, Y0 - 900, Z0 + 110), unreal.Rotator(0, -3, 90))
cam.camera_component.set_editor_property("field_of_view", 55.0)
state = {"wait": 0, "pris": False}


def tick(dt):
    state["wait"] += 1
    # Position reposée après l'initialisation des composants (sinon toutes les poses restent identiques).
    if not state["pris"] and state["wait"] in (60, 100, 140):
        for c, t in COMPS:
            c.play_animation(seq, False); c.set_position(t, False); c.set_play_rate(0.0)
    if not state["pris"] and state["wait"] > 150:
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, FICHIER, cam)
        state["pris"] = True; state["wait"] = 0
    elif state["pris"] and state["wait"] > 60 and os.path.exists(FICHIER):
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
