# EON — matériau maître M_EON_BG = BG.shadergraph d'Unity, à l'identique (Vincent, 14/11 : « respecter les couleurs
# et les matériaux d'Unity, reprendre les mêmes principes »).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_bg_master.py
# 15/11 : éclairé par Lumen (Vincent : le lightmapping d'Unity devient Lumen). Couleur de base = BC × BC_1, sans
# normal map ; émission = néons seulement (EmisAdd).
# Couleurs héritées des Material Variant (unity_bg_couleurs.py). Reconstruit SUR PLACE : références gardées.
# Lu par eon_hub_bg.py, qui pose les MI_BG_* pièce par pièce comme dans le prefab Room_elevator.
import json, os, re, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Materials"
MI_DIR = "/Game/EON/Materials/BG"
HERE = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
PARAMS = json.load(open(HERE + "eon_bg_params.json", encoding="utf-8"))
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_BG " + str(m))


path = DIR + "/M_EON_BG"
# Le maître est construit sous un nom provisoire, les MI_BG_* (mises à jour SUR PLACE, références des cartes gardées)
# y sont rattachées, puis l'ancien maître, devenu libre, est supprimé et le nouveau prend son nom. (Vider un graphe
# sur place plante l'éditeur, et un maître encore référencé ne se supprime pas.)
TMP = DIR + "/M_EON_BG_neuf"
if EAL.does_asset_exist(TMP):
    EAL.delete_asset(TMP)
m = AT.create_asset("M_EON_BG_neuf", DIR, unreal.Material, unreal.MaterialFactoryNew())
# Éclairé (15/11, Vincent : « la version finale, avec Lumen ») : le lightmapping d'Unity devient Lumen.
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
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
# Couleur de base = BC × BC_1 d'Unity ; l'ombrage vient de Lumen (lumières + GI des néons), plus de rampe figée.
MEL.connect_material_property(base, "", P.MP_BASE_COLOR)
MEL.connect_material_property(sparam("Roughness", 0.85, -700, 100), "", P.MP_ROUGHNESS)
MEL.connect_material_property(const(0.0, -700, 200), "", P.MP_METALLIC)
# Pas de normal map ni de triplanaire (Vincent, 15/11 : « enlève le normal mapping ») : un vrai master paint viendra.
# Émission : seulement les néons (EmisAdd, Room light).
emi = vparam("EmisAdd", (0, 0, 0, 0), -400, 500)
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


def make_mi(name, bc1=None, tex=None, triplanar=False, trisize=None, emis=None):
    p = MI_DIR + "/" + name
    if EAL.does_asset_exist(p):
        mi = unreal.load_asset(p)
        MEL.clear_all_material_instance_parameters(mi)   # retire les anciens K / Rim de la version unlit
    else:
        mi = AT.create_asset(name, MI_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, m)
    if bc1 is not None:
        MEL.set_material_instance_vector_parameter_value(mi, "BC1", unreal.LinearColor(*bc1, 1.0))
    if tex:
        MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", tex)
    if triplanar:
        MEL.set_material_instance_static_switch_parameter_value(mi, "Triplanar", True)
        MEL.set_material_instance_scalar_parameter_value(mi, "TriSize", trisize)
    if emis:
        MEL.set_material_instance_vector_parameter_value(mi, "EmisAdd", unreal.LinearColor(*emis, 0.0))
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


for mat, p in PARAMS.items():
    name = "MI_BG_" + re.sub(r"[^A-Za-z0-9]", "_", mat)
    t = ue_texture(p.get("BC"))
    tri = bool(p["Triplanar"]) and p["Scale1"] > 0
    make_mi(name, p["BC1"], t, tri, 100.0 / p["Scale1"] if tri else None)
    log("%s : BC1=%s tex=%s tri=%s" % (name, [round(c, 3) for c in p["BC1"]], t.get_name() if t else "-", tri))

# Room light.mat (URP Lit, base noire, émission HDR (2 ; 1,09 ; 0,53)) : bandeaux lumineux des murs, au-dessus de 1 -> bloom.
make_mi("MI_BG_Room_light", bc1=(0, 0, 0), emis=(2.0, 1.0935495, 0.53459084))
# BaseWhite (URP Lit blanc, échantillon URP).
make_mi("MI_BG_BaseWhite", bc1=(1, 1, 1))
log("instances : %d" % (len(PARAMS) + 2))
if EAL.does_asset_exist(path):
    log("ancien maître supprimé : %s" % EAL.delete_asset(path))
log("nouveau maître renommé : %s" % EAL.rename_asset(TMP, path))
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_bg_master.txt", "w", encoding="utf-8").write("\n".join(LOG))
