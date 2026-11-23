# EON — change des réglages de DT_GameFeel (toutes les lignes) et enregistre. EON_FEEL="Cle=valeur;Cle2=valeur".
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_feel_set.py
import json, os, unreal
DT = "/Game/EON/Data/DT_GameFeel"
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_feel_set.txt"
REGLES = dict(p.split("=", 1) for p in os.environ.get("EON_FEEL", "").split(";") if "=" in p)
LOG = []
dt = unreal.load_asset(DT)
lib = unreal.DataTableFunctionLibrary
txt = lib.export_data_table_to_json_string(dt) if hasattr(lib, "export_data_table_to_json_string") else dt.export_to_json_string()
rows = json.loads(txt)
for r in rows:
    for k, v in REGLES.items():
        LOG.append("%s : %s %s -> %s" % (r.get("Name"), k, r.get(k), v))
        r[k] = type(r[k])(json.loads(v)) if k in r and not isinstance(r[k], str) else v
ok = lib.fill_data_table_from_json_string(dt, json.dumps(rows))
LOG.append("rempli : %s ; sauvegarde : %s" % (ok, unreal.EditorAssetLibrary.save_loaded_asset(dt, False)))
rows2 = json.loads(lib.export_data_table_to_json_string(dt) if hasattr(lib, "export_data_table_to_json_string") else dt.export_to_json_string())
for r in rows2:
    LOG.append("relu %s : %s" % (r.get("Name"), {k: r.get(k) for k in REGLES}))
open(OUT, "w", encoding="utf-8").write("\n".join(LOG))
