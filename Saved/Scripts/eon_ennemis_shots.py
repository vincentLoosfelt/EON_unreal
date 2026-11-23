# EON — captures des ennemis du studio dans le HUB (-ExecutePythonScript) : SK_<Nom> en pose de leur anim de jeu,
# même rotation que AEONEnemy (acteur face +X, modèle à yaw -90), tous ramenés à 180 cm. Rien n'est enregistré.
# Sortie : Saved/Shots/Ennemis (face, profil).
import os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Ennemis"
os.makedirs(OUT, exist_ok=True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
# Sans post-process (Vincent, 23/11) : volumes coupés, peinture à 0. Rien n'est enregistré dans la carte.
for v in eas.get_all_level_actors():
    if isinstance(v, unreal.PostProcessVolume):
        v.set_editor_property("enabled", False)
# Rendu brut (Vincent, 23/11 : « enlève tout post-process ») : textures en pleine résolution tout de suite
# (pas de streaming progressif), et effets par défaut du moteur coupés (exposition auto, bloom, flou, grain,
# reflets d'objectif, franges, netteté). Les variables console ne sont pas enregistrées.
for cmd in ("r.TextureStreaming 0", "r.DefaultFeature.AutoExposure 0", "r.BloomQuality 0", "r.DefaultFeature.MotionBlur 0",
            "r.MotionBlurQuality 0", "r.LensFlareQuality 0", "r.SceneColorFringeQuality 0", "r.FilmGrain 0",
            "r.Tonemapper.GrainQuantization 0", "r.Tonemapper.Sharpen 0", "r.ScreenPercentage 100", "r.DepthOfFieldQuality 0"):
    unreal.SystemLibrary.execute_console_command(None, cmd)
MOBS = [("Nuke", "A_Nuke_Walk"), ("Onibi", "A_Onibi_Run"), ("Kama", "A_Kama_Run"), ("Ubume", "A_Ubume_Walk"), ("Ikiryo", None)]
X0, Y0, Z0 = 900.0, -240.0, 464.0
# EON_ENNEMI=<Nom> : cet ennemi seul, sur l'emplacement dégagé du milieu (une capture par lancement :
# l'éditeur en arrière-plan cesse de tourner après la première).
SEUL = os.environ.get("EON_ENNEMI")
# EON_TOUR=<degrés> : fait tourner l'ennemi seul (ex. 90 = profil) ; la capture prend le suffixe _<degrés>.
TOUR = float(os.environ.get("EON_TOUR", "0"))
SUF = ("_%d" % TOUR) if TOUR else ""
if SEUL:
    # Seul, au-dessus du HUB (ciel en fond, lumière du soleil), sur un sol neutre.
    MOBS = [m for m in MOBS if m[0] == SEUL]
    X0, Y0, Z0 = 0.0, 0.0, 6000.0
    sol = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(X0, Y0, Z0 - 1), unreal.Rotator(0, 0, 0))
    sol.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
    sol.set_actor_scale3d(unreal.Vector(12, 12, 1))
    # Salle vide : mur du fond et murs latéraux (cubes gris), plafonnier au-dessus de l'ennemi.
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    for loc, sc in ((unreal.Vector(X0 - 450, Y0, Z0 + 250), unreal.Vector(0.2, 12, 5)),
                    (unreal.Vector(X0, Y0 - 550, Z0 + 250), unreal.Vector(12, 0.2, 5)),
                    (unreal.Vector(X0, Y0 + 550, Z0 + 250), unreal.Vector(12, 0.2, 5))):
        m = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0, 0, 0))
        m.static_mesh_component.set_static_mesh(cube)
        m.set_actor_scale3d(sc)
    lum = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(X0 + 150, Y0 - 80, Z0 + 380), unreal.Rotator(0, 0, 0))
    lum.point_light_component.set_editor_property("intensity", 40.0)
    lum.point_light_component.set_editor_property("attenuation_radius", 1500.0)
for i, (nom, anim) in enumerate(MOBS):
    sk = unreal.load_asset("/Game/EON/Ennemis/%s/SK_%s" % (nom, nom))
    h = sk.get_bounds().box_extent.z * 2
    k = 180.0 / max(h, 1.0)
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(X0, Y0 + i * 120, Z0), unreal.Rotator(0, 0, -90 + TOUR))
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
    if SEUL:
        loc = unreal.Vector(X0 + 330, Y0 - 100, Z0 + 95)
    cam = eas.spawn_actor_from_class(unreal.CameraActor, loc, unreal.Rotator(0, -1 if SEUL else -6, 163))
    cam.camera_component.set_editor_property("field_of_view", 55.0)
    cams.append((nom, cam))
state = {"i": 0, "wait": 0}


def tick(dt):
    state["wait"] += 1
    if state["wait"] < (120 if state["i"] == 0 else 40):
        return
    if state["i"] < len(cams):
        name, cam = cams[state["i"]]
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, "%s/%s%s.png" % (OUT, name, SUF), cam)
        state["i"] += 1; state["wait"] = 0
    elif state["wait"] > 60 and all(os.path.exists("%s/%s%s.png" % (OUT, n, SUF)) for n, _ in cams):
        unreal.unregister_slate_post_tick_callback(handle)
        open(OUT + "/fini.txt", "w").write("ok")


handle = unreal.register_slate_post_tick_callback(tick)
