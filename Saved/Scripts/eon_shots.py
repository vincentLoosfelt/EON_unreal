# EON — captures de contrôle d'une carte depuis l'éditeur (lancé avec -ExecutePythonScript).
# Variables d'environnement : EON_SHOT_MAP (carte), EON_SHOT_DIR (dossier de sortie).
import os, math, unreal

MAP = os.environ.get("EON_SHOT_MAP", "/Game/EON/Maps/L_HUB")
OUT = os.environ.get("EON_SHOT_DIR", "D:/_PROJETS/briscard/EON_UE/Saved/Shots")
os.makedirs(OUT, exist_ok=True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level(MAP)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Emprise des meshes
lo, hi = [1e9] * 3, [-1e9] * 3
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor):
        o, e = a.get_actor_bounds(False)
        for i, c in enumerate("xyz"):
            lo[i] = min(lo[i], getattr(o, c) - getattr(e, c)); hi[i] = max(hi[i], getattr(o, c) + getattr(e, c))
c = unreal.Vector((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2)
span = max(hi[0] - lo[0], hi[1] - lo[1])
unreal.log("EON_SHOT emprise %s .. %s" % (lo, hi))
open(OUT + "/emprise.txt", "w").write("%s\n%s\n" % (lo, hi))

views = [("dessus", -89.0, 0.0, 1.1), ("plongee_sud", -55.0, 0.0, 0.9), ("plongee_est", -55.0, 90.0, 0.9), ("rasant", -25.0, 45.0, 0.7)]
cams = []
for name, pitch, yaw, k in views:
    d = span * k / math.tan(math.radians(25))
    p, y = math.radians(-pitch), math.radians(yaw)
    loc = unreal.Vector(c.x - d * math.cos(p) * math.cos(y), c.y - d * math.cos(p) * math.sin(y), c.z + d * math.sin(p))
    cam = eas.spawn_actor_from_class(unreal.CameraActor, loc, unreal.Rotator(0, pitch, yaw))
    cam.camera_component.set_editor_property("field_of_view", 50.0)
    cams.append((name, cam))

state = {"i": 0, "wait": 0}


def tick(dt):
    state["wait"] += 1
    if state["wait"] < 90:      # laisse Lumen et le streaming se poser
        return
    if state["i"] < len(cams):
        name, cam = cams[state["i"]]
        unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, "%s/%s.png" % (OUT, name), cam)
        state["i"] += 1; state["wait"] = 60
    elif state["wait"] > 200:
        unreal.unregister_slate_post_tick_callback(handle)
        open(OUT + "/fini.txt", "w").write("ok")


handle = unreal.register_slate_post_tick_callback(tick)
