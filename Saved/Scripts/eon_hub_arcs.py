# EON — arcs électriques du HUB (Vincent, 15/11 : « des arcs électriques qui relient les lumières bleues du C.O.M et
# les montures aux quatre coins », champ magnétique qui fait circuler le HUB verticalement).
# 1) M_EON_Arc : additif, non éclairé, double face ; émission = couleur des sommets × alpha × 4 × Gain
#    (AEONElectricArc range la teinte en RGB et l'intensité HDR / 4 dans l'alpha).
# 2) L_HUB : un AEONElectricArc par arc Unity (hub_arcs.json, extrait par unity_arcs.py), anciens arcs retirés.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_arcs.py
import json, os, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
HERE = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
MAT = "/Game/EON/Materials/M_EON_Arc"
GAIN = 1.0
LOG = lambda s: unreal.log("EON_ARCS " + s)

# --- 1) Matériau (créé une fois ; le gain se règle ensuite sur place)
if not EAL.does_asset_exist(MAT):
    m = AT.create_asset("M_EON_Arc", "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -800, 0)
    a4 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, 100)
    a4.set_editor_property("const_b", 4.0)
    MEL.connect_material_expressions(vc, "A", a4, "A")
    g = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 250)
    g.set_editor_property("parameter_name", "Gain")
    g.set_editor_property("default_value", GAIN)
    k = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 150)
    MEL.connect_material_expressions(a4, "", k, "A")
    MEL.connect_material_expressions(g, "", k, "B")
    e = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 0)
    MEL.connect_material_expressions(vc, "", e, "A")
    MEL.connect_material_expressions(k, "", e, "B")
    MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    LOG("M_EON_Arc créé")

# --- 1b) Instance plus lumineuse (Vincent, 15/11 : « un peu plus lumineux »)
MI = "/Game/EON/Materials/MI_EON_Arc"
GAIN_MI = 4.0
mi = unreal.load_asset(MI) if EAL.does_asset_exist(MI) else AT.create_asset("MI_EON_Arc", "/Game/EON/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mi.set_editor_property("parent", unreal.load_asset(MAT))
MEL.set_material_instance_scalar_parameter_value(mi, "Gain", GAIN_MI)
EAL.save_loaded_asset(mi)

# --- 2) Arcs dans L_HUB
ARCS = json.load(open(HERE + "hub_arcs.json", encoding="utf-8"))
ue = lambda p: unreal.Vector(p[2] * 100.0, p[0] * 100.0, p[1] * 100.0)
unreal.EditorLoadingAndSavingUtils.load_map("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(a, unreal.EONElectricArc) or a.get_actor_label().startswith("Arc_"):
        eas.destroy_actor(a)
# Surfaces émissives : îles des emplacements « electro » (glissières Rampe_electro) et « light » (C.O.M).
# Les extrémités se promènent sur les sommets de l'île la plus proche et de ses voisines (< 300 cm).
import collections
ILES = []   # (centre, [points])
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor): continue
    sm = a.static_mesh_component.static_mesh
    if sm is None or sm.get_name() not in ("Rampe_electro", "COMv1"): continue
    slot = "electro" if sm.get_name() == "Rampe_electro" else "light"
    idx = [i for i, sl in enumerate(sm.static_materials) if str(sl.material_slot_name) == slot][0]
    md = sm.get_static_mesh_description(0)
    tf = a.get_actor_transform()
    polys = md.get_polygon_group_polygons(unreal.PolygonGroupID(idx))
    par = {}
    def f(x):
        while par.setdefault(x, x) != x: x = par[x]
        return x
    vs = {}
    for p in polys:
        v = [md.get_vertex_instance_vertex(x).id_value for x in md.get_polygon_vertex_instances(p)]
        vs[p.id_value] = v
        for x in v[1:]: par[f(x)] = f(v[0])
    il = collections.defaultdict(set)
    for p in polys:
        for v in vs[p.id_value]: il[f(vs[p.id_value][0])].add(v)
    for vv in il.values():
        pts = [tf.transform_location(md.get_vertex_position(unreal.VertexID(v))) for v in vv]
        c = unreal.Vector(sum(q.x for q in pts) / len(pts), sum(q.y for q in pts) / len(pts), sum(q.z for q in pts) / len(pts))
        ILES.append((c, pts))
LOG("%d îles émissives" % len(ILES))
def candidats(p):
    proche = min(ILES, key=lambda i: min((q - p).length() for q in i[1]))
    pts = []
    for c, q in ILES:
        if (c - proche[0]).length() < 300.0: pts += q
    uniq = {}
    for q in pts: uniq[(round(q.x), round(q.y), round(q.z))] = q
    return list(uniq.values())

for d in ARCS:
    s, t = ue(d["startPoint"]), ue(d["endPoint"])
    act = eas.spawn_actor_from_class(unreal.EONElectricArc, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    act.set_editor_property("start_point", s)
    act.set_editor_property("end_point", t)
    act.set_editor_property("start_radius", d["startPointRadius"] * 100.0)
    act.set_editor_property("end_radius", d["endPointRadius"] * 100.0)
    act.set_editor_property("material", mi)
    cs, ce = candidats(s), candidats(t)
    act.set_editor_property("start_candidates", cs)
    act.set_editor_property("end_candidates", ce)
    LOG("  %d points de départ, %d d'arrivée" % (len(cs), len(ce)))
    act.set_actor_label("Arc_" + d["name"].replace(" ", "_").replace("(", "").replace(")", ""))
    act.set_folder_path("HUB/Arcs")
    LOG("%s : %s -> %s" % (d["name"], [round(v) for v in (s.x, s.y, s.z)], [round(v) for v in (t.x, t.y, t.z)]))
unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), "/Game/EON/Maps/L_HUB")
LOG("%d arcs posés" % len(ARCS))
