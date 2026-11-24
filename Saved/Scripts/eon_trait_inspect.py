import unreal
MEL = unreal.MaterialEditingLibrary
L = []
m = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")
L.append("domain=%s loc=%s nexpr=%s" % (m.material_domain, m.blendable_location, MEL.get_num_material_expressions(m)))
cu = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
L.append("emissive node=%s" % (cu.get_class().get_name() if cu else None))
if cu:
    L.append("inputs=%s" % [str(i.get_editor_property("input_name")) for i in cu.get_editor_property("inputs")])
    for i in MEL.get_inputs_for_material_expression(m, cu) if hasattr(MEL, "get_inputs_for_material_expression") else []:
        L.append("  in %s" % (i.get_class().get_name() if i else None))
    L.append("SW in code=%s" % ("float SW" in cu.get_editor_property("code")))
mi = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Trait")
L.append("mi parent=%s" % mi.parent.get_path_name())
st = MEL.get_statistics(m) if hasattr(MEL, "get_statistics") else None
L.append("stats=%s" % st)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_trait_inspect.txt", "w", encoding="utf-8").write("\n".join(L))
