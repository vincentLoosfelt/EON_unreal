# EON — COM-V9, bord chair des trous (Vincent, 24/11 : « bord chair rouge ») dans M_EON_ToonEnnemi.
# Même tube que EON_Trous (axe caméra) : anneau de largeur max(TrouBordMin cm, TrouBord × rayon) autour de chaque trou,
# mélangé vers TrouBordCouleur dans la couleur du toon, avec une faible lueur (TrouBordLueur) pour rester lisible à l'ombre.
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
CODE = r"""
float4 T[8] = { T0, T1, T2, T3, T4, T5, T6, T7 };
float3 a = normalize(Axe + float3(0, 0, 1e-5));
float b = 0;
for (int i = 0; i < 8; i++) {
  if (T[i].w > 0) {
    float3 d = P - T[i].xyz; d -= dot(d, a) * a;
    float r = length(d); float w = max(Mini, T[i].w * Frac);
    if (r >= T[i].w && r < T[i].w + w) { b = max(b, 1 - (r - T[i].w) / w * 0.6); }
  }
}
return b;
"""
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
ex = MEL.get_material_expressions(m)
def cls(e): return e.get_class().get_name()
if any(cls(e) == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_TrouBord" for e in ex):
    L.append("bord déjà posé")
else:
    tr = [e for e in ex if cls(e) == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Trous"][0]
    toon = [e for e in ex if cls(e) == "MaterialExpressionSubstrateToonBSDF"][0]
    tins = list(MEL.get_inputs_for_material_expression(m, tr))   # P, T0..T7, Axe
    bins = list(MEL.get_inputs_for_material_expression(m, toon))
    base = bins[0]
    c = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -900, 1700)
    c.set_editor_property("code", CODE)
    c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    c.set_editor_property("description", "EON_TrouBord")
    arr = []
    for n in ["P"] + ["T%d" % i for i in range(8)] + ["Axe", "Mini", "Frac"]:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); arr.append(ci)
    c.set_editor_property("inputs", arr)
    L.append("P : %s" % MEL.connect_material_expressions(tins[0], "", c, "P"))
    for i in range(8): L.append("T%d : %s" % (i, MEL.connect_material_expressions(tins[1 + i], "RGBA", c, "T%d" % i)))
    L.append("Axe : %s" % MEL.connect_material_expressions(tins[9], "RGB", c, "Axe"))
    def scal(n, v, y):
        e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1200, y)
        e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", v); return e
    L.append("Mini : %s" % MEL.connect_material_expressions(scal("TrouBordMin", 3.0, 1900), "", c, "Mini"))
    L.append("Frac : %s" % MEL.connect_material_expressions(scal("TrouBord", 0.4, 1960), "", c, "Frac"))
    col = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 1500)
    col.set_editor_property("parameter_name", "TrouBordCouleur"); col.set_editor_property("default_value", unreal.LinearColor(0.42, 0.02, 0.05, 1))
    lp = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -500, 300)
    L.append("lerp A : %s" % MEL.connect_material_expressions(base, "", lp, "A"))
    L.append("lerp B : %s" % MEL.connect_material_expressions(col, "RGB", lp, "B"))
    L.append("lerp Alpha : %s" % MEL.connect_material_expressions(c, "", lp, "Alpha"))
    L.append("toon BaseColor : %s" % MEL.connect_material_expressions(lp, "", toon, "BaseColor"))
    k = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, 1500)
    MEL.connect_material_expressions(col, "RGB", k, "A"); MEL.connect_material_expressions(c, "", k, "B")
    k2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -350, 1500)
    MEL.connect_material_expressions(k, "", k2, "A"); MEL.connect_material_expressions(scal("TrouBordLueur", 0.35, 2020), "", k2, "B")
    L.append("toon Emissive : %s" % MEL.connect_material_expressions(k2, "", toon, "EmissiveColor"))
    L.append("entrées trous après : %s" % [x.get_class().get_name() if x else None for x in MEL.get_inputs_for_material_expression(m, tr)])
    MEL.recompile_material(m)
    L.append("sauvé : %s" % unreal.EditorAssetLibrary.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_trous_bord.txt"), "w", encoding="utf-8").write("\n".join(L))
