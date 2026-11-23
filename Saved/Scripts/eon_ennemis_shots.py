# EON — captures des ennemis du studio dans le HUB (-ExecutePythonScript) : SK_<Nom> en pose de leur anim de jeu,
# même rotation que AEONEnemy (acteur face +X, modèle à yaw -90), tous ramenés à 180 cm. Rien n'est enregistré.
# Sortie : Saved/Shots/Ennemis (face, profil).
import os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Ennemis"
os.makedirs(OUT, exist_ok=True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MOBS = [("Nuke", "A_Nuke_Walk"), ("Onibi", "A_Onibi_Run"), ("Kama", "A_Kama_Run"), ("Ubume", "A_Ubume_Walk"), ("Ikiryo", None)]
X0, Y0, Z0 = 900.0, -240.0, 464.0
for i, (nom, anim) in enumerate(MOBS):
    sk = unreal.load_asset("/Game/EON/Ennemis/%s/SK_%s" % (nom, nom))
    h = sk.get_bounds().box_extent.z * 2
    k = 180.0 / max(h, 1.0)
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(X0, Y0 + i * 120, Z0), unreal.Rotator(0, 0, -90))
    a.set_actor_scale3d(unreal.Vector(k, k, k))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(sk)
    if anim:
        seq = unreal.load_asset("/Game/EON/Ennemis/%s/Anims/%s" % (nom, anim))
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        c.override_animation_data(seq, False, False, seq.get_play_length() * 0.25, 0.0)
cams = []
# Une caméra par ennemi, de 3/4 face (les ennemis regardent +X, la caméra vers -X).
for i, (nom, anim) in enumerate(MOBS):
    loc = unreal.Vector(X0 + 300, Y0 + i * 120 - 90, Z0 + 110)
    cam = eas.spawn_actor_from_class(unreal.CameraActor, loc, unreal.Rotator(0, -6, 163))
    cam.camera_component.set_editor_property("field_of_view", 55.0)
    cams.append((nom, cam))
# EON_ENNEMI=<Nom> : une seule capture (l'éditeur en arrière-plan cesse de tourner après la première).
if os.environ.get("EON_ENNEMI"):
    cams = [c for c in cams if c[0] == os.environ["EON_ENNEMI"]]
state = {"i": 0, "wait": 0}


def tick(dt):
    state["wait"] += 1
    if state["wait"] < (120 if state["i"] == 0 else 40):
        return
    if state["i"] < len(cams):
        name, cam = cams[state["i"]]
        unreal.AutomationLibrary.take_high_res_screenshot(900, 1100, "%s/%s.png" % (OUT, name), cam)
        state["i"] += 1; state["wait"] = 0
    elif state["wait"] > 60 and all(os.path.exists("%s/%s.png" % (OUT, n)) for n, _ in cams):
        unreal.unregister_slate_post_tick_callback(handle)
        open(OUT + "/fini.txt", "w").write("ok")


handle = unreal.register_slate_post_tick_callback(tick)
