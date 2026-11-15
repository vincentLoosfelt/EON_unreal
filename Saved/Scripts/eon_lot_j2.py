# EON — enchaîne les scripts éditeur du lot du 14/11 (couleurs Unity, vitesse, anims adoucies, carte d'étage).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_lot_j2.py
import traceback, unreal
H = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
OUT = []
for f in ("eon_bg_master.py", "eon_hub_bg.py", "eon_feel_speed.py", "eon_jira_anims_import.py", "eon_etage_map.py"):
    try:
        exec(open(H + f, encoding="utf-8").read(), {"__name__": "__main__"})
        OUT.append("OK  " + f)
    except Exception:
        OUT.append("ERR " + f + "\n" + traceback.format_exc())
    unreal.log("EON_LOT " + OUT[-1])
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_lot_j2.txt", "w", encoding="utf-8").write("\n".join(OUT))
