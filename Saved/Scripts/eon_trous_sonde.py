# EON — sonde : branchements autour des trous et du toon dans M_EON_ToonEnnemi (avant le bord chair, 24/11).
import unreal, os
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
L = []
for e in MEL.get_material_expressions(m):
    n = e.get_class().get_name()
    d = ""
    try: d = e.get_editor_property("description")
    except Exception: pass
    try: ins = MEL.get_inputs_for_material_expression(m, e); names = list(MEL.get_material_expression_input_names(e))
    except Exception as ex: ins, names = [], ["?%s" % ex]
    pn = ""
    try: pn = str(e.get_editor_property("parameter_name"))
    except Exception: pass
    L.append("%s [%s] %s : %s" % (n, d, pn, ["%s=%s" % (names[i] if i < len(names) else i, x.get_class().get_name() if x else None) for i, x in enumerate(ins)]))
for p in ("MP_BASE_COLOR", "MP_EMISSIVE_COLOR", "MP_OPACITY_MASK", "MP_FRONT_MATERIAL"):
    try:
        x = MEL.get_material_property_input_node(m, getattr(unreal.MaterialProperty, p))
        L.append("%s <- %s / %s" % (p, x.get_class().get_name() if x else None, MEL.get_material_property_input_node_output_name(m, getattr(unreal.MaterialProperty, p))))
    except Exception as ex: L.append("%s ? %s" % (p, ex))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_trous_sonde.txt"), "w", encoding="utf-8").write("\n".join(L))
