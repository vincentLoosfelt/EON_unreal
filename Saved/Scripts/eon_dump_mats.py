# EON — inventaire des paramètres des matériaux des assets de salle (pour l'aperçu texturé de l'éditeur de salles).
import unreal

OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_dump_mats.txt"
MEL = unreal.MaterialEditingLibrary
seen = {}
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in ar.get_assets_by_path("/Game/EON/Unity/import/fbx", recursive=True):
    if str(ad.asset_class_path.asset_name) != "StaticMesh":
        continue
    sm = ad.get_asset()
    for sl in sm.get_editor_property("static_materials"):
        m = sl.get_editor_property("material_interface")
        if m and m.get_path_name() not in seen:
            seen[m.get_path_name()] = m
for p in ["/Game/EON/Materials/MI_Sol_Eclaire", "/Game/EON/Materials/BG/MI_BG_walll_painted", "/Game/EON/Materials/BG/MI_BG_Barrier01", "/Game/EON/Materials/BG/MI_BG_pipe_1"]:
    seen[p] = unreal.load_asset(p)
out = []
for p, m in sorted(seen.items()):
    par = m.get_base_material().get_path_name() if hasattr(m, "get_base_material") else ""
    line = "%s  <%s>  [%s]" % (p, type(m).__name__, par)
    try:
        tn = MEL.get_texture_parameter_names(m)
        line += "\n   T: " + ", ".join("%s=%s" % (n, (MEL.get_material_instance_texture_parameter_value(m, n).get_path_name() if isinstance(m, unreal.MaterialInstance) and MEL.get_material_instance_texture_parameter_value(m, n) else "-")) for n in tn)
        vn = MEL.get_vector_parameter_names(m)
        line += "\n   V: " + ", ".join("%s=%s" % (n, (MEL.get_material_instance_vector_parameter_value(m, n) if isinstance(m, unreal.MaterialInstance) else "")) for n in vn)
        sn = MEL.get_scalar_parameter_names(m)
        line += "\n   S: " + ", ".join(str(n) for n in sn)
        if not isinstance(m, unreal.MaterialInstance):
            line += "\n   used: " + ", ".join(t.get_path_name() for t in MEL.get_used_textures(m))
    except Exception as e:
        line += " ERR %s" % e
    out.append(line)
open(OUT, "w", encoding="utf-8").write("\n".join(out) + "\n")
