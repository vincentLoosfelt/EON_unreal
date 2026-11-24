# EON — inventaire des drapeaux d'usage manquants (Vincent, 24/11 : « 4 ») : pour chaque StaticMesh Nanite et chaque
# SkeletalMesh de /Game, le matériau de base de chaque slot doit avoir used_with_nanite (+ instances) / used_with_skeletal_mesh,
# sinon le jeu empaqueté affiche le matériau par défaut d'Unreal. EON_FIX=1 pose les drapeaux manquants et sauve.
# Liste aussi les références à M_PP_EON_Peinture.
import os, unreal
FIX = os.environ.get("EON_FIX") == "1"
MEL, EAL = unreal.MaterialEditingLibrary, unreal.EditorAssetLibrary
AR = unreal.AssetRegistryHelpers.get_asset_registry()
L = []
need = {}   # chemin matériau de base -> set(usages)
def base_of(mi):
    if not mi: return None
    try: return mi.get_base_material()
    except Exception: return None
def add(mesh, mats, usages):
    for m in mats:
        b = base_of(m)
        if not b: continue
        e = need.setdefault(b.get_path_name(), [b, set(), set()])
        e[1].update(usages); e[2].add(mesh)
for ad in AR.get_assets_by_path("/Game", recursive=True):
    cls = str(ad.asset_class_path.asset_name)
    if cls not in ("StaticMesh", "SkeletalMesh"): continue
    a = ad.get_asset()
    if not a: continue
    if cls == "StaticMesh":
        ns = a.get_editor_property("nanite_settings")
        mats = [s.material_interface for s in a.get_editor_property("static_materials")]
        if ns.enabled: add(a.get_name(), mats, {"nanite", "ism"})
        else: add(a.get_name(), mats, {"ism"})
    else:
        mats = [s.material_interface for s in a.get_editor_property("materials")]
        add(a.get_name(), mats, {"skel"})
# Matériaux posés par le code (EONTileRoom, cuves, arcs...) sur des ISM ou des meshes Nanite : Nanite + instances.
RUNTIME = ["/Game/EON/Materials/M_Flat", "/Game/EON/Materials/MI_EON_Arc", "/Game/EON/Materials/Cuves/MI_EON_Liquide",
           "/Game/EON/Materials/Cuves/MI_EON_Silhouette", "/Game/EON/Materials/Cuves/MI_EON_Vapeur", "/Game/EON/Materials/Cuves/MI_EON_Verre",
           "/Game/EON/Materials/MI_Sol_Eclaire"]
for rp in RUNTIME:
    if EAL.does_asset_exist(rp):
        b = base_of(unreal.load_asset(rp))
        if b:
            e = need.setdefault(b.get_path_name(), [b, set(), set()]); e[2].add("code:" + rp.split("/")[-1])
            e[1].update({"ism"} if b.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT else {"nanite", "ism"})
    else:
        L.append("absent : %s" % rp)
PROP = {"nanite": ("used_with_nanite", unreal.MaterialUsage.MATUSAGE_NANITE),
        "ism": ("used_with_instanced_static_meshes", unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES),
        "skel": ("used_with_skeletal_mesh", unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)}
nfix = 0
for p, (b, us, meshes) in sorted(need.items()):
    if p.startswith("/Engine"): continue
    miss = [u for u in sorted(us) if not b.get_editor_property(PROP[u][0])]
    if not miss: continue
    # l'usage « instances » n'est exigé que si le mesh est posé en ISM ; on le signale sans le compter comme bloquant pour les non-Nanite
    L.append("%s manque %s (%d meshes, ex. %s)" % (p, ",".join(miss), len(meshes), ", ".join(sorted(meshes)[:3])))
    if FIX and p.startswith("/Game/EON"):   # /Game/Levels et MetaHumans : pas à nous
        for u in miss: MEL.set_material_usage(b, PROP[u][1])
        MEL.recompile_material(b)
        L.append("   -> corrigé, sauvegarde %s" % EAL.save_loaded_asset(b, False))
        nfix += 1
L.append("--- %d matériaux de base vus, %d corrigés" % (len(need), nfix))
refs = AR.get_referencers("/Game/EON/Materials/M_PP_EON_Peinture", unreal.AssetRegistryDependencyOptions())
L.append("références à M_PP_EON_Peinture : %s" % [str(r) for r in (refs or [])])
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_usages_scan.txt", "w", encoding="utf-8").write("\n".join(L))
