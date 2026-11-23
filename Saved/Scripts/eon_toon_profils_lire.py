# EON — lecture seule : Toon Profile branché sur chaque Substrate Toon BSDF des matériaux de base, et réglages de
# chaque profil du projet (rampes diffuse / spéculaire en clés de courbe, extinction, GI).
# Sortie : Saved/Logs/eon_toon_profils.txt
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_toon_profils_lire.py
import unreal
AR = unreal.AssetRegistryHelpers.get_asset_registry()
MEL = unreal.MaterialEditingLibrary
LOG = []
for base in ("/Game/EON/Materials/M_EON_ToonEnnemi", "/Game/EON/Materials/M_EON_ToonMasked", "/Game/EON/Unity/Materials/M_UnityLit"):
    m = unreal.load_asset(base)
    for e in MEL.get_material_expressions(m):
        if isinstance(e, unreal.MaterialExpressionSubstrateToonBSDF):
            tp = e.get_editor_property("toon_profile")
            LOG.append("%s -> %s" % (base, tp.get_path_name() if tp else "AUCUN profil"))


def courbe(rc):
    out = []
    try:
        for i, nom in enumerate(("R", "G", "B", "A")):
            cs = rc.get_editor_property("color_curves")[i]
            keys = cs.get_editor_property("keys")
            out.append("%s=%s" % (nom, [(round(k.get_editor_property("time"), 3), round(k.get_editor_property("value"), 3), str(k.get_editor_property("interp_mode")).split(".")[-1][:12]) for k in keys]))
    except Exception as ex:
        out.append("? %s" % ex)
    return " ".join(out)


f = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "ToonProfile")], package_paths=["/Game"], recursive_paths=True)
for ad in AR.get_assets(f):
    tp = ad.get_asset()
    s = tp.get_editor_property("settings")
    LOG.append("PROFIL %s" % ad.package_name)
    for k in ("diffuse_ramp", "specular_ramp"):
        LOG.append("   %s : %s" % (k, courbe(s.get_editor_property(k))))
    for k in ("shadow_extinction_coefficient", "diffuse_indirect_scale", "specular_indirect_scale", "diffuse_ramp_offset_strength",
              "diffuse_ramp_offset_texture", "b_diffuse_ramp_include_shadow"):
        try:
            LOG.append("   %s = %s" % (k, s.get_editor_property(k)))
        except Exception as ex:
            LOG.append("   %s ? %s" % (k, ex))
    LOG.append("   référencé par : %s" % [str(r) for r in (AR.get_referencers(str(ad.package_name), unreal.AssetRegistryDependencyOptions()) or [])])
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_toon_profils.txt", "w", encoding="utf-8").write("\n".join(LOG))
unreal.log("EON_TOONP ok")
