# EON — carte L_Etage (jalons 2–3) : un AEONFloorManager au centre (il génère l'étage et reconstruit la salle à chaque porte),
# un départ, la lumière et le post-process du HUB (scène Unity : deux directionnelles, ambiance grise, bloom, sans tonemapping).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_etage_map.py   (après eon_bg_master.py)
import math, unreal
MAP = "/Game/EON/Maps/L_Etage"
LOG = []
EAL = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if EAL.does_asset_exist(MAP):
    EAL.delete_asset(MAP)
les.new_level(MAP)


def qrot(q, v):
    x, y, z, w = q

    def m(a, b):
        ax, ay, az, aw = a; bx, by, bz, bw = b
        return [aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz]
    return m(m(q, [v[0], v[1], v[2], 0]), [-x, -y, -z, w])[:3]


fm = eas.spawn_actor_from_class(unreal.EONFloorManager, unreal.Vector(0, 0, 0))
fm.set_actor_label("Etage")
eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 300, 120)).set_actor_label("Depart_Jira")

for i, (label, q, col, inten, shadows) in enumerate([
        ("Soleil_principal", (0.2622, 0.7108, -0.3314, 0.5623), (1.0, 0.8058, 0.6761), 0.6, False),
        ("Soleil_2", (0.1094, -0.8754, 0.4082, 0.2346), (1.0, 1.0, 1.0), 0.56, True)]):
    f = qrot(q, [0, 0, 1])
    rot = unreal.MathLibrary.make_rot_from_x(unreal.Vector(f[2], f[0], f[1]))
    L = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1500), rot)
    lc = L.light_component
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property("intensity", inten * math.pi)
    r, g, b = [int(round(255 * c)) for c in col]
    lc.set_editor_property("light_color", unreal.Color(r=r, g=g, b=b, a=255))
    lc.set_editor_property("cast_shadows", shadows)
    L.set_actor_label(label)
sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_editor_property("intensity", 1.0)

ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
ppv.set_editor_property("unbound", True)
s = ppv.get_editor_property("settings")
s.set_editor_property("override_auto_exposure_method", True)
s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
s.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
s.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
for k, v in (("auto_exposure_bias", math.log2(1.2)), ("bloom_threshold", 1.0), ("bloom_intensity", 1.0),
             ("tone_curve_amount", 0.0), ("expand_gamut", 0.0), ("blue_correction", 0.0)):
    try:
        s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
    except Exception as e:
        LOG.append("PPV %s : %s" % (k, e))
# Post-process validé le 24/11 (Vincent) : décor aquarelle B, trait des celluloïds, film ; bloom 6 / 0,8 / 8, sans motion blur.
# (L'ancien filtre M_PP_EON_Peinture est supprimé.)
for k, v in (("bloom_intensity", 6.0), ("bloom_threshold", 0.8), ("bloom_size_scale", 8.0), ("motion_blur_amount", 0.0)):
    s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
s.set_editor_property("weighted_blendables", unreal.WeightedBlendables([unreal.WeightedBlendable(1.0, unreal.load_asset(p))
    for p in ("/Game/EON/Materials/Peint/MI_PP_EON_Decor_B", "/Game/EON/Materials/MI_PP_EON_Trait", "/Game/EON/Materials/MI_PP_EON_Film")]))
ppv.set_editor_property("settings", s)
ppv.set_actor_label("PP_Etage")

ws = unreal.EditorLevelLibrary.get_editor_world().get_world_settings()
ws.set_editor_property("default_game_mode", unreal.load_asset("/Game/EON/Blueprints/BP_EONGameMode").generated_class())
LOG.append("carte enregistrée : %s" % les.save_current_level())
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_etage_map.txt", "w", encoding="utf-8").write("\n".join(LOG))
