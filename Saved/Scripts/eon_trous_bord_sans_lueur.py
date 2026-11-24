# EON — test : retire la lueur du bord des trous (branche Emissive du toon) pour isoler la panne des trous (24/11).
import unreal, os
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
L = []
for e in MEL.get_material_expressions(m):
    if e.get_class().get_name() == "MaterialExpressionScalarParameter" and str(e.get_editor_property("parameter_name")) == "TrouBordLueur":
        for x in MEL.get_material_expressions(m):
            if x.get_class().get_name() == "MaterialExpressionMultiply" and e in list(MEL.get_inputs_for_material_expression(m, x)):
                MEL.delete_material_expression(m, x); L.append("multiply lueur retiré")
        MEL.delete_material_expression(m, e); L.append("TrouBordLueur retiré")
toon = [e for e in MEL.get_material_expressions(m) if e.get_class().get_name() == "MaterialExpressionSubstrateToonBSDF"][0]
L.append("entrées toon : %s" % [x.get_class().get_name() if x else None for x in MEL.get_inputs_for_material_expression(m, toon)])
MEL.recompile_material(m)
L.append("sauvé : %s" % unreal.EditorAssetLibrary.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_trous_bord_sans_lueur.txt"), "w", encoding="utf-8").write("\n".join(L))
