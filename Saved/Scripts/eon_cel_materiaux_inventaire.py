# EON — inventaire (lecture seule) des matériaux des celluloïds avant passage au Substrate Toon BSDF (Vincent, 23/11) :
# pour chaque personnage / ennemi : slots, instance -> parent(s) -> matériau de base, nœuds Substrate du matériau de
# base, et qui d'autre référence ce matériau de base (pour ne pas toucher au décor).
# Sortie : Saved/Logs/eon_cel_materiaux_inventaire.txt
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_cel_materiaux_inventaire.py
import unreal
AR = unreal.AssetRegistryHelpers.get_asset_registry()
MEL = unreal.MaterialEditingLibrary
LOG = []
f = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "SkeletalMesh")], package_paths=["/Game/EON"], recursive_paths=True)
bases = {}
for ad in AR.get_assets(f):
    sk = ad.get_asset()
    LOG.append("== %s" % ad.package_name)
    for i, sm in enumerate(sk.get_editor_property("materials")):
        mi = sm.get_editor_property("material_interface")
        chaine = []
        m = mi
        while m and isinstance(m, unreal.MaterialInstance):
            chaine.append(m.get_path_name().split(".")[0]); m = m.get_editor_property("parent")
        base = m.get_path_name().split(".")[0] if m else "-"
        chaine.append(base)
        bases.setdefault(base, set()).add(str(ad.package_name))
        LOG.append("  slot %d %s : %s" % (i, sm.get_editor_property("material_slot_name"), " -> ".join(chaine)))
LOG.append("")
for base, sks in sorted(bases.items()):
    if base == "-":
        continue
    m = unreal.load_asset(base)
    types = {}
    try:
        for e in MEL.get_material_expressions(m) if hasattr(MEL, "get_material_expressions") else []:
            types[e.get_class().get_name()] = types.get(e.get_class().get_name(), 0) + 1
    except Exception as ex:
        types["?"] = str(ex)
    refs = [str(r) for r in (AR.get_referencers(base, unreal.AssetRegistryDependencyOptions()) or [])]
    LOG.append("BASE %s | domaine=%s blend=%s modèle=%s | squelettes=%s" % (base, m.get_editor_property("material_domain"), m.get_editor_property("blend_mode"),
                                                                        m.get_editor_property("shading_model"), sorted(sks)))
    LOG.append("   nœuds : %s" % sorted(types.items()))
    LOG.append("   référencé par (%d) : %s" % (len(refs), refs[:25]))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_cel_materiaux_inventaire.txt", "w", encoding="utf-8").write("\n".join(LOG))
unreal.log("EON_CELMAT ok")
