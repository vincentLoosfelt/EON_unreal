# EON — M_UnityLit devient le matériau maître toon (Substrate Toon BSDF + TP_EON_BG), mêmes paramètres :
# ATTENTION : à lancer une seule fois sur M_UnityLit d'origine ; sur un maître déjà toon, DeleteAllMaterialExpressions plante (UE 5.8). Usages : eon_material_usage.py.
# toutes les MI_Unity_* et MI_COM_* passent au toon sans être retouchées.
# Rampe = BG.shadergraph d'Unity : N·L×0,5+0,5, seuil 0,682, douceur 0,085, ombre = 0,49/1,49 de l'éclairé.
# Éclairé ×1,49 obtenu par le soleil (intensité 1,49 π lux) : le Toon BSDF ne multiplie pas par N·L.
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
P = unreal.MaterialProperty
LOG = []
DIR = "/Game/EON/Materials"

tp_path = DIR + "/TP_EON_BG"
tp = unreal.load_asset(tp_path) if EAL.does_asset_exist(tp_path) else AT.create_asset("TP_EON_BG", DIR, unreal.ToonProfile, unreal.ToonProfileFactory())
ok = unreal.EONStatics.configure_toon_profile(tp, 0.682, 0.085, unreal.LinearColor(0.49 / 1.49, 0.49 / 1.49, 0.49 / 1.49, 1.0), 0.35, True)
EAL.save_loaded_asset(tp)
LOG.append("TP_EON_BG : %s" % ok)

m = unreal.load_asset("/Game/EON/Unity/Materials/M_UnityLit")
MEL.delete_all_material_expressions(m)


def node(cls, x, y, **kw):
    n = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n


def con(a, ao, b, names):
    for nm in names:
        if MEL.connect_material_expressions(a, ao, b, nm):
            return nm
    LOG.append("ÉCHEC connexion vers %s" % names)


def tparam(name, default, x, y, sampler=None):
    n = node(unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=name, texture=unreal.load_asset(default))
    if sampler is not None:
        n.set_editor_property("sampler_type", sampler)
    return n


def vparam(name, val, x, y):
    return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(*val))


def sparam(name, val, x, y):
    return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)


def mul(a, ao, b, bo, x, y):
    n = node(unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(a, ao, n, "A"); MEL.connect_material_expressions(b, bo, n, "B")
    return n


white = "/Engine/EngineResources/WhiteSquareTexture"
black = "/Engine/EngineResources/Black"
toon = node(unreal.MaterialExpressionSubstrateToonBSDF, 0, 0, toon_profile=tp)
base = mul(tparam("BaseTex", white, -900, -400), "RGB", vparam("Tint", (1, 1, 1, 1), -900, -200), "", -600, -300)
LOG.append("base -> " + str(con(base, "", toon, ["Base Color", "BaseColor"])))
nrm = tparam("NormalTex", "/Engine/EngineMaterials/DefaultNormal", -900, 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
LOG.append("normal -> " + str(con(nrm, "RGB", toon, ["Normal"])))
# Pas de métal : BG.shadergraph d'Unity n'en a pas (tout passe par la rampe) ; un métal à 1 tuerait le diffus toon.
LOG.append("rough -> " + str(con(sparam("Roughness", 1.0, -600, 450), "", toon, ["Roughness"])))
LOG.append("spec -> " + str(con(sparam("Specular", 0.0, -600, 550), "", toon, ["Specular"])))
emi = mul(mul(tparam("EmisTex", black, -900, 650), "RGB", vparam("EmisColor", (1, 1, 1, 1), -900, 850), "", -650, 700), "", sparam("EmisStrength", 20.0, -900, 1000), "", -450, 750)
LOG.append("emis -> " + str(con(emi, "", toon, ["Emissive Color", "EmissiveColor"])))
fm = getattr(P, "MP_FRONT_MATERIAL", None)
LOG.append("front -> %s" % MEL.connect_material_property(toon, "", fm))
MEL.recompile_material(m)
m.set_editor_property("used_with_skeletal_mesh", True)   # Jira, ennemis
EAL.save_loaded_asset(m)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_toon_master.txt", "w", encoding="utf-8").write("\n".join(LOG))
