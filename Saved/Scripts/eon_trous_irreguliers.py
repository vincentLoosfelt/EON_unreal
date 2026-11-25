# EON — trous des ennemis moins réguliers + flash de coup à la couleur du sang (Vincent, 25/11).
#   - EON_Trous / EON_TrouBord : le rayon du tube varie avec l'angle autour de son axe (somme de sinus, phase tirée de la
#     position du trou) : bord déchiqueté, différent pour chaque trou, de 0,5 à 1,2 fois le rayon.
#   - 16 trous (Trou0..Trou15, Vincent 25/11 : « fusion et 16 emplacements ») : les entrées des 2 nœuds sont refaites et
#     rebranchées (P, T0..T15, Axe ; Mini, Frac pour le bord).
#   - FlashCouleur (rgb = couleur, a = force 0..1) : la couleur de base glisse vers elle. Remplace l'ancien flash qui
#     changeait tout le matériau (blanc ×6, et qui effaçait les trous pendant le coup).
# Idempotent. Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
RAYON = r"""
float3 eu = normalize(cross(a, abs(a.z) < 0.9 ? float3(0, 0, 1) : float3(1, 0, 0)));
float3 ev = cross(a, eu);
#define RIRR(t, dd) (t.w * (0.84 + 0.17 * sin(3 * atan2(dot(dd, ev), dot(dd, eu)) + frac(sin(dot(t.xyz, float3(12.99, 78.23, 37.72))) * 43758.5) * 6.28) \
  + 0.11 * sin(5 * atan2(dot(dd, ev), dot(dd, eu)) + frac(sin(dot(t.xyz, float3(39.35, 11.13, 83.17))) * 24634.6) * 6.28) \
  + 0.07 * sin(11 * atan2(dot(dd, ev), dot(dd, eu)) + frac(sin(dot(t.xyz, float3(7.71, 51.9, 23.3))) * 15731.7) * 6.28)))
"""
TROUS = r"""
float4 T[16] = { T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 };
float3 a = normalize(Axe + float3(0, 0, 1e-5));
""" + RAYON + r"""
float m = 1;
for (int i = 0; i < 16; i++) {
  if (T[i].w > 0) { float3 d = P - T[i].xyz; d -= dot(d, a) * a; if (length(d) < RIRR(T[i], d)) { m = 0; } }
}
return m;
"""
BORD = r"""
float4 T[16] = { T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 };
float3 a = normalize(Axe + float3(0, 0, 1e-5));
""" + RAYON + r"""
float b = 0;
for (int i = 0; i < 16; i++) {
  if (T[i].w > 0) {
    float3 d = P - T[i].xyz; d -= dot(d, a) * a;
    float r = length(d); float R = RIRR(T[i], d); float w = max(Mini, T[i].w * Frac);
    if (r >= R && r < R + w) { b = max(b, 1 - (r - R) / w * 0.6); }
  }
}
return b;
"""
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
ex = MEL.get_material_expressions(m)
par = {}
for e in ex:
    try: par[str(e.get_editor_property("parameter_name"))] = e
    except Exception: pass
interp = [e for e in ex if e.get_class().get_name() == "MaterialExpressionVertexInterpolator"][0]
for i in range(16):
    if "Trou%d" % i not in par:
        v = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1200, 1000 + i * 70)
        v.set_editor_property("parameter_name", "Trou%d" % i); v.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 0))
        par["Trou%d" % i] = v
for e in ex:
    if e.get_class().get_name() != "MaterialExpressionCustom": continue
    d = e.get_editor_property("description")
    if d not in ("EON_Trous", "EON_TrouBord"): continue
    noms = ["P"] + ["T%d" % i for i in range(16)] + ["Axe"] + (["Mini", "Frac"] if d == "EON_TrouBord" else [])
    arr = []
    for n in noms:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); arr.append(ci)
    e.set_editor_property("inputs", arr)
    e.set_editor_property("code", TROUS if d == "EON_Trous" else BORD)
    ok = [MEL.connect_material_expressions(interp, "", e, "P")]
    for i in range(16): ok.append(MEL.connect_material_expressions(par["Trou%d" % i], "RGBA", e, "T%d" % i))
    ok.append(MEL.connect_material_expressions(par["TrouAxe"], "RGB", e, "Axe"))
    if d == "EON_TrouBord":
        ok.append(MEL.connect_material_expressions(par["TrouBordMin"], "", e, "Mini"))
        ok.append(MEL.connect_material_expressions(par["TrouBord"], "", e, "Frac"))
    L.append("%s : 16 trous, liens %d/%d" % (d, sum(ok), len(ok)))
# Flash : lerp(couleur de base, FlashCouleur.rgb, FlashCouleur.a) avant le toon
if any(e.get_class().get_name() == "MaterialExpressionVectorParameter" and str(e.get_editor_property("parameter_name")) == "FlashCouleur" for e in ex):
    L.append("flash déjà posé")
else:
    toon = [e for e in ex if e.get_class().get_name() == "MaterialExpressionSubstrateToonBSDF"][0]
    pin = list(MEL.get_material_expression_input_names(toon))[0]
    src = MEL.get_inputs_for_material_expression(m, toon)[0]
    fc = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -500, 600)
    fc.set_editor_property("parameter_name", "FlashCouleur"); fc.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 0))
    lp = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -250, 400)
    L.append("A %s" % MEL.connect_material_expressions(src, "", lp, "A"))
    L.append("B %s" % MEL.connect_material_expressions(fc, "RGB", lp, "B"))
    L.append("Alpha %s" % MEL.connect_material_expressions(fc, "A", lp, "Alpha"))
    L.append("toon.%s %s" % (pin, MEL.connect_material_expressions(lp, "", toon, pin)))
MEL.recompile_material(m)
L.append("sauvé %s" % unreal.EditorAssetLibrary.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_saved_dir(), "trous_irreguliers.txt"), "w", encoding="utf-8").write("\n".join(L))
