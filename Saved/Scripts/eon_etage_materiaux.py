# EON — inventaire (lecture seule) des matériaux de l'étage : acteurs de L_Etage et blueprints de salles BP_Salle_*.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_etage_materiaux.py
import unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_etage_materiaux.txt"
LOG = []
ar = unreal.AssetRegistryHelpers.get_asset_registry()
opt = unreal.AssetRegistryDependencyOptions(True, True, True, True, True)


def mats_de(pkg, vus, prof=0):
    res = set()
    if pkg in vus or prof > 3:
        return res
    vus.add(pkg)
    for d in ar.get_dependencies(pkg, opt) or []:
        d = str(d)
        if not d.startswith("/Game/"):
            continue
        nom = d.rsplit("/", 1)[-1]
        if nom.startswith(("MI_", "M_")):
            res.add(d)
        elif nom.startswith(("SM_", "BP_")) or "/fbx/" in d or "/Meshes/" in d:
            res |= mats_de(d, vus, prof + 1)
    return res


tous = {}
for a in ar.get_assets_by_path("/Game/EON/Salles", False):
    p = str(a.package_name)
    if not p.rsplit("/", 1)[-1].startswith("BP_Salle_"):
        continue
    for m in mats_de(p, set()):
        tous.setdefault(m, []).append(p.rsplit("/", 1)[-1])
LOG.append("=== matériaux des salles (%d) ===" % len(tous))
for m in sorted(tous):
    parent = ""
    o = unreal.load_asset(m)
    if isinstance(o, unreal.MaterialInstance) and o.get_editor_property("parent"):
        parent = o.get_editor_property("parent").get_name()
    LOG.append("%s [%s] : %d salles" % (m, parent, len(tous[m])))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_Etage")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
carte = {}
for act in eas.get_all_level_actors():
    for c in act.get_components_by_class(unreal.MeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m:
                carte[m.get_path_name()] = carte.get(m.get_path_name(), 0) + 1
LOG.append("=== matériaux posés dans L_Etage (%d) ===" % len(carte))
for m, n in sorted(carte.items()):
    LOG.append("%s : %d" % (m, n))
for ppv in [v for v in eas.get_all_level_actors() if isinstance(v, unreal.PostProcessVolume)]:
    s = ppv.settings
    LOG.append("PPV %s bloom %.2f/%.2f/%.2f blendables %s" % (ppv.get_actor_label(), s.bloom_intensity, s.bloom_threshold, s.bloom_size_scale,
               [b.object.get_name() for b in s.weighted_blendables.array if b.object]))
open(OUT, "w", encoding="utf-8").write("\n".join(LOG))
