# EON — rendu 4K du prototype de décor peint dans le vrai L_HUB, personnages compris (même placement et mêmes caméras
# que eon_persos_hub_4k.py). Les MI_BG_* du décor sont remplacées EN MÉMOIRE par MI_Peint_*_A / _B (rien n'est
# enregistré, la carte reste intacte), post-process MI_PP_EON_Decor_A / _B avant le trait. « AVANT » = HUB actuel.
# Sortie : Saved/Shots/DecorPeint/<plan>.png (3840×2160)
#   UnrealEditor.exe "D:/_PROJETS/briscard/EON_UE/EON.uproject" -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_decor_peint_4k.py"
# Variable EON_PEINT_PLANS="A,B" pour ne rendre que certaines variantes.
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/DecorPeint"
os.makedirs(OUT, exist_ok=True)
for c in ("t.IdleWhenNotForeground 0", "r.ScreenPercentage 100", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0",
          "r.HighResScreenshotDelay 32", "r.Editor.Viewport.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, c)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MEL = unreal.MaterialEditingLibrary
ppv = [v for v in eas.get_all_level_actors() if isinstance(v, unreal.PostProcessVolume)][0]
ppv.set_editor_property("enabled", True)
TRAIT = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")
FILM = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Film")
FILM_MI = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Film")
PPD = {v: unreal.load_asset("/Game/EON/Materials/Peint/MI_PP_EON_Decor_" + v) for v in ("A", "B")}

SOL = 465.0
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

# Emplacements de matériaux du décor : (composant, index, MI_BG d'origine)
SLOTS = []
for act in eas.get_all_level_actors():
    for comp in act.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(comp.get_num_materials()):
            mt = comp.get_material(i)
            if mt and mt.get_name().startswith("MI_BG_") and mt.get_name() != "MI_BG_Room_light":
                SLOTS.append((comp, i, mt))
PEINT = {}


def peint(mt, v):
    key = (mt.get_name(), v)
    if key not in PEINT:
        PEINT[key] = unreal.load_asset("/Game/EON/Materials/Peint/" + mt.get_name().replace("MI_BG_", "MI_Peint_") + "_" + v)
    return PEINT[key]


def variante(v, debug=0.0):
    # « A@LiserAdd=0.5;LiserBlanc=0 » : réglages du post-process posés en mémoire pour ce plan
    v, _, reg = v.partition("@")
    if v == "CARTE":   # la carte telle qu'enregistrée ; « CARTE@F.GrainStrength=0.1;UE.bloom_intensity=1 » : essais
        # en mémoire du grain (MI_PP_EON_Film, mis à la place du film) et du bloom Unreal (réglages du volume)
        if reg:
            s = ppv.get_editor_property("settings")
            bl = list(s.get_editor_property("weighted_blendables").get_editor_property("array"))
            for kv in filter(None, reg.split(";")):
                k, x = kv.split("=")
                if k.startswith("F."):
                    MEL.set_material_instance_scalar_parameter_value(FILM_MI, k[2:], float(x))
                    bl = [unreal.WeightedBlendable(1.0, FILM_MI) if (b.get_editor_property("object") and b.get_editor_property("object").get_name() in ("M_PP_EON_Film", "MI_PP_EON_Film")) else b for b in bl]
                elif k.startswith("UE."):
                    s.set_editor_property("override_" + k[3:], True); s.set_editor_property(k[3:], float(x))
            MEL.update_material_instance(FILM_MI)
            s.set_editor_property("weighted_blendables", unreal.WeightedBlendables(bl))
            ppv.set_editor_property("settings", s)
        return
    for comp, i, mt in SLOTS:
        comp.set_material(i, mt if v == "AVANT" else (peint(mt, v) or mt))
    bl = [unreal.WeightedBlendable(1.0, TRAIT), unreal.WeightedBlendable(1.0, FILM)]
    if v != "AVANT":
        MEL.set_material_instance_scalar_parameter_value(PPD[v], "DebugView", debug)
        for kv in filter(None, reg.split(";")):
            k, x = kv.split("=")
            MEL.set_material_instance_scalar_parameter_value(PPD[v], k, float(x))
        MEL.update_material_instance(PPD[v])
        bl.insert(0, unreal.WeightedBlendable(1.0, PPD[v]))
    s = ppv.get_editor_property("settings")
    s.set_editor_property("weighted_blendables", unreal.WeightedBlendables(bl))
    ppv.set_editor_property("settings", s)


def camera(target, dist, pitch, fov):
    P = math.radians(pitch)
    cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(target.x, target.y + dist * math.cos(P), target.z + dist * math.sin(P)),
                                     unreal.Rotator(0, -pitch, -90))
    cam.camera_component.set_editor_property("field_of_view", fov)
    cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
    return cam


PRES = camera(unreal.Vector(980.0, 55.0, SOL + 70.0), 1450.0, 50.0, 25.0)
SERRE = camera(unreal.Vector(985.0, 40.0, SOL + 85.0), 1200.0, 50.0, 25.0)   # plan serré sur les personnages
LARGE = camera(unreal.Vector(980.0, 55.0, SOL + 90.0), 2700.0, 50.0, 25.0)
# Plan d'ensemble : boîte de tout le décor (hors ciel et volumes géants), caméra même direction, recul pour tout cadrer.
lo = [1e9] * 3; hi = [-1e9] * 3
for act in eas.get_all_level_actors():
    if not act.get_components_by_class(unreal.StaticMeshComponent):
        continue
    o, e = act.get_actor_bounds(False)
    if max(e.x, e.y, e.z) > 20000 or max(e.x, e.y, e.z) < 1:
        continue
    for k, (c, r) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
        lo[k] = min(lo[k], c - r); hi[k] = max(hi[k], c + r)
CTR = unreal.Vector(*[(l + h) / 2 for l, h in zip(lo, hi)])
RAY = 0.5 * math.sqrt(sum((h - l) ** 2 for l, h in zip(lo, hi)))
unreal.log("EON_PEINT boite HUB %s %s rayon %.0f" % (lo, hi, RAY))
HUB = camera(CTR, RAY / math.sin(math.radians(20)) * 0.62, 50.0, 40.0)


def camera_az(target, dist, pitch, fov, az):
    """Caméra qui regarde target depuis l'azimut az (degrés, 90 = côté +Y comme camera())."""
    P, A = math.radians(pitch), math.radians(az)
    pos = unreal.Vector(target.x + dist * math.cos(P) * math.cos(A), target.y + dist * math.cos(P) * math.sin(A), target.z + dist * math.sin(P))
    cam = eas.spawn_actor_from_class(unreal.CameraActor, pos, unreal.Rotator(0, -pitch, az + 180.0))
    cam.camera_component.set_editor_property("field_of_view", fov)
    cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
    return cam


PERS = unreal.Vector(980.0, 55.0, SOL + 85.0)
VUES = [("Vue_01_serre", SERRE), ("Vue_02_rapproche", PRES), ("Vue_03_large", LARGE), ("Vue_04_HUB_entier", HUB),
        ("Vue_05_trois_quarts", camera_az(PERS, 1300.0, 35.0, 30.0, 135.0)),
        ("Vue_06_contrechamp", camera_az(PERS, 1300.0, 40.0, 30.0, -90.0)),
        ("Vue_07_plongee", camera_az(PERS, 1100.0, 75.0, 30.0, 90.0)),
        ("Vue_08_axe_passerelle", camera_az(unreal.Vector(980.0, 55.0, SOL + 100.0), 800.0, 14.0, 45.0, 0.0))]
if os.environ.get("EON_PEINT_VUES"):   # ne refaire que certaines vues (préfixes séparés par des virgules)
    VUES = [(n, c) for n, c in VUES if any(n.startswith(k) for k in os.environ["EON_PEINT_VUES"].split(","))]
PLANS = os.environ.get("EON_PEINT_PLANS", "A,B,AVANT,DEBUG").split(",")
NOMS = {}   # EON_PEINT_NOMS="A@...=A_l50,..." : nom de fichier pour un plan à réglages
for kv in filter(None, os.environ.get("EON_PEINT_NOMS", "").split(",")):
    k, n = kv.rsplit("=", 1); NOMS[k] = n
SHOTS = []
for v in PLANS:
    if v == "DEBUG":
        SHOTS += [("DEBUG_valeurs_A", "A", 1.0, PRES), ("DEBUG_diaphs_A", "A", 2.0, PRES), ("DEBUG_aretes_A", "A", 3.0, PRES)]
    else:
        if v == "VUES":   # série de vues de la carte telle qu'enregistrée
            SHOTS += [(n, "CARTE", 0.0, c) for n, c in VUES]
            continue
        if v.startswith("HUB"):   # « HUB » ou « HUB:CARTE » : plan d'ensemble seul
            SHOTS += [("HUB_entier", v.split(":")[1] if ":" in v else "CARTE", 0.0, HUB)]
            continue
        nm = NOMS.get(v, v)
        if os.environ.get("EON_PEINT_SERRE") == "1":   # plan serré seul
            SHOTS += [("%s_serre" % nm, v, 0.0, SERRE)]
            continue
        SHOTS += [("%s_pres" % nm, v, 0.0, PRES), ("%s_large" % nm, v, 0.0, LARGE)]
unreal.log("EON_PEINT emplacements : %d ; plans : %s" % (len(SLOTS), [s[0] for s in SHOTS]))
state = {"i": 0, "n": 0}
variante(SHOTS[0][1], SHOTS[0][2])


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
    if n == (900 if i == 0 else 240):
        unreal.AutomationLibrary.take_high_res_screenshot(3840, 2160, f, SHOTS[i][3])
    elif n > (960 if i == 0 else 300) and os.path.exists(f):
        state["i"] = i + 1; state["n"] = 0
        if i + 1 < len(SHOTS):
            variante(SHOTS[i + 1][1], SHOTS[i + 1][2])


h = unreal.register_slate_post_tick_callback(tick)
