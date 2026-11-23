# EON — inventaire des textures (lecture seule) : taille, filtrage, mips, groupe LOD, compression, streaming, et si
# elles servent aux cartes L_HUB / L_Etage (dépendances récursives carte -> matériaux -> textures).
# Sortie : Saved/Logs/eon_textures_inventaire.txt (une ligne par texture, suspectes marquées « !! »).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_textures_inventaire.py
import unreal
AR = unreal.AssetRegistryHelpers.get_asset_registry()
opt = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)


def deps_rec(pkg, seen):
    for d in AR.get_dependencies(pkg, opt) or []:
        d = str(d)
        if d.startswith("/Game/") and d not in seen:
            seen.add(d); deps_rec(d, seen)
    return seen


used = {}
for carte in ("/Game/EON/Maps/L_HUB", "/Game/EON/Maps/L_Etage"):
    for d in deps_rec(carte, set()):
        used.setdefault(d, []).append(carte.rsplit("/", 1)[1])
lignes = []
f = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "Texture2D")], package_paths=["/Game"], recursive_paths=True)
for ad in AR.get_assets(f):
    pkg = str(ad.package_name)
    t = ad.get_asset()
    try:
        w, h = t.blueprint_get_size_x(), t.blueprint_get_size_y()
    except Exception:
        w = h = -1
    filt = str(t.get_editor_property("filter")).split(".")[-1]
    mips = str(t.get_editor_property("mip_gen_settings")).split(".")[-1]
    lod = str(t.get_editor_property("lod_group")).split(".")[-1]
    comp = str(t.get_editor_property("compression_settings")).split(".")[-1]
    ns = t.get_editor_property("never_stream")
    srgb = t.get_editor_property("srgb")
    bias = t.get_editor_property("lod_bias")
    try:
        maxs = t.get_editor_property("max_texture_size")
    except Exception:
        maxs = -1
    sus = []
    if filt in ("TF_NEAREST",): sus.append("filtre nearest")
    if mips in ("TMGS_NO_MIPMAPS", "TMGS_LEAVE_EXISTING_MIPS"): sus.append("sans mips")
    if lod in ("TEXTUREGROUP_PIXELS2D", "TEXTUREGROUP_UI"): sus.append("groupe " + lod)
    if bias: sus.append("lod_bias %d" % bias)
    if maxs and maxs > 0 and maxs < max(w, h): sus.append("max_size %d" % maxs)
    if 0 < max(w, h) <= 128: sus.append("petite")
    lignes.append("%s %s | %dx%d | %s | %s | %s | %s | sRGB=%s | never_stream=%s | cartes=%s | %s" % (
        "!!" if sus else "  ", pkg, w, h, filt, mips, lod, comp, srgb, ns, ",".join(used.get(pkg, [])) or "-", "; ".join(sus)))
lignes.sort()
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_textures_inventaire.txt", "w", encoding="utf-8").write("\n".join(lignes))
unreal.log("EON_TEX %d textures" % len(lignes))
