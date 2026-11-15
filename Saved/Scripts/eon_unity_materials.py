# EON — matériaux des meshes repris d'Unity (à lancer éditeur fermé, après eon_import_unity.py) :
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_unity_materials.py
# Les Shader Graph Unity (trame, pigment) ne sont pas recréés : on garde couleur, normale,
# métal, émission et teinte, sur un matériau maître simple M_UnityLit.
# Les meshes à couleurs unies (Hitech, moniteurs, cuves) gardent les matériaux de leur FBX.
import unreal

AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TX = "/Game/EON/Unity/import/textures"
MAT_DIR = "/Game/EON/Unity/Materials"
REPORT = []


def log(m):
    REPORT.append(m)
    unreal.log("EON_UMAT " + m)


# Jeux de textures (d'après les .mat Unity ; teinte = _BaseColor quand elle n'est pas neutre).
SETS = {
    "Grille01": dict(base=TX + "/enviro/Grille01_Material_AlbedoTransparency", normal=TX + "/enviro/Grille01_Material_Normal", tint=(0.78, 0.78, 0.78)),
    "Panel": dict(base=TX + "/enviro/Panel_DefaultMaterial_AlbedoTransparency", normal=TX + "/enviro/Panel_DefaultMaterial_Normal"),
    "Pipe01": dict(base=TX + "/enviro/ASSET02_DefaultMaterial_AlbedoTransparency", normal=TX + "/enviro/ASSET02_DefaultMaterial_Normal"),
    "Pipe02": dict(base=TX + "/enviro/Pipe02_DefaultMaterial_AlbedoTransparency", normal=TX + "/enviro/Pipe02_DefaultMaterial_Normal"),
    "Porte": dict(base=TX + "/Porte_BC", normal=TX + "/Porte_Nml", metal=TX + "/Porte_M", emis=TX + "/Porte_Emi"),
    "Casier": dict(base=TX + "/Casier_Base_Color", normal=TX + "/Casier_Normal_Map", metal=TX + "/Casier_Metallic"),
    "Mur": dict(base=TX + "/Mur_BC", normal=TX + "/Mur_NM", metal=TX + "/Mur_M"),
    "Barrier": dict(base=TX + "/enviro/yellow_black_strips"),
    "Tube01": dict(base=TX + "/enviro/tube/Tube01_DefaultMaterial_AlbedoTransparency", normal=TX + "/enviro/tube/Tube01_DefaultMaterial_Normal"),
    "Holo": dict(base=TX + "/enviro/holovincent/holovincent_BC", normal=TX + "/enviro/holovincent/holovincent_Nml",
                 metal=TX + "/enviro/holovincent/holovincent_M", emis=TX + "/enviro/holovincent/holovincent__Emi"),
    "Captain": dict(base=TX + "/chara/captain_v0_1b_Material_001_AlbedoTransparency"),
    "Minion": dict(base=TX + "/Meshy_AI_Minion_0106165841_texture", normal=TX + "/Meshy_AI_Minion_0106165841_texture_Material_001_Normal"),
    "Monster05": dict(base=TX + "/xeno/Monster05_DefaultMaterial_AlbedoTransparency", normal=TX + "/xeno/Monster05_DefaultMaterial_Normal"),
    "Ghoul": dict(base=TX + "/xeno/T_Ghoul_BaseColor", normal=TX + "/xeno/Monster05_DefaultMaterial_Normal"),
    "Weapon01": dict(normal=TX + "/weapons/weapon01_nm"),
    "Crate": dict(normal="/Game/EON/Unity/import/fbx/lootable/crate_low_DefaultMaterial_Normal", tint=(1.0, 0.46, 0.0)),
}

# Mesh (nom de l'asset importé) -> jeu de textures.
MESHES = {
    "Grille01": "Grille01", "Panel01": "Panel", "Pipe01": "Pipe01", "Pipe02": "Pipe02",
    "porte": "Porte", "Casier": "Casier", "Tube01": "Tube01", "holovincent_ad_gm": "Holo",
    "WallA040": "Mur", "WallA040Corner": "Mur", "WallA040CornerIn": "Mur", "WallA160": "Mur",
    "WallA160Corner": "Mur", "WallA160CornerIn": "Mur", "WallA320": "Mur",
    "Barrier_V1": "Barrier", "Barrier_V2_1": "Barrier", "Barrier_V2_2": "Barrier", "Barrier_V3_1": "Barrier",
    "Barrier_V3_2": "Barrier", "Barrier_V4_1": "Barrier", "Barrier_V4_2": "Barrier",
    "captain_v0_2": "Captain", "Minion": "Minion", "Monster05": "Monster05", "Ghoul_Anim": "Ghoul",
    "weapon01": "Weapon01", "crate_low": "Crate",
}


def tex(path):
    if not path:
        return None
    a = unreal.load_asset(path)
    if not a:
        log("texture absente : " + path)
    return a


def make_master():
    path = MAT_DIR + "/M_UnityLit"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    m = AT.create_asset("M_UnityLit", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    P = unreal.MaterialProperty

    def tparam(name, default, x, y, sampler=None):
        n = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        n.set_editor_property("parameter_name", name)
        n.set_editor_property("texture", unreal.load_asset(default))
        if sampler is not None:
            n.set_editor_property("sampler_type", sampler)
        return n

    def vparam(name, val, x, y):
        n = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
        n.set_editor_property("parameter_name", name)
        n.set_editor_property("default_value", unreal.LinearColor(*val))
        return n

    def sparam(name, val, x, y):
        n = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", name)
        n.set_editor_property("default_value", val)
        return n

    def mul(a, ao, b, bo, x, y):
        n = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, x, y)
        MEL.connect_material_expressions(a, ao, n, "A")
        MEL.connect_material_expressions(b, bo, n, "B")
        return n

    white = "/Engine/EngineResources/WhiteSquareTexture"
    black = "/Engine/EngineResources/Black"
    base = tparam("BaseTex", white, -800, -300)
    tint = vparam("Tint", (1, 1, 1, 1), -800, -100)
    MEL.connect_material_property(mul(base, "RGB", tint, "", -500, -250), "", P.MP_BASE_COLOR)

    nrm = tparam("NormalTex", "/Engine/EngineMaterials/DefaultNormal", -800, 100, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_property(nrm, "RGB", P.MP_NORMAL)

    met = tparam("MetalTex", black, -800, 350, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    mets = sparam("Metallic", 0.0, -800, 550)
    add = MEL.create_material_expression(m, unreal.MaterialExpressionMax, -500, 400)
    MEL.connect_material_expressions(met, "R", add, "A")
    MEL.connect_material_expressions(mets, "", add, "B")
    MEL.connect_material_property(add, "", P.MP_METALLIC)
    MEL.connect_material_property(sparam("Roughness", 0.6, -500, 600), "", P.MP_ROUGHNESS)

    emi = tparam("EmisTex", black, -800, 750)
    # L_TestRoom est en exposition physique : l'émission doit être forte pour se voir.
    emc = vparam("EmisColor", (1, 1, 1, 1), -800, 950)
    emk = sparam("EmisStrength", 20.0, -800, 1100)
    MEL.connect_material_property(mul(mul(emi, "RGB", emc, "", -550, 800), "", emk, "", -350, 850), "", P.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


master = make_master()
log("M_UnityLit créé")

mis = {}
for key, s in SETS.items():
    path = MAT_DIR + "/MI_Unity_" + key
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    mi = AT.create_asset("MI_Unity_" + key, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, master)
    for p, name in (("base", "BaseTex"), ("normal", "NormalTex"), ("metal", "MetalTex"), ("emis", "EmisTex")):
        t = tex(s.get(p))
        if t:
            MEL.set_material_instance_texture_parameter_value(mi, name, t)
    if "tint" in s:
        MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*s["tint"], 1.0))
    EAL.save_loaded_asset(mi)
    mis[key] = mi
log("%d instances créées" % len(mis))

# Affectation : tous les emplacements du mesh reçoivent l'instance (ces meshes n'en ont qu'un, ou
# plusieurs qui partageaient le même matériau côté Unity).
reg = unreal.AssetRegistryHelpers.get_asset_registry()
done = 0
for ad in reg.get_assets_by_path("/Game/EON/Unity", recursive=True):
    name = str(ad.asset_name)
    if name not in MESHES:
        continue
    a = ad.get_asset()
    mi = mis[MESHES[name]]
    if isinstance(a, unreal.StaticMesh):
        mats = list(a.get_editor_property("static_materials"))
        for sm in mats:
            sm.set_editor_property("material_interface", mi)
        a.set_editor_property("static_materials", mats)
    elif isinstance(a, unreal.SkeletalMesh):
        mats = list(a.get_editor_property("materials"))
        for sm in mats:
            sm.set_editor_property("material_interface", mi)
        a.set_editor_property("materials", mats)
    else:
        continue
    EAL.save_loaded_asset(a)
    done += 1
    log("%s (%d emplacements) <- MI_Unity_%s" % (name, len(mats), MESHES[name]))
log("meshes texturés : %d" % done)

out = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_unity_materials.txt"
open(out, "w", encoding="utf-8").write("\n".join(REPORT))
