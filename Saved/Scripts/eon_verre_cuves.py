# EON — verre des cuves, calé sur le shader Unity Glass.shadergraph (Assets/materials/shaders) et ses matériaux
# Glass 1 (cuves, vert foncé) et Glass 2 (cuve du boss, vert clair émissif).
#
# Repris d'Unity : base, emissive, fresnel color / effect / intensity, transparency, smoothness, texture de déplacement
# (Pigment01) lue deux fois, UV + t et UV + (1 − t), t = temps × vitesse ; la somme donne une normale (Normal From Height).
# Dans Unity, cette normale décalait la Scene Color, mais le résultat n'était branché nulle part (pas de réfraction visible)
# et transparency > 1 rendait le verre opaque. Ici la normale pilote la vraie réfraction d'Unreal (Pixel Normal Offset)
# et la transparence est baissée pour qu'on voie au travers (demande de Vincent, 25/11).
# Verre lisse (Vincent, 25/11 : « pas de trame ») : normale large, lente et faible, smoothness 0,85 au lieu de −0,74.
# Le déplacement de sommets d'Unity (displacement intensity) vaut 0 sur Glass 1 et Glass 2 : non repris.
#
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_verre_cuves.py
import unreal

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
P = unreal.MaterialProperty
MDIR = "/Game/EON/Materials/Cuves"
M = MDIR + "/M_EON_Glass"
TEX = unreal.load_asset("/Game/EON/Unity/import/textures/Shaders/Pigment01")
LOG = lambda s: unreal.log("EON_VERRE " + s)

if EAL.does_asset_exist(M):
    # delete_all_material_expressions plante le commandlet (!IsRooted) : on refait l'asset ; les instances sont reparentées plus bas.
    EAL.delete_asset(M)
m = AT.create_asset("M_EON_Glass", MDIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
m.set_editor_property("refraction_method", unreal.RefractionMode.RM_PIXEL_NORMAL_OFFSET)
m.set_editor_property("two_sided", False)

X = [0]


def node(cls, y, **props):
    X[0] += 1
    e = MEL.create_material_expression(m, cls, -900, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def scal(name, v, y, grp="Unity"):
    return node(unreal.MaterialExpressionScalarParameter, y, parameter_name=name, default_value=v, group=grp)


def vec(name, c, y, grp="Unity"):
    return node(unreal.MaterialExpressionVectorParameter, y, parameter_name=name, default_value=unreal.LinearColor(*c), group=grp)


base = vec("base", (0.037941, 0.377359, 0.013053, 1), -600)
emis = vec("emissive", (0, 0, 0, 0), -500)
fcol = vec("fresnel color", (0.404485, 1, 0, 1), -400)
feff = scal("fresnel effect", 5.28, -300)
fint = scal("fresnel intensity", 1.26, -250)
trans = scal("transparency", 0.3, -200)
smooth = scal("smoothness", 0.85, -150)
speed = scal("refraction displacement speed", 0.08, -100)
tiling = scal("refraction tiling", 0.3, -50, "Unreal")
force = scal("refraction normal strength", 0.06, 0, "Unreal")
ior = scal("refraction", 1.15, 50, "Unreal")
boost = scal("emission boost", 1.0, 100, "Unreal")
tex = node(unreal.MaterialExpressionTextureObjectParameter, 150, parameter_name="refraction displacement texture", texture=TEX, group="Unity")
uv = node(unreal.MaterialExpressionTextureCoordinate, 250)
tim = node(unreal.MaterialExpressionTime, 300)

# Normale « Normal From Height » de la somme des deux lectures qui défilent en sens contraires (comme Unity).
cust = node(unreal.MaterialExpressionCustom, 400)
cust.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cust.set_editor_property("description", "Normale verre (Unity Glass)")
names = ["Tex", "UV", "T", "Tiling", "Force"]
ins = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
cust.set_editor_property("inputs", ins)
cust.set_editor_property("code", """
float2 o = T.xx;
float2 p = UV * Tiling;
float e = 1.0 / 256.0;
float h0 = Texture2DSample(Tex, TexSampler, p + o).r + Texture2DSample(Tex, TexSampler, p + 1.0 - o).r;
float hx = Texture2DSample(Tex, TexSampler, p + float2(e, 0) + o).r + Texture2DSample(Tex, TexSampler, p + float2(e, 0) + 1.0 - o).r;
float hy = Texture2DSample(Tex, TexSampler, p + float2(0, e) + o).r + Texture2DSample(Tex, TexSampler, p + float2(0, e) + 1.0 - o).r;
return normalize(float3((h0 - hx) * Force, (h0 - hy) * Force, 1.0));
""")
t_mul = node(unreal.MaterialExpressionMultiply, 300)
MEL.connect_material_expressions(tim, "", t_mul, "A")
MEL.connect_material_expressions(speed, "", t_mul, "B")
MEL.connect_material_expressions(tex, "", cust, "Tex")
MEL.connect_material_expressions(uv, "", cust, "UV")
MEL.connect_material_expressions(t_mul, "", cust, "T")
MEL.connect_material_expressions(tiling, "", cust, "Tiling")
MEL.connect_material_expressions(force, "", cust, "Force")
MEL.connect_material_property(cust, "", P.MP_NORMAL)
MEL.connect_material_property(ior, "", P.MP_REFRACTION)

# Bulles qui remontent dans le liquide (Vincent, 25/11). Pas de particules : chaque pixel du verre prolonge le rayon de vue
# dans le cylindre (axe vertical au centre des bounds de l'objet) et le recoupe avec 3 couches internes (22, 44, 66 % du rayon),
# faces avant et arrière. Sur chaque couche, des colonnes de bulles montent à vitesse variable, avec un léger zigzag.
bul = scal("bulles", 1.0, 900, "Bulles")
bcell = scal("bulles taille cm", 8.0, 950, "Bulles")
bspeed = scal("bulles vitesse cm/s", 14.0, 1000, "Bulles")
bdens = scal("bulles densite", 0.3, 1050, "Bulles")
bcol = vec("bulles couleur", (0.5, 1.0, 0.55, 1), 1100, "Bulles")
wpos = node(unreal.MaterialExpressionWorldPosition, 1150)
opos = node(unreal.MaterialExpressionObjectPositionWS, 1200)
cpos = node(unreal.MaterialExpressionCameraPositionWS, 1250)
bc = node(unreal.MaterialExpressionCustom, 1300)
bc.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
bc.set_editor_property("description", "Bulles dans le cylindre")
bins = []
for n in ("P", "C", "Cam", "T", "Cell", "Speed", "Density"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    bins.append(ci)
bc.set_editor_property("inputs", bins)
bc.set_editor_property("code", """
float3 V = normalize(P - Cam);
float2 O = P.xy - C.xy;
float R = length(O);
float a = dot(V.xy, V.xy);
float acc = 0.0;
if (a > 1e-4)
{
    for (int k = 1; k <= 3; k++)
    {
        float rk = R * 0.22 * k;
        float b = dot(O, V.xy);
        float c = dot(O, O) - rk * rk;
        float disc = b * b - a * c;
        if (disc <= 0.0) continue;
        float sq = sqrt(disc);
        for (int sd = 0; sd < 2; sd++)
        {
            float t = (-b + (sd == 0 ? -sq : sq)) / a;
            if (t <= 0.0) continue;
            float3 H = P + t * V;
            float arc = atan2(H.y - C.y, H.x - C.x) * rk;
            float col = floor(arc / Cell);
            float hc = frac(sin(col * 12.9898 + k * 78.233) * 43758.5453);
            float zz = H.z - T * Speed * (0.6 + 0.8 * hc) + hc * 100.0;
            float row = floor(zz / Cell);
            float h = frac(sin(dot(float2(col, row) + k * 7.0, float2(39.346, 11.135))) * 24634.6345);
            if (h > Density) continue;
            float2 cc = float2(frac(arc / Cell), frac(zz / Cell)) - 0.5;
            cc.x += 0.12 * sin(T * 3.0 + h * 40.0 + zz * 0.15);
            float rad = 0.1 + 0.12 * frac(h * 17.0);
            float d = length(cc) / rad;
            float m = (1.0 - smoothstep(0.75, 1.0, d)) * (0.35 + 0.65 * smoothstep(0.3, 0.9, d));
            acc += m * (sd == 0 ? 1.0 : 0.5);
        }
    }
}
return saturate(acc);
""")
for src, pin in ((wpos, "P"), (opos, "C"), (cpos, "Cam"), (tim, "T"), (bcell, "Cell"), (bspeed, "Speed"), (bdens, "Density")):
    MEL.connect_material_expressions(src, "", bc, pin)
bm = node(unreal.MaterialExpressionMultiply, 1350)   # masque × intensité
MEL.connect_material_expressions(bc, "", bm, "A")
MEL.connect_material_expressions(bul, "", bm, "B")
be = node(unreal.MaterialExpressionMultiply, 1400)   # × couleur -> émission
MEL.connect_material_expressions(bm, "", be, "A")
MEL.connect_material_expressions(bcol, "", be, "B")

# Emission = fresnel^effect × intensity × fresnel color + emissive (Unity), × boost.
fr = node(unreal.MaterialExpressionFresnel, 500, base_reflect_fraction=0.0)
MEL.connect_material_expressions(feff, "", fr, "ExponentIn")
f1 = node(unreal.MaterialExpressionMultiply, 550)
MEL.connect_material_expressions(fr, "", f1, "A")
MEL.connect_material_expressions(fint, "", f1, "B")
f2 = node(unreal.MaterialExpressionMultiply, 600)
MEL.connect_material_expressions(f1, "", f2, "A")
MEL.connect_material_expressions(fcol, "", f2, "B")
f3 = node(unreal.MaterialExpressionAdd, 650)
MEL.connect_material_expressions(f2, "", f3, "A")
MEL.connect_material_expressions(emis, "", f3, "B")
f4 = node(unreal.MaterialExpressionMultiply, 700)
MEL.connect_material_expressions(f3, "", f4, "A")
MEL.connect_material_expressions(boost, "", f4, "B")
f5 = node(unreal.MaterialExpressionAdd, 720)
MEL.connect_material_expressions(f4, "", f5, "A")
MEL.connect_material_expressions(be, "", f5, "B")
MEL.connect_material_property(f5, "", P.MP_EMISSIVE_COLOR)
MEL.connect_material_property(base, "", P.MP_BASE_COLOR)

# Opacité = transparency + bulles (bornée), rugosité = 1 − smoothness (bornée).
oa = node(unreal.MaterialExpressionAdd, 740)
MEL.connect_material_expressions(trans, "", oa, "A")
MEL.connect_material_expressions(bm, "", oa, "B")
sat = node(unreal.MaterialExpressionSaturate, 750)
MEL.connect_material_expressions(oa, "", sat, "")
MEL.connect_material_property(sat, "", P.MP_OPACITY)
sm = node(unreal.MaterialExpressionSaturate, 800)
MEL.connect_material_expressions(smooth, "", sm, "")
om = node(unreal.MaterialExpressionOneMinus, 850)
MEL.connect_material_expressions(sm, "", om, "")
MEL.connect_material_property(om, "", P.MP_ROUGHNESS)
MEL.recompile_material(m)
EAL.save_asset(M, only_if_is_dirty=False)
LOG("M_EON_Glass compilé")

# Instances : Glass 1 (cuves) et Glass 2 (cuve du boss), valeurs d'Unity sauf transparency.
# Glass 1 (Vincent, 25/11) : verre vert « un peu opaque, émet légèrement, liquide dedans » -> opacité 0,6 (on voit l'intérieur), émission verte faible, reflets adoucis, bulles qui remontent.
INST = {
    "MI_EON_Glass_1": {"base": (0.037941024, 0.3773585, 0.013053237, 1), "emissive": (0.02, 0.22, 0.04, 0), "fresnel color": (0.40448475, 1, 0, 1), "transparency": 0.5, "smoothness": 0.6, "bulles": 0.7, "bulles taille cm": 16.0, "bulles densite": 0.14},
    "MI_EON_Glass_2": {"base": (0.40968397, 1, 0.36163503, 1), "emissive": (0.42138356, 0.38295546, 0.38295546, 0), "fresnel color": (0.40448475, 1, 0, 1), "transparency": 0.22, "bulles": 0.0},
}
for name, vals in INST.items():
    p = MDIR + "/" + name
    mi = unreal.load_asset(p) if EAL.does_asset_exist(p) else AT.create_asset(name, MDIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    for k, v in vals.items():
        if isinstance(v, tuple):
            MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(*v))
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_asset(p, only_if_is_dirty=False)
    LOG(name + " enregistré")

# Slots de verre des cuves.
G1 = unreal.load_asset(MDIR + "/MI_EON_Glass_1")
G2 = unreal.load_asset(MDIR + "/MI_EON_Glass_2")
CIBLES = [("/Game/EON/Unity/import/fbx/Cuve_1", G1), ("/Game/EON/Unity/import/fbx/Cuve_2", G1), ("/Game/EON/Unity/import/fbx/Cuve_3", G1),
          ("/Game/EON/Unity/import/fbx/Cuve_4", G1), ("/Game/EON/Unity/import/fbx/Cuve_5", G1), ("/Game/EON/Unity/import/fbx/boss/cuve", G2),
          ("/Game/EON/Unity/import/fbx/Cuve_Reveil/Cuve_Reveil_Corps", G1), ("/Game/EON/Unity/import/fbx/Cuve_Reveil/Cuve_Reveil_Verre", G1)]
for path, mat in CIBLES:
    sm = unreal.load_asset(path)
    n = 0
    for i, s in enumerate(sm.get_editor_property("static_materials")):
        if str(s.material_slot_name).lower() == "glass":
            sm.set_material(i, mat)
            n += 1
    EAL.save_asset(path, only_if_is_dirty=False)
    LOG("%s : %d slot(s) de verre -> %s" % (path.rsplit("/", 1)[-1], n, mat.get_name()))
