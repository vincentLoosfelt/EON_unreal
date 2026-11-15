# EON — relève les acteurs de L_HUB (classe, maillage, position, emprise) pour brancher le hub comme salle centrale de l'étage.
import unreal
unreal.EditorLoadingAndSavingUtils.load_map("/Game/EON/Maps/L_HUB")
w = unreal.EditorLevelLibrary.get_editor_world()
L = []
from collections import Counter
cnt = Counter()
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    cls = a.get_class().get_name()
    mesh = ""
    if isinstance(a, unreal.StaticMeshActor):
        sm = a.static_mesh_component.static_mesh
        mesh = sm.get_name() if sm else ""
    cnt[(cls, mesh)] += 1
    o, e = a.get_actor_bounds(False)
    loc = a.get_actor_location()
    if cls.startswith("EON") or cls.startswith("BP_") or "Start" in cls or "Volume" in cls or any(k in mesh for k in ("COM", "Passerelle", "Porte", "porte", "Bridge", "Grille", "Rampe", "Floor", "Sol", "Tile")) or "Bounds" in cls:
        L.append("%-28s %-28s %-30s loc(%.0f,%.0f,%.0f) centre(%.0f,%.0f,%.0f) demi(%.0f,%.0f,%.0f)" % (cls, a.get_actor_label(), mesh, loc.x, loc.y, loc.z, o.x, o.y, o.z, e.x, e.y, e.z))
L.append("\n--- comptes ---")
for (c, m), n in cnt.most_common(60):
    L.append("%4d %s %s" % (n, c, m))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/hub_inventaire.txt", "w", encoding="utf-8").write("\n".join(L))
