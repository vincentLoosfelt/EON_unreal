# EON — calibrage du trait v6 (encre multipliée) en 4K, sur le banc de tous les personnages (Vincent, 23/11 :
# « le trait est un peu trop gros, le taux de destruction / crayonné un peu trop fort » ; « une vraie planche 4K »).
# 3 épaisseurs × 3 forces de destruction (cassures + pression de main + bord flou), puis la planche finale.
# Éclairage relevé : soleil directionnel plus fort, SkyLight de remplissage, exposure compensation (+EV) dans le PPV.
# Textures toujours en pleine résolution (streaming coupé). Rien n'est enregistré : instance de test dans
# /Game/EON/_TmpTrait supprimée avant de quitter. Sortie : Saved/Shots/TraitCalib/<variante>.png (3840×2160)
#   UnrealEditor.exe "D:/_PROJETS/briscard/EON_UE/EON.uproject" -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_trait_calib_4k.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/TraitCalib"
os.makedirs(OUT, exist_ok=True)
for c in ("t.IdleWhenNotForeground 0", "r.ScreenPercentage 100", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0",
          "r.HighResScreenshotDelay 32", "r.Editor.Viewport.ScreenPercentage 100"):
    unreal.SystemLibrary.execute_console_command(None, c)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
TMP = "/Game/EON/_TmpTrait"
LOG = []
if EAL.does_directory_exist(TMP):
    EAL.delete_directory(TMP)
trait = AT.create_asset("MI_TraitCalib", TMP, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
trait.set_editor_property("parent", unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait"))

# Post-process : trait (instance de test) + film F4 ; exposure compensation.
EV = 0.6   # carte : manuel, 0,26
ppv = [v for v in eas.get_all_level_actors() if isinstance(v, unreal.PostProcessVolume)][0]
ppv.set_editor_property("enabled", True)
s = ppv.get_editor_property("settings")
for k in ("auto_exposure_method", "auto_exposure_bias", "auto_exposure_min_brightness", "auto_exposure_max_brightness"):
    try:
        LOG.append("PPV avant %s = %s (override %s)" % (k, s.get_editor_property(k), s.get_editor_property("override_" + k)))
    except Exception as ex:
        LOG.append("PPV %s ? %s" % (k, ex))
s.set_editor_property("override_auto_exposure_bias", True)
s.set_editor_property("auto_exposure_bias", EV)
s.set_editor_property("weighted_blendables", unreal.WeightedBlendables([
    unreal.WeightedBlendable(1.0, trait),
    unreal.WeightedBlendable(1.0, unreal.load_asset("/Game/EON/Materials/M_PP_EON_Film"))]))
ppv.set_editor_property("settings", s)

PERSOS = [("/Game/EON/Characters/Jira/SK_Jira_UEFN", None, -380.0, 181.25, 0),
          ("/Game/EON/Ennemis/Nuke/SK_Nuke", "/Game/EON/Ennemis/Nuke/Anims/A_Nuke_Walk", -240.0, 40.0, 20),
          ("/Game/EON/Ennemis/Onibi/SK_Onibi", "/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run", -115.0, 150.0, -25),
          ("/Game/EON/Ennemis/Kama/SK_Kama", "/Game/EON/Ennemis/Kama/Anims/A_Kama_Run", 35.0, 180.0, 15),
          ("/Game/EON/Ennemis/Ubume/SK_Ubume", "/Game/EON/Ennemis/Ubume/Anims/A_Ubume_Walk", 195.0, 200.0, -15),
          ("/Game/EON/Ennemis/Ikiryo/SK_Ikiryo", None, 350.0, 180.0, 0)]
BX, BY, BZ = 0.0, 0.0, 6000.0
for path, anim, x, taille, yaw in PERSOS:
    sk = unreal.load_asset(path)
    k = taille / max(sk.get_bounds().box_extent.z * 2, 1.0)
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(BX + x, BY, BZ), unreal.Rotator(0, 0, yaw))
    a.set_actor_scale3d(unreal.Vector(k, k, k))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(sk)
    if anim:
        seq = unreal.load_asset(anim)
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        c.override_animation_data(seq, False, False, seq.get_play_length() * 0.25, 0.0)
    unreal.EONStatics.mark_cel(a)
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for loc, sc in ((unreal.Vector(BX, BY, BZ - 50), unreal.Vector(30, 16, 1)), (unreal.Vector(BX, BY - 500, BZ + 250), unreal.Vector(30, 0.2, 6))):
    m = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0, 0, 0))
    m.static_mesh_component.set_static_mesh(cube)
    m.set_actor_scale3d(sc)

# Lumière : soleil de L_HUB réorienté et monté ; SkyLight de remplissage (cubemap de jour du moteur).
SOLEIL_LUX = 5.0
soleils = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.DirectionalLight)]
soleil = max(soleils, key=lambda a: a.light_component.get_editor_property("intensity")) if soleils else \
    eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(BX, BY, BZ + 500), unreal.Rotator(0, 0, 0))
soleil.set_actor_rotation(unreal.Rotator(0.0, -45.0, -60.0), False)
soleil.light_component.set_editor_property("intensity", SOLEIL_LUX)
LOG.append("SkyLights existants : %s" % [a.get_actor_label() for a in eas.get_all_level_actors() if isinstance(a, unreal.SkyLight)])
sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(BX, BY, BZ + 300), unreal.Rotator(0, 0, 0))
slc = sky.light_component
slc.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
slc.set_editor_property("cubemap", unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap"))
slc.set_editor_property("lower_hemisphere_is_black", False)
slc.set_editor_property("intensity", 0.5)
slc.recapture_sky()
LOG.append("soleil %s %s lux, SkyLight 0.5, exposure compensation +%s EV" % (soleil.get_actor_label(), SOLEIL_LUX, EV))

P = math.radians(15.0)
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(BX - 15.0, BY + 2050.0 * math.cos(P), BZ + 95.0 + 2050.0 * math.sin(P)),
                                 unreal.Rotator(0, -15.0, -90))
cam.camera_component.set_editor_property("field_of_view", 25.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)

# Calibrage 2 (Vincent, 23/11 : « je prendrais E1·D3, en grossissant encore un peu le trait » ; traits « absents sur
# certaines zones »). Référence = E1·D3 tel que rendu avant ; G1-G3 = E1·D3 grossi, amincissement en lumière réduit
# (plus de modulation par l'ombre depuis v6c) et traits intérieurs plus faciles à détecter (CreaseCos .60 -> .70 ; depuis v6d, profondeur, normales et teinte, jamais le ton).
D3 = dict(Breaks=0.10, Jitter=0.12, Soft=1.2)
DET = dict(CreaseCos=0.70)
SHOTS = [("REF_E1D3", dict(D3, SilWidth=2.6, InnerWidth=1.8, CreaseCos=0.60)),
         ("G1_x115", dict(D3, SilWidth=3.0, InnerWidth=2.1, **DET)),
         ("G2_x130", dict(D3, SilWidth=3.4, InnerWidth=2.35, **DET)),
         ("G3_x150", dict(D3, SilWidth=3.9, InnerWidth=2.7, **DET)),
         ("PLANCHE_G3", dict(D3, SilWidth=3.9, InnerWidth=2.7, **DET))]
if os.environ.get("EON_CALIB_BC"):   # contrôle : couleur de base (GBuffer) et masque du trait vus par le shader
    SHOTS = [("DEBUG_BC", dict(DebugView=1.0)), ("DEBUG_MASQUE", dict(DebugView=2.0))]
if os.environ.get("EON_CALIB_VERIF"):   # masque du trait + planche finale
    SHOTS = [("DEBUG_MASQUE", dict(DebugView=2.0)), ("PLANCHE_G3", dict(DebugView=0.0))]
if os.environ.get("EON_CALIB_PLANCHE"):   # planche finale seule (réglages par défaut du matériau)
    SHOTS = [("PLANCHE_G3", {})]
open(OUT + "/reglages.txt", "w", encoding="utf-8").write("\n".join(LOG + ["%s %s" % sh for sh in SHOTS]))
state = {"i": 0, "n": 0}


def apply(reg):
    for k2, val in reg.items():
        MEL.set_material_instance_scalar_parameter_value(trait, k2, val)
    MEL.update_material_instance(trait)


apply(SHOTS[0][1])


def tick(dt):
    state["n"] += 1
    i, n = state["i"], state["n"]
    if i >= len(SHOTS):
        if n > 60:
            unreal.unregister_slate_post_tick_callback(h)
            s2 = ppv.get_editor_property("settings")
            s2.set_editor_property("weighted_blendables", unreal.WeightedBlendables([]))
            ppv.set_editor_property("settings", s2)
            EAL.delete_directory(TMP)
            open(OUT + "/fini.txt", "w").write("ok")
            unreal.SystemLibrary.quit_editor()
        return
    f = "%s/%s.png" % (OUT, SHOTS[i][0])
    if n == (600 if i == 0 else 120):
        unreal.AutomationLibrary.take_high_res_screenshot(3840, 2160, f, cam)
    elif n > (660 if i == 0 else 180) and os.path.exists(f):
        state["i"] = i + 1; state["n"] = 0
        if i + 1 < len(SHOTS):
            apply(SHOTS[i + 1][1])


h = unreal.register_slate_post_tick_callback(tick)
