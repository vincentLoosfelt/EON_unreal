# EON — import des planches d'effets (SourceAssets/VFX_Planches) et matériau M_EON_Planche (Vincent, 25/11).
#   Textures : /Game/EON/VFX/Planches/T_EON_VFX_<nom> (alpha gardé, pas de streaming : planches toujours nettes).
#   M_EON_Planche : non éclairé, translucide après DOF (bords nets), double face. Joue une image d'une planche Cols × Rows (paramètre Image, 0 en
#     haut à gauche, ligne par ligne). Émissif = texture × Color × Intensite (les planches de sang sont en niveaux de gris :
#     blanc = ton vif, gris = ton sombre ; Color = couleur du monstre). Opacité = alpha × Opacite seuillé à 0,5.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
L = []
SRC = os.path.join(unreal.Paths.project_dir(), "SourceAssets", "VFX_Planches")
DST = "/Game/EON/VFX/Planches"
# nom de la texture -> fichier source (P1 : planche ChatGPT détourée de son fond magenta, grille 4 × 2 de 512)
PLANCHES = {"P1_Eclats": "EON_VFX_P1_Eclats_alpha.png", "P2_Giclee": "EON_VFX_P2_Giclee.png", "P3_TachesSol": "EON_VFX_P3_TachesSol.png"}

tasks = []
for n in PLANCHES:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(SRC, PLANCHES[n]))
    t.set_editor_property("destination_path", DST)
    t.set_editor_property("destination_name", "T_EON_VFX_%s" % n)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    tasks.append(t)
AT.import_asset_tasks(tasks)
for n in PLANCHES:
    p = "%s/T_EON_VFX_%s" % (DST, n)
    tx = unreal.load_asset(p)
    if not tx:
        L.append("ECHEC import " + p); continue
    tx.set_editor_property("srgb", True)
    tx.set_editor_property("never_stream", True)
    tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
    tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    L.append("%s : %s %dx%d" % (p, EAL.save_loaded_asset(tx), tx.blueprint_get_size_x(), tx.blueprint_get_size_y()))


def scal(m, name, val, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", val)
    return e


def link(a, b, pin, out=""):
    ok = MEL.connect_material_expressions(a, out, b, pin)
    if not ok: L.append("  ECHEC lien %s -> %s.%s" % (a.get_class().get_name(), b.get_class().get_name(), pin))
    return ok


UVC = r"""
float n = Cols * Rows;
float i = clamp(floor(Image + 0.001), 0, n - 1);
float2 c = float2(fmod(i, Cols), floor(i / Cols));
return (saturate(UV) * 0.998 + 0.001 + c) / float2(Cols, Rows);
"""
P = "/Game/EON/VFX/M_EON_Planche"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
m = AT.create_asset("M_EON_Planche", "/Game/EON/VFX", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
# Translucide APRÈS la profondeur de champ : composité après les post-process « avant DOF » (décor peint, trait), qui
# repeignaient le sang en noir quand il était opaque. Bords nets : opacité = alpha × Opacite seuillé à 0,5.
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_AFTER_DOF)
m.set_editor_property("two_sided", True)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
c = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -900, 0)
c.set_editor_property("code", UVC)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
c.set_editor_property("description", "EON_PlancheUV")
ins = []
for nm in ["UV", "Cols", "Rows", "Image"]:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
c.set_editor_property("inputs", ins)
link(uv, c, "UV")
link(scal(m, "Cols", 4.0, -1200, 100), c, "Cols")
link(scal(m, "Rows", 2.0, -1200, 160), c, "Rows")
link(scal(m, "Image", 0.0, -1200, 220), c, "Image")
tx = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
tx.set_editor_property("parameter_name", "Planche")
t0 = unreal.load_asset("%s/T_EON_VFX_P2_Giclee" % DST)
if t0: tx.set_editor_property("texture", t0)
tx.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
link(c, tx, "UVs")
col = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, -250)
col.set_editor_property("parameter_name", "Color"); col.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
k = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, -100)
link(tx, k, "A", "RGB"); link(col, k, "B")
k2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, -100)
link(k, k2, "A"); link(scal(m, "Intensite", 1.0, -300, 60), k2, "B")
L.append("émissif : %s" % MEL.connect_material_property(k2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
ka = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 150)
link(tx, ka, "A", "A"); link(scal(m, "Opacite", 1.0, -450, 250), ka, "B")
st = MEL.create_material_expression(m, unreal.MaterialExpressionStep, -150, 150)
link(scal(m, "Seuil", 0.5, -300, 300), st, "Y"); link(ka, st, "X")
L.append("opacité : %s" % MEL.connect_material_property(st, "", unreal.MaterialProperty.MP_OPACITY))
MEL.recompile_material(m)
L.append("M_EON_Planche : %s" % EAL.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_saved_dir(), "planches_import.txt"), "w", encoding="utf-8").write("\n".join(L))
