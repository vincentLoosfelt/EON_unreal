# EON — veines JIR-V24, réglage du 24/11 après rendu de contrôle : traits plus fins (pow 8), lueur 0,6 (au lieu de 3,
# qui donnait un marbré rose saturé). Rien d'autre ne change.
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonMasked")
for e in MEL.get_material_expressions(m):
    n = e.get_class().get_name()
    if n == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Veines":
        e.set_editor_property("code", e.get_editor_property("code").replace("pow(saturate(t), 5)", "pow(saturate(t), 8)")); L.append("pow 8")
    if n == "MaterialExpressionScalarParameter" and str(e.get_editor_property("parameter_name")) == "VeinesLueur":
        e.set_editor_property("default_value", 0.6); L.append("lueur 0.6")
MEL.recompile_material(m)
L.append("sauvé : %s" % unreal.EditorAssetLibrary.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_fx_abs_veines3.txt"), "w", encoding="utf-8").write("\n".join(L))
