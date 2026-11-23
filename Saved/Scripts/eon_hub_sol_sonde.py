# EON — lecture seule : planchers de L_HUB (les maillages importés d'Unity n'ont pas de collision, les tracés passent au
# travers) : acteurs dont le dessus de la boîte englobante est à hauteur de passerelle, avec leur emprise X/Y.
# Sortie : Saved/Logs/eon_hub_sol.txt, puis quitte.
#   UnrealEditor.exe "D:/_PROJETS/briscard/EON_UE/EON.uproject" -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_hub_sol_sonde.py"
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []
for a in eas.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    o, e = a.get_actor_bounds(False)
    top, bot = o.z + e.z, o.z - e.z
    if 380 <= top <= 560 and e.x > 40 and e.y > 40 and e.z < 120:
        sm = a.static_mesh_component.static_mesh
        LOG.append("%-28s top=%5.0f bas=%5.0f  x %6.0f..%6.0f  y %6.0f..%6.0f  %s" % (
            a.get_actor_label()[:28], top, bot, o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y, sm.get_name() if sm else "-"))
LOG.sort(key=lambda l: l.split("x ")[1])
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_hub_sol.txt", "w", encoding="utf-8").write("\n".join(LOG))
unreal.SystemLibrary.quit_editor()
