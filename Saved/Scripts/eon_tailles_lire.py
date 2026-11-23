# EON — lecture seule : DT_Ennemis exporté en JSON (hauteur de hitbox HH en px du proto) -> Saved/Logs/eon_tailles.json
import unreal
dt = unreal.load_asset("/Game/EON/Data/DT_Ennemis")
s = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(dt) if dt else "DT introuvable"
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_tailles.json", "w", encoding="utf-8").write(s)
