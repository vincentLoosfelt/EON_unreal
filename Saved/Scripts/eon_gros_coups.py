# EON — gros coups (Vincent, 25/11) : matériau M_PP_EON_Bichrome (écran en 2 tons, noir + couleur de l'effet) et
# texture T_EON_VFX_P7_Onomatopees (planche 3 × 2 détourée par eon_p7_alpha.py).
# M_PP_EON_Bichrome : post-process après le tonemapper ; luminance > Seuil -> Couleur, sinon noir. Posé par
# AEONPlayerCameraManager pendant 2 images à 12 i/s (EONFx::GrosCoup).
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
L = []

# --- texture P7 ---
t = unreal.AssetImportTask()
t.set_editor_property("filename", os.path.join(unreal.Paths.project_dir(), "SourceAssets", "VFX_Planches", "EON_VFX_P7_Onomatopees_alpha.png"))
t.set_editor_property("destination_path", "/Game/EON/VFX/Planches")
t.set_editor_property("destination_name", "T_EON_VFX_P7_Onomatopees")
t.set_editor_property("automated", True)
t.set_editor_property("replace_existing", True)
t.set_editor_property("save", False)
AT.import_asset_tasks([t])
tx = unreal.load_asset("/Game/EON/VFX/Planches/T_EON_VFX_P7_Onomatopees")
tx.set_editor_property("srgb", True)
tx.set_editor_property("never_stream", True)
tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
L.append("P7 %s %dx%d" % (EAL.save_loaded_asset(tx), tx.blueprint_get_size_x(), tx.blueprint_get_size_y()))

# --- matériau bichrome ---
P = "/Game/EON/Materials/M_PP_EON_Bichrome"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
g = AT.create_asset("M_PP_EON_Bichrome", "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())
g.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
g.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
st = MEL.create_material_expression(g, unreal.MaterialExpressionSceneTexture, -700, 0)
st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
col = MEL.create_material_expression(g, unreal.MaterialExpressionVectorParameter, -700, 200)
col.set_editor_property("parameter_name", "Couleur"); col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.45, 0.1, 1.0))
seuil = MEL.create_material_expression(g, unreal.MaterialExpressionScalarParameter, -700, 350)
seuil.set_editor_property("parameter_name", "Seuil"); seuil.set_editor_property("default_value", 0.3)
cu = MEL.create_material_expression(g, unreal.MaterialExpressionCustom, -300, 0)
cu.set_editor_property("code", "float l = dot(C.rgb, float3(0.299, 0.587, 0.114));\nreturn l > Seuil ? Couleur.rgb : float3(0, 0, 0);")
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cu.set_editor_property("description", "Bichrome EON")
ins = []
for nm in ("C", "Couleur", "Seuil"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
cu.set_editor_property("inputs", ins)
L.append("lien C %s" % MEL.connect_material_expressions(st, "Color", cu, "C"))
L.append("lien Couleur %s" % MEL.connect_material_expressions(col, "", cu, "Couleur"))
L.append("lien Seuil %s" % MEL.connect_material_expressions(seuil, "", cu, "Seuil"))
L.append("sortie %s" % MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
MEL.recompile_material(g)
L.append("bichrome %s" % EAL.save_loaded_asset(g))
for l in L:
    unreal.log("EON_GROS " + l)
