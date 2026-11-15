# EON — ménage après le passage de Jira sur le squelette UEFN : liste ce qui, dans /Game/EON/Characters/Jira et
# /Game/EON/Anims, appartient à l'ancien système (SKEL_Jira, Motion Matching maison, retargets) et qui le référence.
# Écrit Saved/Logs/menage_inventaire.txt et Saved/Logs/menage_a_supprimer.txt (une ligne par paquet).
import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
GARDE_PREFIX = ("/Game/EON/Characters/Jira/SK_Jira_UEFN", "/Game/EON/Characters/Jira/Anims/UEFN/",
                "/Game/EON/Characters/Jira/MI_", "/Game/EON/Characters/Jira/Textures/")
cand = set()
for root in ("/Game/EON/Characters/Jira", "/Game/EON/Anims"):
    for a in ar.get_assets_by_path(root, recursive=True):
        p = str(a.package_name)
        if not p.startswith(GARDE_PREFIX): cand.add(p)
L, ok = [], []
opt = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                            include_searchable_names=False, include_soft_management_references=False,
                                            include_hard_management_references=False)
for p in sorted(cand):
    refs = [str(r) for r in (ar.get_referencers(p, opt) or [])]
    ext = [r for r in refs if r not in cand]
    L.append("%s  <- %s" % (p, ext if ext else "(seulement l'ancien système)"))
    if not ext: ok.append(p)
L.insert(0, "candidats %d, supprimables sans casser %d" % (len(cand), len(ok)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/menage_inventaire.txt", "w", encoding="utf-8").write("\n".join(L))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/menage_a_supprimer.txt", "w", encoding="utf-8").write("\n".join(ok))
