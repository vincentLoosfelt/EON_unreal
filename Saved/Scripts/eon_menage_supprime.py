# EON — supprime les paquets listés par eon_menage_inventaire.py (ancien système de Jira, sans référence extérieure).
import unreal
EAL = unreal.EditorAssetLibrary
lst = [l.strip() for l in open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/menage_a_supprimer.txt", encoding="utf-8") if l.strip()]
ok, ko = 0, []
for p in lst:
    if not EAL.does_asset_exist(p): continue
    if EAL.delete_asset(p): ok += 1
    else: ko.append(p)
for d in ("/Game/EON/Characters/Jira/Anims/MM", "/Game/EON/Characters/Jira/Anims/Jira", "/Game/EON/Characters/Jira/Anims/Locomotion",
          "/Game/EON/Characters/Jira/Anims/Locomotion_GASP", "/Game/EON/Characters/Jira/MotionMatching"):
    if EAL.does_directory_exist(d) and not EAL.list_assets(d, recursive=True): EAL.delete_directory(d)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/menage_supprime.txt", "w", encoding="utf-8").write("supprimés %d, refusés %d\n%s" % (ok, len(ko), "\n".join(ko)))
