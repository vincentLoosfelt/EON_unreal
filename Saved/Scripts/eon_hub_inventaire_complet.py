# EON — inventaire complet de L_HUB : tous les acteurs, triés par direction depuis le C.O.M (pour le C.O.M à quai).
import unreal
unreal.EditorLoadingAndSavingUtils.load_map("/Game/EON/Maps/L_HUB")
w = unreal.EditorLevelLibrary.get_editor_world()
acts = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor)
com = None
for a in acts:
    if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name().startswith("COMv1"):
        com = a.get_actor_location()
L = ["COM %s" % com]
rows = []
for a in acts:
    cls = a.get_class().get_name()
    mesh = ""
    if isinstance(a, unreal.StaticMeshActor):
        sm = a.static_mesh_component.static_mesh
        mesh = sm.get_name() if sm else ""
    o, e = a.get_actor_bounds(False)
    loc = a.get_actor_location()
    rot = a.get_actor_rotation()
    sc = a.get_actor_scale3d()
    dx, dy = o.x - com.x, o.y - com.y
    rows.append((cls, a.get_name(), a.get_actor_label(), mesh, loc, rot, sc, o, e, dx, dy))
rows.sort(key=lambda r: (r[0], r[3], r[9], r[10]))
for cls, nm, lab, mesh, loc, rot, sc, o, e, dx, dy in rows:
    L.append("%-22s %-26s %-26s %-26s loc(%.0f,%.0f,%.0f) yaw %.0f sc(%.2f,%.2f,%.2f) d(%.0f,%.0f) z%.0f demi(%.0f,%.0f,%.0f)" % (
        cls, nm, lab, mesh, loc.x, loc.y, loc.z, rot.yaw, sc.x, sc.y, sc.z, dx, dy, o.z, e.x, e.y, e.z))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/hub_inventaire_complet.txt", "w", encoding="utf-8").write("\n".join(L))
