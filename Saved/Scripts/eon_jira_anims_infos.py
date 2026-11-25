# EON — lecture seule : durée, cadence, root motion des anims de Jira (Saved/Logs/eon_jira_anims_infos.txt).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_jira_anims_infos.py
import unreal
L = []
for a in unreal.EditorAssetLibrary.list_assets("/Game/EON/Characters/Jira/Anims/UEFN", recursive=False):
    s = unreal.load_asset(a)
    if not isinstance(s, unreal.AnimSequence): continue
    L.append("%s len=%.3f fps=%s rm=%s lock=%s" % (s.get_name(), s.get_play_length(), s.get_editor_property("target_frame_rate") if hasattr(s, "target_frame_rate") else "?",
             s.get_editor_property("enable_root_motion"), s.get_editor_property("root_motion_root_lock")))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_jira_anims_infos.txt", "w", encoding="utf-8").write("\n".join(L))
