# EON — cuves de réveil du Kibo (assets) :
#  1. Cuve_Reveil_Corps / Cuve_Reveil_Verre importés depuis Saved/Import/Cuve_Reveil (blender_cuve_reveil.py), sans Nanite
#     (le verre est translucide), matériaux Unity de Cuve_1 reposés par nom d'emplacement.
#  2. M_EON_Translucide (translucide éclairé : Couleur, Opacite, Emission) et ses instances :
#     MI_EON_Verre, MI_EON_Liquide (liquide amniotique vert du proto, #1f5a48 / #14342c), MI_EON_Vapeur, MI_EON_Silhouette.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_cuve_reveil.py
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Unity/import/fbx/Cuve_Reveil"
MDIR = "/Game/EON/Materials/Cuves"
L = []

# --- 2. matériau maître
mp = MDIR + "/M_EON_Translucide"
if EAL.does_asset_exist(mp):
    m = unreal.load_asset(mp); MEL.delete_all_material_expressions(m)
else:
    m = AT.create_asset("M_EON_Translucide", MDIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
m.set_editor_property("two_sided", True)
P = unreal.MaterialProperty
def node(cls, x, y):
    return MEL.create_material_expression(m, cls, x, y)
col = node(unreal.MaterialExpressionVectorParameter, -600, -100); col.set_editor_property("parameter_name", "Couleur"); col.set_editor_property("default_value", unreal.LinearColor(0.6, 0.9, 0.85, 1))
opa = node(unreal.MaterialExpressionScalarParameter, -600, 150); opa.set_editor_property("parameter_name", "Opacite"); opa.set_editor_property("default_value", 0.2)
emi = node(unreal.MaterialExpressionScalarParameter, -600, 50); emi.set_editor_property("parameter_name", "Emission"); emi.set_editor_property("default_value", 0.0)
rou = node(unreal.MaterialExpressionScalarParameter, -600, 250); rou.set_editor_property("parameter_name", "Rugosite"); rou.set_editor_property("default_value", 0.08)
mul = node(unreal.MaterialExpressionMultiply, -300, 0)
MEL.connect_material_expressions(col, "", mul, "A"); MEL.connect_material_expressions(emi, "", mul, "B")
MEL.connect_material_property(col, "", P.MP_BASE_COLOR)
MEL.connect_material_property(mul, "", P.MP_EMISSIVE_COLOR)
MEL.connect_material_property(opa, "", P.MP_OPACITY)
MEL.connect_material_property(rou, "", P.MP_ROUGHNESS)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)

def inst(name, c, o, e, r=0.08):
    p = MDIR + "/" + name
    mi = unreal.load_asset(p) if EAL.does_asset_exist(p) else AT.create_asset(name, MDIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, m)
    MEL.set_material_instance_vector_parameter_value(mi, "Couleur", unreal.LinearColor(*c))
    MEL.set_material_instance_scalar_parameter_value(mi, "Opacite", o)
    MEL.set_material_instance_scalar_parameter_value(mi, "Emission", e)
    MEL.set_material_instance_scalar_parameter_value(mi, "Rugosite", r)
    EAL.save_loaded_asset(mi)
    return mi
def srgb(h):
    return [((int(h[i:i + 2], 16) / 255.0) ** 2.2) for i in (1, 3, 5)]
verre = inst("MI_EON_Verre", (0.55, 0.8, 0.8, 1), 0.16, 0.0, 0.04)
inst("MI_EON_Liquide", tuple(srgb("#1f5a48") + [1]), 0.6, 2.0, 0.2)
inst("MI_EON_Vapeur", tuple(srgb("#d8f0e8") + [1]), 0.3, 0.4, 0.9)
inst("MI_EON_Silhouette", tuple(srgb("#050d0a") + [1]), 0.92, 0.0, 0.6)

# --- 1. maillages
tasks = []
for nm in ("Cuve_Reveil_Corps", "Cuve_Reveil_Verre"):
    o = unreal.FbxImportUI()
    o.automated_import_should_detect_type = False
    o.import_mesh = True; o.import_textures = False; o.import_materials = False; o.import_as_skeletal = False; o.import_animations = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    d = o.static_mesh_import_data
    d.combine_meshes = True; d.auto_generate_collision = False; d.generate_lightmap_u_vs = False
    d.transform_vertex_to_absolute = False; d.import_uniform_scale = 1.0
    t = unreal.AssetImportTask()
    t.filename = "D:/_PROJETS/briscard/EON_UE/Saved/Import/Cuve_Reveil/%s.fbx" % nm
    t.destination_path = DIR; t.destination_name = nm
    t.automated = True; t.replace_existing = True; t.save = True; t.options = o
    tasks.append(t)
AT.import_asset_tasks(tasks)
for nm in ("Cuve_Reveil_Corps", "Cuve_Reveil_Verre"):
    b = unreal.load_asset(DIR + "/" + nm)
    ns = b.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", False); b.set_editor_property("nanite_settings", ns)
    mats = list(b.static_materials)
    for s in mats:
        n = str(s.material_slot_name)
        mi = verre if n.startswith("Glass") else unreal.load_asset("/Game/EON/Unity/import/fbx/%s" % n)
        s.set_editor_property("material_interface", mi)
    b.set_editor_property("static_materials", mats)
    EAL.save_loaded_asset(b)
    L.append("%s : bornes %s, emplacements %s" % (nm, b.get_bounding_box(), [(str(s.material_slot_name), s.material_interface.get_name() if s.material_interface else None) for s in b.static_materials]))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/cuve_reveil.txt", "w", encoding="utf-8").write("\n".join(L))
