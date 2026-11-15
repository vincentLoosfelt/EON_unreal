# EON — couleurs Unity de la COM (COMv1.prefab : surcharges de matériaux par pièce -> emplacements du mesh fusionné).
#   metal -> New Material (BG, 0.52/0.63/0.61)   ground -> New Material 2 (0.54)
#   metal.001 -> New Material 3 (0.78, 4 pièces sur 9)   Material.006 (ancres) -> New Material 1 (0.74/0.67/0.54)
#   light : non surchargé dans Unity (matériau du FBX, émissif) ; dôme : M_EON_ChampForce (eon_com_dome.py).
# Provisoire sur M_UnityLit : la rampe toon (BG.shadergraph) viendra avec le matériau maître Substrate Toon.
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
master = unreal.load_asset("/Game/EON/Unity/Materials/M_UnityLit")
DIR = "/Game/EON/Unity/Materials/COM"
COLS = {"metal": ("MI_COM_GrisVert", (0.523545, 0.627, 0.6105413)),
        "ground": ("MI_COM_Gris", (0.5377358, 0.5377358, 0.5377358)),
        "metal_001": ("MI_COM_GrisClair", (0.7830189, 0.7830189, 0.7830189)),
        "Material_006": ("MI_COM_Beige", (0.735849, 0.6692896, 0.5449448))}
sm = unreal.load_asset("/Game/EON/Unity/import/fbx/enviro/COMv1")
mats = list(sm.get_editor_property("static_materials"))
log = []
for s in mats:
    slot = str(s.get_editor_property("material_slot_name"))
    key = slot.replace(".", "_")
    if key not in COLS:
        log.append("garde %s" % slot); continue
    name, rgb = COLS[key]
    path = DIR + "/" + name
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else AT.create_asset(name, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", master)
    MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*rgb, 1.0))
    EAL.save_loaded_asset(mi)
    s.set_editor_property("material_interface", mi)
    log.append("%s -> %s" % (slot, name))
sm.set_editor_property("static_materials", mats)
EAL.save_loaded_asset(sm)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_com_materials.txt", "w", encoding="utf-8").write("\n".join(log))
