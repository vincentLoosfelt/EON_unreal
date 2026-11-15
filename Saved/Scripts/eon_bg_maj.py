# EON — met à jour, SUR PLACE, les instances MI_BG_* d'après eon_bg_params.json (après unity_bg_couleurs.py).
# Sur place : les cartes et Blueprints gardent leurs références (eon_bg_master.py, lui, supprime et recrée).
# 15/11 : couleurs héritées des Material Variant d'Unity (walll painted = MasterPainted vert-de-gris, etc.).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_bg_maj.py
import json, os, re, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
HERE = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
PARAMS = json.load(open(HERE + "eon_bg_params.json", encoding="utf-8"))
MI_DIR = "/Game/EON/Materials/BG"
reg = unreal.AssetRegistryHelpers.get_asset_registry()


def ue_texture(unity_path):
    if not unity_path or "/Assets/" not in unity_path:
        return None
    rel = os.path.splitext(unity_path.split("/Assets/", 1)[1])[0]
    parts = [re.sub(r"[^A-Za-z0-9_]", "_", p) for p in rel.split("/")]
    folder, name = "/Game/EON/Unity/" + "/".join(parts[:-1]), parts[-1]
    for ad in reg.get_assets_by_path(folder, recursive=False):
        if str(ad.asset_name) == name:
            return ad.get_asset()
    return None


for mat, p in PARAMS.items():
    path = MI_DIR + "/MI_BG_" + re.sub(r"[^A-Za-z0-9]", "_", mat)
    if not EAL.does_asset_exist(path):
        unreal.log("EON_BGMAJ absent : %s (lancer eon_bg_master.py pour le créer)" % path)
        continue
    mi = unreal.load_asset(path)
    MEL.set_material_instance_vector_parameter_value(mi, "BC1", unreal.LinearColor(*p["BC1"], 1.0))
    MEL.set_material_instance_vector_parameter_value(mi, "K", unreal.LinearColor(*p["K"], 1.0))
    t = ue_texture(p.get("BC"))
    MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", t or unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    tri = bool(p["Triplanar"]) and p["Scale1"] > 0
    MEL.set_material_instance_static_switch_parameter_value(mi, "Triplanar", tri)
    if tri:
        MEL.set_material_instance_scalar_parameter_value(mi, "TriSize", 100.0 / p["Scale1"])
    MEL.set_material_instance_scalar_parameter_value(mi, "RimPower", p["RimPower"])
    MEL.set_material_instance_scalar_parameter_value(mi, "RimBright", p["RimBright"])
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    unreal.log("EON_BGMAJ %s : BC1=%s K=%s tex=%s" % (mat, [round(c, 3) for c in p["BC1"]], [round(c, 3) for c in p["K"]], t.get_name() if t else "-"))
