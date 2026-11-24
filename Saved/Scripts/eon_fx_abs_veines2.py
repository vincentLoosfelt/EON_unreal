# EON — veines JIR-V24 lisibles depuis la caméra de jeu (24/11) : traits plus épais (pow 5), motif 2× plus grand,
# et lueur rouge (EmissiveColor du Toon) pour qu'elles se voient sur une Jira de ~80 px à l'écran.
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
L = []
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonMasked")
ex = MEL.get_material_expressions(m)
cv = [e for e in ex if e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Veines"][0]
code = cv.get_editor_property("code").replace("pow(saturate(t), 14)", "pow(saturate(t), 5)")
cv.set_editor_property("code", code)
L.append("code : %s" % ("pow(saturate(t), 5)" in code))
for e in ex:
    if e.get_class().get_name() == "MaterialExpressionScalarParameter" and str(e.get_editor_property("parameter_name")) == "VeinesEchelle":
        e.set_editor_property("default_value", 0.045); L.append("échelle 0.045")
toon = [e for e in ex if e.get_class().get_name() == "MaterialExpressionSubstrateToonBSDF"][0]
names = list(MEL.get_material_expression_input_names(toon))
ins = list(MEL.get_inputs_for_material_expression(m, toon))
L.append("entrées toon : %s" % list(zip(names, [i.get_class().get_name() if i else None for i in ins])))
ie = names.index("EmissiveColor")
if ins[ie] is None:
    col = [e for e in ex if e.get_class().get_name() == "MaterialExpressionVectorParameter" and str(e.get_editor_property("parameter_name")) == "VeinesCouleur"][0]
    k = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, 650)
    MEL.connect_material_expressions(cv, "", k, "A"); MEL.connect_material_expressions(col, "", k, "B")
    k2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 650)
    MEL.connect_material_expressions(k, "", k2, "A")
    g = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 750)
    g.set_editor_property("parameter_name", "VeinesLueur"); g.set_editor_property("default_value", 3.0)
    MEL.connect_material_expressions(g, "", k2, "B")
    L.append("lueur -> EmissiveColor : %s" % MEL.connect_material_expressions(k2, "", toon, "EmissiveColor"))
else:
    L.append("EmissiveColor déjà branché : %s" % ins[ie].get_class().get_name())
MEL.recompile_material(m)
L.append("sauvé : %s" % EAL.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_fx_abs_veines2.txt"), "w", encoding="utf-8").write("\n".join(L))
