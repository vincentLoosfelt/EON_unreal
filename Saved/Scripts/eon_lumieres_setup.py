# EON — lumières posables de l'éditeur de salles (assets dynamiques) : M_EON_Neon + Blueprints BPA_* enfants d'AEONLampe
# dans /Game/EON/RoomAssets/Dynamiques/Lumieres. Un Blueprint existant n'est pas recréé (retouches de l'artiste gardées).
# Lampes du proto (room.lights) : néon mural (r 52 px, i 0,8, contre le mur du haut), lampe au sol (r 24, i 0,5).
import unreal

OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_lumieres_setup.txt"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("EON lumieres : " + str(m))


EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/RoomAssets/Dynamiques/Lumieres"
NEON = "/Game/EON/Materials/M_EON_Neon"
CUBE = "/Game/EON/Meshes/SM_Cube_N"


def make_neon():
    if EAL.does_asset_exist(NEON):
        return unreal.load_asset(NEON)
    m = AT.create_asset("M_EON_Neon", "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())
    P = unreal.MaterialProperty
    col = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -700, 0)
    col.set_editor_property("parameter_name", "Couleur")
    col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.046, 0.25, 1.0))
    em = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 200)
    em.set_editor_property("parameter_name", "Emission")
    em.set_editor_property("default_value", 40.0)
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 100)
    MEL.connect_material_expressions(col, "", mul, "A")
    MEL.connect_material_expressions(em, "", mul, "B")
    MEL.connect_material_property(mul, "", P.MP_EMISSIVE_COLOR)
    # Proto : lampe éteinte = trait sombre #14122a.
    base = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, -150)
    base.set_editor_property("constant", unreal.LinearColor(0.007, 0.006, 0.023, 1.0))
    MEL.connect_material_property(base, "", P.MP_BASE_COLOR)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    log("M_EON_Neon créé")
    return m


def sub():
    return unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def obj(h):
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))


def make_lamp(name, mode, rayon, inten, light_loc, meshes):
    path = DIR + "/" + name
    if EAL.does_asset_exist(path):
        log("existe déjà : " + path)
        return
    fac = unreal.BlueprintFactory()
    fac.set_editor_property("parent_class", unreal.EONLampe)
    bp = AT.create_asset(name, DIR, unreal.Blueprint, fac)
    s = sub()
    hs = s.k2_gather_subobject_data_for_blueprint(bp)
    root = None
    for h in hs:
        o = obj(h)
        if isinstance(o, unreal.PointLightComponent):
            o.set_editor_property("relative_location", unreal.Vector(*light_loc))
        if o is not None and o.get_name().startswith("Racine"):
            root = h
    root = root or hs[0]
    neon = unreal.load_asset(NEON)
    cube = unreal.load_asset(CUBE)
    for mname, loc, scale in meshes:
        h, fail = s.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.StaticMeshComponent, blueprint_context=bp))
        if not fail.is_empty():
            raise RuntimeError(str(fail))
        s.rename_subobject(h, unreal.Text(mname))
        c = obj(h)
        c.set_editor_property("static_mesh", cube)
        c.set_material(0, neon)
        c.set_editor_property("relative_location", unreal.Vector(*loc))
        c.set_editor_property("relative_scale3d", unreal.Vector(*scale))
        c.set_collision_profile_name("NoCollision")
        c.set_editor_property("cast_shadow", False)
        c.set_editor_property("component_tags", ["neon"])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("mode", mode)
    cdo.set_editor_property("rayon_px", rayon)
    cdo.set_editor_property("intensite", inten)
    if not EAL.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError("enregistrement impossible : " + path)
    log("créé : " + path)


try:
    make_neon()
    M = unreal.EONLampeMode
    # Néon mural : posé sur la tuile de mur, face au sud à rotation 0 (mur du haut, comme le proto). Trait de 50 × 12 cm sur la
    # façade, en haut du mur (220 cm) ; la lumière est devant la façade.
    make_lamp("BPA_Neon_mural", M.FIXE, 52.0, 0.8, (0.0, 85.0, 170.0), [("Neon", (0.0, 51.0, 190.0), (0.5, 0.04, 0.12))])
    # Lampe au sol : dalle lumineuse au centre de la tuile.
    make_lamp("BPA_Lampe_sol", M.FIXE, 24.0, 0.5, (0.0, 0.0, 40.0), [("Dalle", (0.0, 0.0, 1.0), (0.5, 0.5, 0.02))])
    # Point lumineux : seulement la lumière, à 1,5 m.
    make_lamp("BPA_Point_lumineux", M.FIXE, 52.0, 0.8, (0.0, 0.0, 150.0), [])
except Exception as e:
    import traceback
    log("ERREUR " + traceback.format_exc())
open(OUT, "w", encoding="utf-8").write("\n".join(lines) + "\n")
