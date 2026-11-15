# EON — supprime les assets des sprites pixel (ennemis passés en 3D) : M_PixelSprite, DT_Sprites, /Game/EON/Placeholders.
# Un asset n'est supprimé que si rien d'autre que ces assets ne le référence.
import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
eal = unreal.EditorAssetLibrary
cands = ["/Game/EON/Materials/M_PixelSprite", "/Game/EON/Data/DT_Sprites"]
cands += [str(a.package_name) for a in ar.get_assets_by_path("/Game/EON/Placeholders", recursive=True)]
cset = set(cands)
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True, include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
garde, suppr = [], []
for p in cands:
    refs = [str(r) for r in (ar.get_referencers(p, opts) or [])]
    autres = [r for r in refs if r not in cset]
    (garde if autres else suppr).append((p, autres))
for p, a in garde:
    unreal.log("SUPPR garde %s (utilisé par %s)" % (p, a))
n = 0
for p, _ in suppr:
    if eal.delete_asset(p): n += 1
    else: unreal.log("SUPPR echec %s" % p)
unreal.log("SUPPR supprimés %d / %d, gardés %d" % (n, len(cands), len(garde)))
