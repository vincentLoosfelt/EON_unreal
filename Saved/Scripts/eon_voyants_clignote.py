# EON — voyants qui varient d'intensité en signal carré (Vincent, 25/11).
# M_EON_Voyant : émission = EmissiveColor × (bas ou haut selon un créneau), DiffuseColor en couleur de base.
# Chaque voyant a sa phase et sa vitesse, tirées de sa position (cases de 25 cm) : ils ne clignotent pas ensemble.
# Les 7 matériaux Light_green / Light_red importés d'Unity sont reparentés sur M_EON_Voyant : mêmes noms de paramètres
# (EmissiveColor, DiffuseColor), donc les maillages ne changent pas. Intensité de base : eon_emissifs_voyants.py (1,54).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_voyants_clignote.py
import unreal
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
P = unreal.MaterialProperty
MDIR, MNAME = "/Game/EON/Materials", "M_EON_Voyant"
M = MDIR + "/" + MNAME
LOG = lambda s: unreal.log("EON_VOYANT " + s)

if EAL.does_asset_exist(M):
    EAL.delete_asset(M)   # delete_all_material_expressions plante en commandlet ; les instances sont reparentées plus bas
m = AT.create_asset(MNAME, MDIR, unreal.Material, unreal.MaterialFactoryNew())


def node(cls, y, **kw):
    e = MEL.create_material_expression(m, cls, -800, y)
    for k, v in kw.items():
        e.set_editor_property(k, v)
    return e


def scal(n, v, y):
    return node(unreal.MaterialExpressionScalarParameter, y, parameter_name=n, default_value=v, group="Voyant")


emis = node(unreal.MaterialExpressionVectorParameter, -300, parameter_name="EmissiveColor", default_value=unreal.LinearColor(0, 1.54, 0, 1), group="Voyant")
diff = node(unreal.MaterialExpressionVectorParameter, -200, parameter_name="DiffuseColor", default_value=unreal.LinearColor(0, 1, 0, 1), group="Voyant")
rate = scal("Cadence", 1.2, -100)        # créneaux par seconde (moyenne)
spread = scal("Ecart cadence", 0.6, -50)  # ± fraction de cadence d'un voyant à l'autre
duty = scal("Rapport cyclique", 0.5, 0)   # part du temps à l'état haut
low = scal("Intensite basse", 0.25, 50)   # état bas, en fraction de l'état haut
tim = node(unreal.MaterialExpressionTime, 150)
wp = node(unreal.MaterialExpressionWorldPosition, 200)
cu = node(unreal.MaterialExpressionCustom, 300)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cu.set_editor_property("description", "Creneau voyant")
ins = []
for n in ("T", "Pos", "Rate", "Spread", "Duty", "Low"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
cu.set_editor_property("inputs", ins)
cu.set_editor_property("code", """
float3 c = floor(Pos / 25.0);
float h1 = frac(sin(dot(c, float3(12.9898, 78.233, 37.719))) * 43758.5453);
float h2 = frac(sin(dot(c, float3(39.346, 11.135, 83.155))) * 24634.6345);
float f = Rate * (1.0 + Spread * (h2 * 2.0 - 1.0));
float s = step(frac(T * f + h1), Duty);
return lerp(Low, 1.0, s);
""")
for src, pin in ((tim, "T"), (wp, "Pos"), (rate, "Rate"), (spread, "Spread"), (duty, "Duty"), (low, "Low")):
    MEL.connect_material_expressions(src, "", cu, pin)
mul = node(unreal.MaterialExpressionMultiply, 400)
MEL.connect_material_expressions(emis, "", mul, "A")
MEL.connect_material_expressions(cu, "", mul, "B")
MEL.connect_material_property(mul, "", P.MP_EMISSIVE_COLOR)
MEL.connect_material_property(diff, "", P.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionConstant, 500, r=0.5)
MEL.connect_material_property(rough, "", P.MP_ROUGHNESS)
MEL.recompile_material(m)
EAL.save_asset(M, only_if_is_dirty=False)
LOG("M_EON_Voyant compilé")

for a in EAL.list_assets("/Game/EON/Unity/import/fbx", True, False):
    p = a.split(".")[0]
    n = p.rsplit("/", 1)[-1].lower()
    if n not in ("light_green", "light_red"):
        continue
    mi = unreal.load_asset(p)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        continue
    em = MEL.get_material_instance_vector_parameter_value(mi, "EmissiveColor")
    di = MEL.get_material_instance_vector_parameter_value(mi, "DiffuseColor")
    MEL.set_material_instance_parent(mi, m)
    MEL.set_material_instance_vector_parameter_value(mi, "EmissiveColor", em)
    MEL.set_material_instance_vector_parameter_value(mi, "DiffuseColor", di)
    MEL.update_material_instance(mi)
    EAL.save_asset(p, only_if_is_dirty=False)
    LOG("%s -> M_EON_Voyant (émission %.2f,%.2f,%.2f)" % (p, em.r, em.g, em.b))
