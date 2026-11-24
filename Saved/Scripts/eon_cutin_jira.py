# EON — JIR-V23 (24/11) : rendu provisoire de Jira pour le cut-in « Protocole thoracique » (Vincent : « rendu de Jira
# provisoire », remplaçable plus tard). Banc du HUB à Z=6000 (comme la planche), SceneCapture HDR en « show only » :
# l'alpha de la capture donne le détourage exact. Deux sorties brutes (linéaires) : saine, et veines à fond (contrôle JIR-V24).
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_cutin_jira.py"
import os, json, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/CutIn"
os.makedirs(OUT, exist_ok=True)
for c in ("t.IdleWhenNotForeground 0", "r.Streaming.FullyLoadUsedTextures 1", "r.TextureStreaming 0"):
    unreal.SystemLibrary.execute_console_command(None, c)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
BZ = 6000.0
m = unreal.load_asset("/Game/EON/Characters/Jira/SK_Jira_UEFN")
bd = m.get_bounds()
k = 181.25 / max(bd.box_extent.z * 2, 1.0)
z = BZ - (bd.origin.z - bd.box_extent.z) * k
YAW = float(os.environ.get("EON_CI_YAW", "28"))
a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, z), unreal.Rotator(0, 0, YAW))
c = a.skeletal_mesh_component
c.set_skinned_asset_and_update(m)
seq = unreal.load_asset("/Game/EON/Characters/Jira/Anims/UEFN/" + os.environ.get("EON_CI_ANIM", "A_Jira_charged_attack"))
c.set_update_animation_in_editor(True)
c.set_editor_property("visibility_based_anim_tick_option", unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
T = float(os.environ.get("EON_CI_T", "0.45"))
c.override_animation_data(seq, False, False, seq.get_play_length() * T, 0.0)
c.set_position(seq.get_play_length() * T, False)
a.set_actor_scale3d(unreal.Vector(k, k, k))
mids = [c.create_dynamic_material_instance(i) for i in range(c.get_num_materials())]
soleils = [x for x in eas.get_all_level_actors() if isinstance(x, unreal.DirectionalLight)]
if soleils:
    s = max(soleils, key=lambda x: x.light_component.get_editor_property("intensity"))
    s.set_actor_rotation(unreal.Rotator(0.0, -40.0, -55.0), False)
D = 420.0
cap = eas.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, D, BZ + 100.0), unreal.Rotator(0, -4.0, -90))
cc = cap.capture_component2d
cc.set_editor_property("fov_angle", 30.0)
cc.set_editor_property("capture_every_frame", False)
cc.set_editor_property("capture_on_movement", False)
cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_SCENE_COLOR_HDR)
cc.set_editor_property("primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
cc.show_only_actor_components(a, False)
N = 1024
rt = unreal.RenderingLibrary.create_render_target2d(a, N, N, unreal.TextureRenderTargetFormat.RTF_RGBA16F)
cc.set_editor_property("texture_target", rt)
st = {"n": 0}


def dump(name):
    cc.capture_scene()
    px = unreal.RenderingLibrary.read_render_target_raw(a, rt, False)
    with open(OUT + "/" + name + ".json", "w") as f:
        json.dump([[round(p.r, 4), round(p.g, 4), round(p.b, 4), round(p.a, 3)] for p in px], f)


def tick(dt):
    st["n"] += 1
    n = st["n"]
    if n == 400:
        dump("saine")
        for md in mids:
            md.set_scalar_parameter_value("Veines", 1.0)
    elif n == 460:
        dump("veines")
        open(OUT + "/fini.txt", "w").write("ok")
        unreal.unregister_slate_post_tick_callback(h)
        unreal.SystemLibrary.quit_editor()


for f in ("fini.txt",):
    if os.path.exists(OUT + "/" + f):
        os.remove(OUT + "/" + f)
h = unreal.register_slate_post_tick_callback(tick)
