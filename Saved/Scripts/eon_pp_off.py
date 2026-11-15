# EON — filtre peinture (M_PP_EON_Peinture) coupé dans toutes les lignes de DT_GameFeel (Vincent, 15/11 : « pour
# l'instant, tu me stoppes le post-process »). Le volume reste actif (exposition du HUB). Rétablir : PPPaint True.
import json, unreal
DTL = unreal.DataTableFunctionLibrary
dt = unreal.load_asset("/Game/EON/Data/DT_GameFeel")
rows = json.loads(DTL.export_data_table_to_json_string(dt))
for r in rows:
    r["PPPaint"] = False
ok = DTL.fill_data_table_from_json_string(dt, json.dumps(rows))
unreal.EditorAssetLibrary.save_loaded_asset(dt)
unreal.log("EON_PPOFF %d lignes, PPPaint False (%s)" % (len(rows), ok))
