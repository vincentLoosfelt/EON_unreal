# EON — greffe Standard (JIR-G1) en bleu marine #1b2f8a (Vincent, 25/11), dans DT_Greffes.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal
P = "/Game/EON/Data/DT_Greffes"
dt = unreal.load_asset(P)
if not dt:
    unreal.log_error("EON_GREFFE table introuvable " + P)
else:
    js = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(dt) if hasattr(unreal.DataTableFunctionLibrary, "export_data_table_to_json_string") else dt.export_to_json_string()
    import json
    rows = json.loads(js)
    n = 0
    for r in rows:
        if str(r.get("Name", "")).lower() == "standard":
            r["Col"] = {k: (x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4) for k, x in zip("RGB", (0x1b / 255.0, 0x2f / 255.0, 0x8a / 255.0))}; r["Col"]["A"] = 1.0   # sRGB -> linéaire, comme eon_setup_j0.hex_lin
            n += 1
    ok = dt.fill_from_json_string(json.dumps(rows))
    unreal.log("EON_GREFFE lignes %d, rempli %s, sauve %s" % (n, ok, unreal.EditorAssetLibrary.save_loaded_asset(dt)))
