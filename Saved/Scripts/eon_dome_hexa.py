# EON — dôme de la COM : chaque hexagone bouge d'un bloc, reste plat (Vincent, 15/11 : « garder chaque petit hexagone flat »).
# Les 187 hexagones du dôme sont des îles séparées, mais chaque sommet a sa propre UV0 : le bruit lu en UV0
# tordait les hexagones (bosses). On écrit en UV1 le centre UV de chaque île -> même bruit pour tous ses sommets.
# Puis M_EON_ChampForce lit le bruit en UV1 (sur place), défilant dans le temps (animé).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_dome_hexa.py
import collections, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
SMES = unreal.StaticMeshEditorSubsystem()
LOG = lambda s: unreal.log("EON_DOME " + s)
DISPLACE, SPEED, SCALE = -20.0, 0.05, 5.0   # cm le long de la normale ; UV/s ; cellules de bruit par UV

sm = unreal.load_asset("/Game/EON/Unity/import/fbx/enviro/COMv1")
dome = [i for i, s in enumerate(sm.static_materials) if str(s.material_slot_name) == "Dome_Hexagones"][0]
# pas d'UV de lightmap générée par-dessus (Lumen), sinon elle écraserait l'UV1
bs = SMES.get_lod_build_settings(sm, 0)
bs.set_editor_property("generate_lightmap_u_vs", False)
SMES.set_lod_build_settings(sm, 0, bs)
DEJA = SMES.get_num_uv_channels(sm, 0) >= 2
if not DEJA:
    SMES.add_uv_channel(sm, 0)
md = sm.get_static_mesh_description(0)
polys = md.get_polygon_group_polygons(unreal.PolygonGroupID(dome))
par = {}
def f(a):
    while par.setdefault(a, a) != a:
        a = par[a]
    return a
for p in polys:
    vs = [md.get_vertex_instance_vertex(v).id_value for v in md.get_polygon_vertex_instances(p)]
    for v in vs[1:]:
        par[f(v)] = f(vs[0])
acc = collections.defaultdict(lambda: [0.0, 0.0, 0])
for p in polys:
    for vi in md.get_polygon_vertex_instances(p):
        u = md.get_vertex_instance_uv(vi, 0)
        a = acc[f(md.get_vertex_instance_vertex(vi).id_value)]
        a[0] += u.x; a[1] += u.y; a[2] += 1
n = 0
for p in polys:
    for vi in md.get_polygon_vertex_instances(p):
        a = acc[f(md.get_vertex_instance_vertex(vi).id_value)]
        md.set_vertex_instance_uv(vi, unreal.Vector2D(a[0] / a[2], a[1] / a[2]), 1)
        n += 1
sm.build_from_static_mesh_descriptions([md])
ok = unreal.EONStatics.set_mesh_section_collision(sm, 0, dome, False, True)
EAL.save_loaded_asset(sm)
LOG("UV1 = centre d'île : %d hexagones, %d sommets, canaux UV %d, collision dôme coupée %s" % (len(acc), n, SMES.get_num_uv_channels(sm, 0), ok))

# --- Matériau, sur place : l'entrée Position du Noise n'était branchée sur rien (bruit en position monde : figé et
# différent à chaque sommet -> bosses). On la branche : UV1 (centre d'hexagone) + temps × vitesse.
m = unreal.load_asset("/Game/EON/Materials/M_EON_ChampForce")
vu, pile, noise = set(), [MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)], None
while pile:
    e = pile.pop()
    if e is None or e.get_name() in vu:
        continue
    vu.add(e.get_name())
    if isinstance(e, unreal.MaterialExpressionNoise):
        noise = e
    elif isinstance(e, unreal.MaterialExpressionScalarParameter) and str(e.get_editor_property("parameter_name")) == "DisplaceCm":
        e.set_editor_property("default_value", DISPLACE)
    pile += [i for i in MEL.get_inputs_for_material_expression(m, e) if i]
def node(cls, x, y, **kw):
    n = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n
x, y = noise.get_editor_property("material_expression_editor_x"), noise.get_editor_property("material_expression_editor_y")
uv = node(unreal.MaterialExpressionTextureCoordinate, x - 900, y, coordinate_index=1)
t = node(unreal.MaterialExpressionTime, x - 900, y + 120)
spd = node(unreal.MaterialExpressionScalarParameter, x - 900, y + 220, parameter_name="HexaVitesse", default_value=SPEED)
mul = node(unreal.MaterialExpressionMultiply, x - 700, y + 150)
add = node(unreal.MaterialExpressionAdd, x - 500, y + 50)
app = node(unreal.MaterialExpressionAppendVector, x - 300, y + 50)
zero = node(unreal.MaterialExpressionConstant, x - 500, y + 180, r=0.0)
liens = [MEL.connect_material_expressions(t, "", mul, "A"), MEL.connect_material_expressions(spd, "", mul, "B"),
         MEL.connect_material_expressions(uv, "", add, "A"), MEL.connect_material_expressions(mul, "", add, "B"),
         MEL.connect_material_expressions(add, "", app, "A"), MEL.connect_material_expressions(zero, "", app, "B"),
         MEL.connect_material_expressions(app, "", noise, "World Position")]   # nom réel de l'entrée (pas « Position »)
noise.set_editor_property("scale", SCALE)
MEL.delete_unused_expressions(m)   # anciens nœuds débranchés (essais ratés)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
LOG("Noise rebranché : liens %s, entrées du Noise %s" % (liens, [i.get_class().get_name() if i else None for i in MEL.get_inputs_for_material_expression(m, noise)]))
