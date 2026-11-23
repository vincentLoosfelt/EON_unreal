# EON — test des textures floues/pixelisées du décor (Vincent, 23/11) : même plan du HUB rendu deux fois, streaming
# normal puis streaming coupé (toutes les textures à pleine résolution). Rien n'est enregistré.
# Sortie : Saved/Shots/Mips/<nom>.png + Saved/Logs/eon_mips_test.txt (matériaux des rambardes, stats de streaming).
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_mips_test.py"
import math, os, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Mips"
os.makedirs(OUT, exist_ok=True)
SE = unreal.SystemLibrary.execute_console_command
SE(None, "t.IdleWhenNotForeground 0")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []
# Qui utilise yellow_black_strips ? (matériau, échantillonnage)
AR = unreal.AssetRegistryHelpers.get_asset_registry()
for r in AR.get_referencers("/Game/EON/Unity/import/textures/enviro/yellow_black_strips", unreal.AssetRegistryDependencyOptions()) or []:
    LOG.append("strips référencée par %s" % r)
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor):
        c = a.static_mesh_component
        for i, m in enumerate(c.get_materials()):
            if m and "strip" in m.get_path_name().lower() or (m and "jaune" in m.get_path_name().lower()):
                LOG.append("%s | %s | slot %d | %s | échelle %s" % (a.get_actor_label(), c.static_mesh.get_path_name() if c.static_mesh else "-", i,
                                                               m.get_path_name(), a.get_actor_scale3d()))
P = math.radians(40.0)
T = unreal.Vector(900.0, 50.0, 554.0)
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(T.x, T.y + 1000 * math.cos(P), T.z + 1000 * math.sin(P)), unreal.Rotator(0, -40.0, -90))
cam.camera_component.set_editor_property("field_of_view", 25.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
SHOTS = [("A_streaming_normal", []), ("B_streaming_coupe", ["r.TextureStreaming 0"])]
state = {"i": 0, "n": 0}


def tick(dt):
    state["n"] += 1
    i, n = state["i"], state["n"]
    if i >= len(SHOTS):
        if n > 60:
            unreal.unregister_slate_post_tick_callback(h)
            open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_mips_test.txt", "w", encoding="utf-8").write("\n".join(LOG))
            open(OUT + "/fini.txt", "w").write("ok")
            unreal.SystemLibrary.quit_editor()
        return
    f = "%s/%s.png" % (OUT, SHOTS[i][0])
    if n == 2:
        for c in SHOTS[i][1]:
            SE(None, c)
    if n == (600 if i == 0 else 240):
        SE(None, "stat streaming")
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, f, cam)
    elif n > (660 if i == 0 else 300) and os.path.exists(f):
        state["i"] = i + 1; state["n"] = 0


h = unreal.register_slate_post_tick_callback(tick)
