# EON — onde de choc (Vincent, 25/11 ; façon Akira) : M_PP_EON_OndeChoc, post-process après le tonemapper.
# Une sphère de distorsion centrée sur Centre (UV de l'écran), de rayon Rayon (en hauteurs d'écran) : l'image est
# réfractée comme à travers une bulle, avec un taux lié à un fresnel (nul au centre, fort sur le bord de la sphère).
# Force règle la déviation, Fresnel l'exposant. Emission ajoute Teinte selon le même fresnel (bord de la sphère
# légèrement lumineux, pour matérialiser la distance : pouvoir E de Jira) ; 0 par défaut. Posé par AEONPlayerCameraManager pour chaque EONFx::OndeChoc en cours.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
L = []

P = "/Game/EON/Materials/M_PP_EON_OndeChoc"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
g = AT.create_asset("M_PP_EON_OndeChoc", "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())
g.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
g.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
g.set_editor_property("blendable_priority", -1)

def param(cls, nom, val, y):
    e = MEL.create_material_expression(g, cls, -1100, y)
    e.set_editor_property("parameter_name", nom)
    e.set_editor_property("default_value", val)
    return e
centre = param(unreal.MaterialExpressionVectorParameter, "Centre", unreal.LinearColor(0.5, 0.5, 0, 0), 0)
rayon = param(unreal.MaterialExpressionScalarParameter, "Rayon", 0.3, 150)
force = param(unreal.MaterialExpressionScalarParameter, "Force", 0.06, 250)
fres = param(unreal.MaterialExpressionScalarParameter, "Fresnel", 3.0, 350)
teinte = param(unreal.MaterialExpressionVectorParameter, "Teinte", unreal.LinearColor(1, 1, 1, 0), 450)
emis = param(unreal.MaterialExpressionScalarParameter, "Emission", 0.0, 550)
sp = MEL.create_material_expression(g, unreal.MaterialExpressionScreenPosition, -1100, -200)
vs = MEL.create_material_expression(g, unreal.MaterialExpressionViewSize, -1100, -300)

cu = MEL.create_material_expression(g, unreal.MaterialExpressionCustom, -650, 0)
cu.set_editor_property("code", """float k = VS.x / VS.y;
float2 d = (UV - Centre.xy) * float2(k, 1);
float r = length(d) / max(Rayon, 1e-4);
if (r >= 1) return UV;
float nz = sqrt(1 - r * r);
float f = pow(1 - nz, Fr);
float2 n = d / max(length(d), 1e-5);
float2 off = n * f * Force;
off.x /= k;
return UV - off;""")
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
cu.set_editor_property("description", "Sphere fresnel")
ins = []
for nm in ("UV", "Centre", "Rayon", "Force", "Fr", "VS"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
cu.set_editor_property("inputs", ins)
st = MEL.create_material_expression(g, unreal.MaterialExpressionSceneTexture, -300, 0)
st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)

def lien(a, sa, b, sb):
    L.append("%s>%s %s" % (sa, sb, MEL.connect_material_expressions(a, sa, b, sb)))
lien(sp, "ViewportUV", cu, "UV")
lien(centre, "", cu, "Centre")
lien(rayon, "", cu, "Rayon")
lien(force, "", cu, "Force")
lien(fres, "", cu, "Fr")
lien(vs, "", cu, "VS")
lien(cu, "", st, "UVs")
# émission fresnel par-dessus l'image déformée
ce = MEL.create_material_expression(g, unreal.MaterialExpressionCustom, 0, 0)
ce.set_editor_property("code", """float k = VS.x / VS.y;
float2 d = (UV - Centre.xy) * float2(k, 1);
float r = length(d) / max(Rayon, 1e-4);
float f = r >= 1 ? 0 : pow(1 - sqrt(1 - r * r), Fr);
return C.rgb + Teinte.rgb * f * Emission;""")
ce.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ce.set_editor_property("description", "Emission fresnel")
ins = []
for nm in ("C", "UV", "Centre", "Rayon", "Fr", "VS", "Teinte", "Emission"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
ce.set_editor_property("inputs", ins)
lien(st, "Color", ce, "C")
lien(sp, "ViewportUV", ce, "UV")
lien(centre, "", ce, "Centre")
lien(rayon, "", ce, "Rayon")
lien(fres, "", ce, "Fr")
lien(vs, "", ce, "VS")
lien(teinte, "", ce, "Teinte")
lien(emis, "", ce, "Emission")
L.append("sortie %s" % MEL.connect_material_property(ce, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
MEL.recompile_material(g)
L.append("onde %s" % EAL.save_loaded_asset(g))
for l in L:
    unreal.log("EON_ONDE " + l)
