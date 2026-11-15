# EON — SpeedMul3D = 0,55 dans toutes les lignes de DT_GameFeel (Vincent, 14/11 : Jira trop rapide).
# Proto : marche 82 px/s = 5,1 m/s, course ×1,7 = 8,7 m/s -> 2,8 et 4,8 m/s. Voir MANQUANTS.md §9.
import json, unreal
dt = unreal.load_asset("/Game/EON/Data/DT_GameFeel")
DTL = unreal.DataTableFunctionLibrary
log = []
src = None
for fn in ("export_data_table_to_json_string",):
    if hasattr(DTL, fn):
        src = getattr(DTL, fn)(dt)
if src is None and hasattr(dt, "export_to_json_string"):
    src = dt.export_to_json_string()
rows = json.loads(src)
for r in rows:
    r["SpeedMul3D"] = 0.55
ok = DTL.fill_data_table_from_json_string(dt, json.dumps(rows))
unreal.EditorAssetLibrary.save_loaded_asset(dt)
log.append("DT_GameFeel : %d lignes, SpeedMul3D 0,55 (%s)" % (len(rows), ok))
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_feel_speed.txt", "w", encoding="utf-8").write("\n".join(log))
