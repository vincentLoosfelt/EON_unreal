# EON — découpe porte.fbx (fusionné à l'import) en ses 3 pièces Unity : cadre, vantail gauche (PorteA), droit (PorteB).
# Le mesh a 3 îles : cadre X -109..109, vantaux X -91..0 et 0..91 (position fermée). Chaque pièce = copie de « porte »
# dont on retire les polygones des deux autres. Utilisées par AEONSlidingDoor (porte Sas d'Unity : ColoredDoor).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_porte_decoupe.py
import collections, unreal
EAL = unreal.EditorAssetLibrary
SRC = "/Game/EON/Unity/import/fbx/enviro/porte"
DIR = "/Game/EON/Unity/import/fbx/enviro/porte_pieces"
LOG = lambda s: unreal.log("EON_DECOUPE " + s)


def iles(md):
    polys = []
    for g in range(md.get_polygon_group_count()):
        polys += list(md.get_polygon_group_polygons(unreal.PolygonGroupID(g)))
    par = {}
    def f(a):
        while par.setdefault(a, a) != a:
            a = par[a]
        return a
    vs = {}
    for p in polys:
        v = [md.get_vertex_instance_vertex(x).id_value for x in md.get_polygon_vertex_instances(p)]
        vs[p.id_value] = v
        for x in v[1:]:
            par[f(x)] = f(v[0])
    il = collections.defaultdict(list)
    for p in polys:
        il[f(vs[p.id_value][0])].append(p)
    out = {}
    for ps in il.values():
        xs = [md.get_vertex_position(unreal.VertexID(v)).x for p in ps for v in vs[p.id_value]]
        lo, hi = min(xs), max(xs)
        nom = "Cadre" if hi - lo > 150 else ("VantailG" if hi < 1 else "VantailD")
        out[nom] = ps
    return out


for nom in ("Cadre", "VantailG", "VantailD"):
    dst = DIR + "/porte_" + nom
    if EAL.does_asset_exist(dst):
        EAL.delete_asset(dst)
    sm = EAL.duplicate_asset(SRC, dst)
    md = sm.get_static_mesh_description(0)
    parts = iles(md)
    n = 0
    for autre, ps in parts.items():
        if autre == nom:
            continue
        for p in ps:
            md.delete_polygon(p)
            n += 1
    sm.build_from_static_mesh_descriptions([md])
    bs = sm.get_editor_property("body_setup")
    bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    sm.set_editor_property("body_setup", bs)
    EAL.save_loaded_asset(sm)
    b = sm.get_bounding_box()
    LOG("%s : %d polygones retirés, X %.0f..%.0f Y %.0f..%.0f Z %.0f..%.0f" % (nom, n, b.min.x, b.max.x, b.min.y, b.max.y, b.min.z, b.max.z))
