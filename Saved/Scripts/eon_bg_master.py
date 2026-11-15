# EON — matériau maître M_EON_BG = BG.shadergraph d'Unity, à l'identique (Vincent, 14/11 : « respecter les couleurs
# et les matériaux d'Unity, reprendre les mêmes principes »).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_bg_master.py
# Le graphe BG est un Lit URP à BaseColor 0 : tout sort en émission.
#   Base   = BC_1 × BC (UV, ou triplanaire monde si _triplanar)
#   K      = lumière principale × (rampe + Toon_Ramp_Tint), passée par la trame (Blend Multiply, opacité _Strength).
#            Dans la scène Unity la lumière principale n'a pas d'ombre : la rampe vaut 1 partout, K est une constante
#            par matériau (calculée par unity_bg_couleurs.py -> eon_bg_params.json).
#   Émission = saturate(Base² × K) + saturate(Base² × K × Brightness_Rim × step(0,5, dot(N, L) × fresnel^Rim_Power))
# Lu par eon_hub_bg.py, qui pose les MI_BG_* pièce par pièce comme dans le prefab Room_elevator.
import json, os, re, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Materials"
MI_DIR = "/Game/EON/Materials/BG"
HERE = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
PARAMS = json.load(open(HERE + "eon_bg_params.json", encoding="utf-8"))
# Direction vers la lumière principale Unity (Directional Light (1)), repère Unreal (X = z, Y = x, Z = y Unity).
LIGHT_DIR = (0.14797, -0.62558, 0.76599)
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_BG " + str(m))


path = DIR + "/M_EON_BG"
if EAL.does_asset_exist(path):
    EAL.delete_asset(path)
m = AT.create_asset("M_EON_BG", DIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
for _u in ("used_with_instanced_static_meshes", "used_with_skeletal_mesh"):
    m.set_editor_property(_u, True)
P = unreal.MaterialProperty


def node(cls, x, y, **kw):
    n = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n


def con(a, ao, b, bi):
    ok = MEL.connect_material_expressions(a, ao, b, bi)
    if not ok:
        log("ÉCHEC connexion %s.%s -> %s.%s" % (a.get_name(), ao, b.get_name(), bi))
    return ok


def op(cls, a, ao, b, bo, x, y):
    n = node(cls, x, y)
    con(a, ao, n, "A"); con(b, bo, n, "B")
    return n


def vparam(name, val, x, y):
    return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(*val))


def sparam(name, val, x, y):
    return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)


def const(v, x, y):
    return node(unreal.MaterialExpressionConstant, x, y, r=v)


MUL, ADD, SUB, DOT = (unreal.MaterialExpressionMultiply, unreal.MaterialExpressionAdd,
                      unreal.MaterialExpressionSubtract, unreal.MaterialExpressionDotProduct)

tex = node(unreal.MaterialExpressionTextureObjectParameter, -1800, -300, parameter_name="BaseTex",
           texture=unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
uv = node(unreal.MaterialExpressionTextureSample, -1500, -450)
con(tex, "", uv, "Tex")
tri = node(unreal.MaterialExpressionMaterialFunctionCall, -1500, -200)
tri.set_editor_property("material_function", unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture"))
con(tex, "", tri, "TextureObject")
con(sparam("TriSize", 303.0, -1800, -100), "", tri, "TextureSize")      # Unity : _scale_1 par mètre -> 100/_scale_1 cm
sw = node(unreal.MaterialExpressionStaticSwitchParameter, -1200, -300, parameter_name="Triplanar", default_value=False)
con(tri, "XYZ Texture", sw, "True")
con(uv, "RGB", sw, "False")
base = op(MUL, sw, "", vparam("BC1", (1, 1, 1, 1), -1200, -50), "", -1000, -250)
b2 = op(MUL, base, "", base, "", -850, -250)
b2k = op(MUL, b2, "", vparam("K", (0.723, 0.444, 0.3, 1), -1000, -50), "", -700, -250)
flat = node(unreal.MaterialExpressionSaturate, -500, -250)
con(b2k, "", flat, "")

# Liseré : step(0,5, dot(N, L) × fresnel^RimPower) × RimBright × Base² × K
fres = node(unreal.MaterialExpressionFresnel, -1200, 250, base_reflect_fraction=0.0)
con(sparam("RimPower", 5.0, -1500, 300), "", fres, "ExponentIn")
ndl = op(DOT, node(unreal.MaterialExpressionVertexNormalWS, -1500, 450), "", vparam("LightDir", (*LIGHT_DIR, 0), -1500, 550), "", -1200, 450)
x = op(MUL, ndl, "", fres, "", -1000, 350)
stp = node(unreal.MaterialExpressionSaturate, -600, 350)
con(op(MUL, op(SUB, x, "", const(0.5, -1000, 500), "", -850, 400), "", const(10000.0, -850, 550), "", -700, 400), "", stp, "")
rim = node(unreal.MaterialExpressionSaturate, -300, 250)
con(op(MUL, op(MUL, b2k, "", sparam("RimBright", 2.0, -700, 150), "", -500, 150), "", stp, "", -400, 250), "", rim, "")
emi = op(ADD, op(ADD, flat, "", rim, "", -150, -100), "", vparam("EmisAdd", (0, 0, 0, 0), -400, 500), "", 0, 0)
MEL.connect_material_property(emi, "", P.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
log("M_EON_BG créé")

reg = unreal.AssetRegistryHelpers.get_asset_registry()


def ue_texture(unity_path):
    """Assets/import/textures/enviro/x.png -> /Game/EON/Unity/import/textures/enviro/x"""
    if not unity_path or "/Assets/" not in unity_path:
        return None
    rel = os.path.splitext(unity_path.split("/Assets/", 1)[1])[0]
    parts = [re.sub(r"[^A-Za-z0-9_]", "_", p) for p in rel.split("/")]
    folder, name = "/Game/EON/Unity/" + "/".join(parts[:-1]), parts[-1]
    for ad in reg.get_assets_by_path(folder, recursive=False):
        if str(ad.asset_name) == name:
            return ad.get_asset()
    log("texture introuvable : %s/%s" % (folder, name))
    return None


def make_mi(name, bc1=None, K=None, tex=None, triplanar=False, trisize=None, rim=(5.0, 2.0), emis=None):
    p = MI_DIR + "/" + name
    if EAL.does_asset_exist(p):
        EAL.delete_asset(p)
    mi = AT.create_asset(name, MI_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, m)
    if bc1 is not None:
        MEL.set_material_instance_vector_parameter_value(mi, "BC1", unreal.LinearColor(*bc1, 1.0))
    if K is not None:
        MEL.set_material_instance_vector_parameter_value(mi, "K", unreal.LinearColor(*K, 1.0))
    if tex:
        MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", tex)
    if triplanar:
        MEL.set_material_instance_static_switch_parameter_value(mi, "Triplanar", True)
        MEL.set_material_instance_scalar_parameter_value(mi, "TriSize", trisize)
    MEL.set_material_instance_scalar_parameter_value(mi, "RimPower", rim[0])
    MEL.set_material_instance_scalar_parameter_value(mi, "RimBright", rim[1])
    if emis:
        MEL.set_material_instance_vector_parameter_value(mi, "EmisAdd", unreal.LinearColor(*emis, 0.0))
    EAL.save_loaded_asset(mi)
    return mi


for mat, p in PARAMS.items():
    name = "MI_BG_" + re.sub(r"[^A-Za-z0-9]", "_", mat)
    t = ue_texture(p.get("BC"))
    tri = bool(p["Triplanar"]) and p["Scale1"] > 0
    make_mi(name, p["BC1"], p["K"], t, tri, 100.0 / p["Scale1"] if tri else None, (p["RimPower"], p["RimBright"]))
    log("%s : K=%s BC1=%s tex=%s tri=%s rim=%s/%s" % (name, [round(c, 3) for c in p["K"]], [round(c, 3) for c in p["BC1"]],
                                                    t.get_name() if t else "-", tri, p["RimPower"], p["RimBright"]))

# Room light.mat (URP Lit, base noire, émission HDR (2 ; 1,09 ; 0,53)) : bandeaux lumineux des murs, au-dessus de 1 -> bloom.
make_mi("MI_BG_Room_light", bc1=(0, 0, 0), emis=(2.0, 1.0935495, 0.53459084))
# BaseWhite (URP Lit blanc, échantillon URP) : dans Unity, éclairé par ambiance grise 0,855 + deux directionnelles.
# Ici en aplat : ambiance linéaire 0,70 + moitié des directionnelles en moyenne -> blanc cassé chaud.
make_mi("MI_BG_BaseWhite", bc1=(1, 1, 1), K=(0.98, 0.9, 0.85), rim=(5.0, 0.0))
log("instances : %d" % (len(PARAMS) + 2))
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_bg_master.txt", "w", encoding="utf-8").write("\n".join(LOG))
