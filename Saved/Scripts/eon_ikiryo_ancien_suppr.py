# EON — supprime l'ancien Ikiryo en pièces statiques (body/jaws/shooter_arm + squelettes + UV_Shooter), remplacé par
# SK_Ikiryo (rig du studio). Refuse si un asset extérieur à la liste y fait encore référence.
import unreal, os
EAL = unreal.EditorAssetLibrary
AR = unreal.AssetRegistryHelpers.get_asset_registry()
D = "/Game/EON/Ennemis/Ikiryo/"
OLD = [D + n for n in ("body", "body_Skeleton", "jaws", "jaws_Skeleton", "shooter_arm", "shooter_arm_Skeleton", "UV_Shooter")]
opt = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
LOG = []
ext = []
for p in OLD:
    refs = [str(r) for r in (AR.get_referencers(p, opt) or [])]
    LOG.append("%s <- %s" % (p, refs))
    ext += [r for r in refs if r not in OLD]
if ext:
    LOG.append("ARRÊT : références extérieures %s" % sorted(set(ext)))
else:
    for p in OLD:
        if EAL.does_asset_exist(p):
            LOG.append("supprimé %s : %s" % (p, EAL.delete_asset(p)))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_ikiryo_ancien_suppr.txt"), "w", encoding="utf-8").write("\n".join(LOG))
