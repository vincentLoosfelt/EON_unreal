# EON — liste les nœuds de M_UnityLit et M_EON_ToonMasked (classe, paramètre, entrées branchées).
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
for p in ("/Game/EON/Unity/Materials/M_UnityLit", "/Game/EON/Materials/M_EON_ToonMasked"):
    m = unreal.load_asset(p)
    if not m:
        L.append("%s introuvable" % p); continue
    L.append("== %s : blend %s, double face %s" % (p, m.get_editor_property("blend_mode"), m.get_editor_property("two_sided")))
    for e in m.get_editor_property("expressions") if hasattr(m, "expressions") else []:
        pass
    try:
        exprs = MEL.get_material_expressions(m) if hasattr(MEL, "get_material_expressions") else []
    except Exception as ex:
        exprs = []; L.append("  (liste des nœuds indisponible : %s)" % ex)
    for e in exprs:
        nm = e.get_editor_property("parameter_name") if hasattr(e, "parameter_name") else ""
        ins = []
        try:
            ins = [x.get_class().get_name() + ":" + (str(x.get_editor_property("parameter_name")) if hasattr(x, "parameter_name") else "") for x in MEL.get_inputs_for_material_expression(m, e) if x]
        except Exception:
            pass
        L.append("  %s %s <- %s" % (e.get_class().get_name(), nm, ins))
    for name in MEL.get_texture_parameter_names(m):
        L.append("  tex %s" % name)
    for name in MEL.get_vector_parameter_names(m):
        L.append("  vec %s = %s" % (name, MEL.get_material_default_vector_parameter_value(m, name)))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_diag_graphes.txt"), "w", encoding="utf-8").write("\n".join(L))
