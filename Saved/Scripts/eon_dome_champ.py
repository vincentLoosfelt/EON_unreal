# EON — dôme du C.O.M en champ de force (Vincent, 15/11 : « Fresnel additif lumineux », « grille active orangée
# avec un peu d'animation », toujours plus ou moins transparent).
# 1) Maillage : chaque hexagone (4 triangles, 6 sommets) devient un éventail autour d'un sommet central.
#    UV2.x = 0 au centre, 1 sur le bord : interpolé, c'est exactement la distance « hexagonale » au bord -> grille nette.
#    Le sommet central n'a pas de normale (l'API Python ne l'écrit pas) : le matériau prend la normale de la sphère
#    (position - centre de l'objet), pour le Fresnel comme pour le déplacement.
# 2) M_EON_ChampForce reconstruit sur place (les références restent) :
#    émission = Fresnel orange + grille (bords) × (base + onde qui monte + éclairs par hexagone) + léger remplissage + teinte de fond.
#    Déplacement d'un bloc par hexagone (UV1 = centre, eon_dome_hexa.py) conservé.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_dome_champ.py
import collections, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
SMES = unreal.StaticMeshEditorSubsystem()
LOG = lambda s: unreal.log("EON_CHAMP " + s)
P = unreal.MaterialProperty

# --- 1) Maillage
sm = unreal.load_asset("/Game/EON/Unity/import/fbx/enviro/COMv1")
dome = [i for i, s in enumerate(sm.static_materials) if str(s.material_slot_name) == "Dome_Hexagones"][0]
if SMES.get_num_uv_channels(sm, 0) < 3:
    SMES.add_uv_channel(sm, 0)
    md = sm.get_static_mesh_description(0)
    pg = unreal.PolygonGroupID(dome)
    polys = list(md.get_polygon_group_polygons(pg))
    tri = {}
    for p in polys:
        vis = list(md.get_polygon_vertex_instances(p))
        tri[p.id_value] = (p, vis, [md.get_vertex_instance_vertex(v).id_value for v in vis])
    par = {}
    def f(a):
        while par.setdefault(a, a) != a:
            a = par[a]
        return a
    for _, _, vs in tri.values():
        for v in vs[1:]:
            par[f(v)] = f(vs[0])
    iles = collections.defaultdict(list)
    for k, (_, _, vs) in tri.items():
        iles[f(vs[0])].append(k)
    n_ok = 0
    for ks in iles.values():
        # arêtes de bord orientées (une seule occurrence), avec l'instance de sommet de chaque extrémité
        cpt = collections.Counter()
        for k in ks:
            vs = tri[k][2]
            for i in range(3):
                cpt[frozenset((vs[i], vs[(i + 1) % 3]))] += 1
        bords = []
        inst = {}
        for k in ks:
            _, vis, vs = tri[k]
            for i in range(3):
                inst[vs[i]] = vis[i]
                if cpt[frozenset((vs[i], vs[(i + 1) % 3]))] == 1:
                    bords.append((vis[i], vis[(i + 1) % 3]))
        sommets = list(inst.keys())
        pts = [md.get_vertex_position(unreal.VertexID(v)) for v in sommets]
        c = unreal.Vector(sum(q.x for q in pts) / len(pts), sum(q.y for q in pts) / len(pts), sum(q.z for q in pts) / len(pts))
        u0 = [md.get_vertex_instance_uv(inst[v], 0) for v in sommets]
        u1 = md.get_vertex_instance_uv(inst[sommets[0]], 1)
        cv = md.create_vertex()
        md.set_vertex_position(cv, c)
        ci = md.create_vertex_instance(cv)
        md.set_vertex_instance_uv(ci, unreal.Vector2D(sum(u.x for u in u0) / len(u0), sum(u.y for u in u0) / len(u0)), 0)
        md.set_vertex_instance_uv(ci, u1, 1)
        md.set_vertex_instance_uv(ci, unreal.Vector2D(0.0, 0.0), 2)
        for v in sommets:
            md.set_vertex_instance_uv(inst[v], unreal.Vector2D(1.0, 0.0), 2)
        for k in ks:
            md.delete_polygon(tri[k][0])
        for a, b in bords:
            md.create_triangle(pg, [a, b, ci])
        n_ok += 1
    sm.build_from_static_mesh_descriptions([md])
    unreal.EONStatics.set_mesh_section_collision(sm, 0, dome, False, True)
    EAL.save_loaded_asset(sm)
    LOG("maillage : %d hexagones en éventail, canaux UV %d" % (n_ok, SMES.get_num_uv_channels(sm, 0)))
else:
    LOG("maillage déjà en éventail (UV2 présent)")

# --- 1b) MPC_EON_Champ : écrit par AEONChampForce (Now, Jira, Impact0..3 = x, y, z, instant)
MPC = "/Game/EON/Materials/MPC_EON_Champ"
if not EAL.does_asset_exist(MPC):
    mpc = unreal.AssetToolsHelpers.get_asset_tools().create_asset("MPC_EON_Champ", "/Game/EON/Materials", unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    sc = unreal.CollectionScalarParameter(); sc.set_editor_property("parameter_name", "Now"); sc.set_editor_property("default_value", 0.0)
    vs = []
    for nm, dv in [("Jira", (0, 0, -100000, 0))] + [("Impact%d" % i, (0, 0, -100000, -1000)) for i in range(4)]:
        v = unreal.CollectionVectorParameter(); v.set_editor_property("parameter_name", nm); v.set_editor_property("default_value", unreal.LinearColor(*dv))
        vs.append(v)
    mpc.set_editor_property("scalar_parameters", [sc])
    mpc.set_editor_property("vector_parameters", vs)
    EAL.save_loaded_asset(mpc)
    LOG("MPC_EON_Champ créé")
mpc = unreal.load_asset(MPC)

# --- 2) Matériau
m = unreal.load_asset("/Game/EON/Materials/M_EON_ChampForce")
MEL.delete_all_material_expressions(m)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", False)


def node(cls, x, y, **kw):
    n = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n


def sp(name, val, x, y):
    return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)


def vp(name, rgb, x, y):
    return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(*rgb, 1.0))


def op(cls, a, b, x, y, ao="", bo=""):
    n = node(cls, x, y)
    MEL.connect_material_expressions(a, ao, n, "A")
    MEL.connect_material_expressions(b, bo, n, "B")
    return n


def un(cls, a, x, y, inp="", ao=""):
    n = node(cls, x, y)
    MEL.connect_material_expressions(a, ao, n, inp)
    return n


MUL, ADD, SUB = unreal.MaterialExpressionMultiply, unreal.MaterialExpressionAdd, unreal.MaterialExpressionSubtract

# Normale de la sphère : position monde - centre de l'objet
wp = node(unreal.MaterialExpressionWorldPosition, -2400, 0)
op_ws = node(unreal.MaterialExpressionObjectPositionWS, -2400, 120)
nrm = un(unreal.MaterialExpressionNormalize, op(SUB, wp, op_ws, -2200, 50), -2000, 50)
t = node(unreal.MaterialExpressionTime, -2400, 400)

# Fresnel orange additif
fres = node(unreal.MaterialExpressionFresnel, -1700, -300)
MEL.connect_material_expressions(sp("FresnelPower", 5.0, -1950, -350), "", fres, "ExponentIn")
MEL.connect_material_expressions(nrm, "", fres, "Normal")
rim = op(MUL, op(MUL, fres, sp("FresnelIntensity", 0.5, -1700, -150), -1500, -300), vp("FresnelColor", (1.0, 0.42, 0.04), -1500, -150), -1300, -300)

# Grille : UV2.x^GridPower (0 au centre, 1 sur le bord)
uv2 = node(unreal.MaterialExpressionTextureCoordinate, -2000, 300, coordinate_index=2)
d = node(unreal.MaterialExpressionComponentMask, -1850, 300, r=True, g=False, b=False, a=False)
MEL.connect_material_expressions(uv2, "", d, "")
grille = node(unreal.MaterialExpressionPower, -1650, 300)
MEL.connect_material_expressions(d, "", grille, "Base")
MEL.connect_material_expressions(sp("GridPower", 16.0, -1850, 420), "", grille, "Exp")

# Onde qui monte : frac(z / longueur - t × vitesse)^6
z = node(unreal.MaterialExpressionComponentMask, -2200, 600, r=False, g=False, b=True, a=False)
MEL.connect_material_expressions(wp, "", z, "")
zl = op(unreal.MaterialExpressionDivide, z, sp("WaveLength", 500.0, -2200, 720), -2000, 600)
ts = op(MUL, t, sp("WaveSpeed", 0.35, -2200, 820), -2000, 760)
onde = node(unreal.MaterialExpressionPower, -1650, 650)
MEL.connect_material_expressions(un(unreal.MaterialExpressionFrac, op(SUB, zl, ts, -1850, 650), -1750, 650), "", onde, "Base")
MEL.connect_material_expressions(sp("WaveSharp", 6.0, -1850, 780), "", onde, "Exp")

# Éclairs par hexagone : bruit sur UV1 (centre) défilant, seuillé
uv1 = node(unreal.MaterialExpressionTextureCoordinate, -2400, 1000, coordinate_index=1)
p3 = node(unreal.MaterialExpressionAppendVector, -2000, 1000)
MEL.connect_material_expressions(uv1, "", p3, "A")
MEL.connect_material_expressions(op(MUL, t, sp("FlashSpeed", 0.6, -2400, 1150), -2200, 1100), "", p3, "B")
nf = node(unreal.MaterialExpressionNoise, -1850, 1000, scale=40.0, output_min=0.0, output_max=1.0, quality=1)
MEL.connect_material_expressions(p3, "", nf, "World Position")
eclair = un(unreal.MaterialExpressionSaturate, op(MUL, op(SUB, nf, sp("FlashSeuil", 0.62, -1850, 1150), -1650, 1000), node(unreal.MaterialExpressionConstant, -1650, 1120, r=4.0), -1500, 1000), -1350, 1000)

# Réactions (AEONChampForce) : ondes d'impact (4 au plus) et proximité de Jira, en HLSL
def cp(name, x, y):
    return node(unreal.MaterialExpressionCollectionParameter, x, y, collection=mpc, parameter_name=name)
cu = node(unreal.MaterialExpressionCustom, -1650, 1350, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT2, description="EON_Reactions")
cu.set_editor_property("code", """float r = 0;
float4 I[4] = { I0, I1, I2, I3 };
for (int k = 0; k < 4; k++)
{
  float age = Now - I[k].w;
  if (age >= 0 && age < Dur)
  {
    float d = distance(WP, I[k].xyz);
    float f = 1 - age / Dur;
    float ring = exp(-pow((d - age * Speed) / Width, 2));
    float spot = exp(-d * d / (Width * Width * 6)) * saturate(1 - age * 3);
    r += (ring + 2 * spot) * f * f;
  }
}
float p = saturate(1 - distance(WP, Jira) / Prox);
return float2(r, p * p);""")
noms = ["WP", "Now", "I0", "I1", "I2", "I3", "Jira", "Speed", "Width", "Dur", "Prox"]
ins = []
for nm in noms:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
cu.set_editor_property("inputs", ins)
srcs = [wp, cp("Now", -2000, 1300)] + [cp("Impact%d" % i, -2000, 1380 + 80 * i) for i in range(4)] + [cp("Jira", -2000, 1700),
        sp("OndeVitesse", 900.0, -2000, 1780), sp("OndeLargeur", 90.0, -2000, 1860), sp("OndeDuree", 1.6, -2000, 1940), sp("JiraPortee", 450.0, -2000, 2020)]
for nm, s in zip(noms, srcs):
    MEL.connect_material_expressions(s, "", cu, nm)
reac_i = node(unreal.MaterialExpressionComponentMask, -1450, 1350, r=True, g=False, b=False, a=False)
MEL.connect_material_expressions(cu, "", reac_i, "")
reac_j = node(unreal.MaterialExpressionComponentMask, -1450, 1450, r=False, g=True, b=False, a=False)
MEL.connect_material_expressions(cu, "", reac_j, "")
reac = op(ADD, op(MUL, reac_i, sp("ImpactForce", 4.0, -1450, 1550), -1300, 1400), op(MUL, reac_j, sp("JiraForce", 2.0, -1450, 1650), -1300, 1500), -1150, 1450)

# Grille × (base + onde × 2 + éclair × 3) ; remplissage léger = onde × 0,15 + éclair × 0,35
anim = op(ADD, op(ADD, sp("GridBase", 0.35, -1300, 500), op(MUL, onde, node(unreal.MaterialExpressionConstant, -1500, 750, r=2.0), -1350, 650), -1150, 550),
          op(ADD, op(MUL, eclair, node(unreal.MaterialExpressionConstant, -1350, 1150, r=3.0), -1200, 1050), reac, -1100, 1200), -1000, 700)
fill = op(ADD, op(ADD, op(MUL, onde, sp("FillOnde", 0.06, -1300, 850), -1150, 850), op(MUL, eclair, sp("FillEclair", 0.15, -1300, 1250), -1150, 1200), -1000, 950),
          op(MUL, reac, sp("FillReaction", 0.12, -1150, 1300), -1000, 1300), -900, 1100)
gcol = op(MUL, op(MUL, op(ADD, op(MUL, grille, anim, -850, 400), fill, -700, 600), vp("GridColor", (1.0, 0.38, 0.03), -700, 750), -550, 600),
          sp("GridIntensity", 0.5, -700, 880), -400, 600)
fond = op(MUL, vp("Base", (0.358, 0.005, 0.005), -700, -500), sp("BaseGain", 0.07, -700, -350), -500, -450)
emi = op(MUL, op(ADD, op(ADD, rim, gcol, -250, 0), fond, -100, -100), sp("Glow", 0.7, -100, 100), 50, 0)
MEL.connect_material_property(emi, "", P.MP_EMISSIVE_COLOR)

# Déplacement d'un bloc par hexagone : bruit(UV1 + t × HexaVitesse) × DisplaceCm le long de la normale de la sphère
p3d = node(unreal.MaterialExpressionAppendVector, -1000, 1500)
MEL.connect_material_expressions(op(ADD, uv1, op(MUL, t, sp("HexaVitesse", 0.05, -1400, 1600), -1200, 1550), -1100, 1500), "", p3d, "A")
MEL.connect_material_expressions(node(unreal.MaterialExpressionConstant, -1100, 1650, r=0.0), "", p3d, "B")
nd = node(unreal.MaterialExpressionNoise, -850, 1500, scale=5.0, output_min=0.0, output_max=1.0, quality=2)
MEL.connect_material_expressions(p3d, "", nd, "World Position")
wpo = op(MUL, op(MUL, nd, sp("DisplaceCm", -20.0, -850, 1650), -650, 1550), nrm, -450, 1550)
MEL.connect_material_property(wpo, "", P.MP_WORLD_POSITION_OFFSET)
MEL.delete_unused_expressions(m)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
LOG("M_EON_ChampForce reconstruit : %d expressions" % len(MEL.get_material_expressions(m) if hasattr(MEL, "get_material_expressions") else []))
