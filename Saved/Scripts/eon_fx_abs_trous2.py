# EON — trous COM-V9, réglage du 24/11 : une sphère cachée dans le corps ne perçait pas la surface (bornes larges).
# Le trou devient un tube dans l'axe de la caméra (TrouAxe, repère local du maillage) : il traverse le corps
# et se voit toujours depuis la vue de jeu, comme le trou du sprite dans le proto.
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
CODE = r"""
float4 T[8] = { T0, T1, T2, T3, T4, T5, T6, T7 };
float3 a = normalize(Axe + float3(0, 0, 1e-5));
float m = 1;
for (int i = 0; i < 8; i++) {
  if (T[i].w > 0) { float3 d = P - T[i].xyz; d -= dot(d, a) * a; if (length(d) < T[i].w) { m = 0; } }
}
return m;
"""
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
ex = MEL.get_material_expressions(m)
ct = [e for e in ex if e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Trous"][0]
names = [str(i.get_editor_property("input_name")) for i in ct.get_editor_property("inputs")]
if "Axe" not in names:
    arr = list(ct.get_editor_property("inputs"))
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", "Axe"); arr.append(ci)
    ct.set_editor_property("inputs", arr)
ct.set_editor_property("code", CODE)
v = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1200, 1600)
v.set_editor_property("parameter_name", "TrouAxe"); v.set_editor_property("default_value", unreal.LinearColor(0, 0, 1, 0))
L.append("Axe : %s" % MEL.connect_material_expressions(v, "RGB", ct, "Axe"))
# les autres entrées doivent rester branchées (le changement de la liste peut les défaire) : on vérifie
ins = list(MEL.get_inputs_for_material_expression(m, ct))
L.append("entrées : %s" % [i.get_class().get_name() if i else None for i in ins])
MEL.recompile_material(m)
L.append("sauvé : %s" % unreal.EditorAssetLibrary.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_fx_abs_trous2.txt"), "w", encoding="utf-8").write("\n".join(L))
