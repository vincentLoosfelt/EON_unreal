# EON — dôme hexagonal de la COM (nœuds bubble.0xx de COMv1.fbx, section 5 sans matériau FBX).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_com_dome.py
# Reprend Glass.shadergraph + Glass.mat d'Unity :
#   transparent additif, base (0.358, 0.005, 0.005), Fresnel orange (1, 0.547, 0) puissance 5.28 × 1.26,
#   sommets poussés le long de la normale par un bruit (UV0 défilant à la vitesse 1, échelle 5) × -0.005
#   en unités objet Unity (× 100 du FBX × 1,5 du prefab ≈ -75 cm).
#   15/11 : bruit lu en UV1 = centre de chaque hexagone (eon_dome_hexa.py) -> hexagones plats, déplacés d'un bloc.
#   La réfraction (Scene Color) du graphe n'est branchée sur aucune sortie dans Unity : non reprise.
# Collision : coupée sur le dôme ; le reste de la COM en collision exacte (complexe = simple).
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
DIR = "/Game/EON/Materials"
NAME = "M_EON_ChampForce"
P = unreal.MaterialProperty
LOG = []

if EAL.does_asset_exist(DIR + "/" + NAME):
    EAL.delete_asset(DIR + "/" + NAME)
m = AT.create_asset(NAME, DIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", False)


def node(cls, x, y, **props):
    n = MEL.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def sp(name, val, x, y):
    return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)


def vp(name, rgb, x, y):
    return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(*rgb, 1.0))


def op(cls, a, ao, b, bo, x, y):
    n = node(cls, x, y)
    MEL.connect_material_expressions(a, ao, n, "A")
    MEL.connect_material_expressions(b, bo, n, "B")
    return n


# --- Couleur : base × gain + Fresnel
base = vp("Base", (0.358, 0.005, 0.005), -900, -300)
gain = sp("BaseGain", 1.0, -900, -150)
fres = node(unreal.MaterialExpressionFresnel, -900, 0)
MEL.connect_material_expressions(sp("FresnelPower", 5.28, -1150, 0), "", fres, "ExponentIn")
fcol = vp("FresnelColor", (1.0, 0.547, 0.0), -900, 150)
fint = sp("FresnelIntensity", 1.26, -900, 300)
glow = sp("Glow", 0.17, -600, 350)   # = alpha 0,17 du Glass.mat : en additif Unity, couleur × alpha
rim = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, fres, "", fint, "", -650, 50), "", fcol, "", -450, 100)
col = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, base, "", gain, "", -650, -250), "", rim, "", -300, -50)
MEL.connect_material_property(op(unreal.MaterialExpressionMultiply, col, "", glow, "", -150, 0), "", P.MP_EMISSIVE_COLOR)

# --- Mouvement des hexagones : normale × bruit(UV0 + temps × vitesse) × amplitude
uv = node(unreal.MaterialExpressionTextureCoordinate, -1300, 600, coordinate_index=1)   # UV1 = centre d'hexagone (eon_dome_hexa.py)
t = node(unreal.MaterialExpressionTime, -1300, 750)
spd = op(unreal.MaterialExpressionMultiply, t, "", sp("HexaVitesse", 0.05, -1300, 850), "", -1100, 750)
uvt = op(unreal.MaterialExpressionAdd, uv, "", spd, "", -950, 650)
pos3 = node(unreal.MaterialExpressionAppendVector, -800, 650)
MEL.connect_material_expressions(uvt, "", pos3, "A")
MEL.connect_material_expressions(node(unreal.MaterialExpressionConstant, -950, 780, r=0.0), "", pos3, "B")
noise = node(unreal.MaterialExpressionNoise, -620, 650, scale=5.0, output_min=0.0, output_max=1.0, quality=2)
MEL.connect_material_expressions(pos3, "", noise, "World Position")   # 15/11 : « Position » n'existe pas -> Noise débranché
nrm = node(unreal.MaterialExpressionVertexNormalWS, -620, 850)
amp = sp("DisplaceCm", -20.0, -620, 950)
wpo = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, noise, "", amp, "", -420, 700), "", nrm, "", -250, 800)
MEL.connect_material_property(wpo, "", P.MP_WORLD_POSITION_OFFSET)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
LOG.append(NAME + " créé")

# --- COMv1 : section 5 = dôme
sm = unreal.load_asset("/Game/EON/Unity/import/fbx/enviro/COMv1")
mats = list(sm.get_editor_property("static_materials"))
dome = None
for i, s in enumerate(mats):
    mi = s.get_editor_property("material_interface")
    if mi is None or mi.get_name() == "WorldGridMaterial":
        dome = i
if dome is None:
    dome = len(mats) - 1
mats[dome].set_editor_property("material_interface", m)
mats[dome].set_editor_property("material_slot_name", "Dome_Hexagones")
sm.set_editor_property("static_materials", mats)
ok = unreal.EONStatics.set_mesh_section_collision(sm, 0, dome, False, True)
EAL.save_loaded_asset(sm)
LOG.append("COMv1 : emplacement %d = dôme, collision coupée : %s" % (dome, ok))
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_com_dome.txt", "w", encoding="utf-8").write("\n".join(LOG))
