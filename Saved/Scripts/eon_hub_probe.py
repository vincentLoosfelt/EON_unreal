# Sonde : acteurs proches du PlayerStart du HUB (emprises), portes fermées.
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
out = []
ps = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PlayerStart)][0].get_actor_location()
out.append("PlayerStart %s" % ps)
for a in eas.get_all_level_actors():
    o, e = a.get_actor_bounds(False)
    if abs(o.x - ps.x) < 900 + e.x and abs(o.y - ps.y) < 900 + e.y and abs(o.z - ps.z) < 400 + e.z and e.x < 5000:
        out.append("%-30s c=(%.0f,%.0f,%.0f) e=(%.0f,%.0f,%.0f) %s" % (a.get_actor_label()[:30], o.x, o.y, o.z, e.x, e.y, e.z, a.get_class().get_name()))
open(unreal.Paths.project_saved_dir() + "Logs/eon_hub_probe.txt", "w", encoding="utf-8").write("\n".join(out))
