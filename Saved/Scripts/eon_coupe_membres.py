# EON — membres coupés (Vincent, 25/11 : « découper les parties qui ne sont plus attachées et les faire tomber en rigid body »).
# Méthode du plan de coupe, sur M_EON_ToonEnnemi :
#   - Coupe0..5A = (J.xyz, R) : point de coupe (articulation) et rayon du membre, espace du maillage avant skinning ;
#     Coupe0..5B = (axe.xyz, L) : axe du membre et longueur ; L = 0 → coupe inactive ;
#     Coupe0..5C = (N.xyz) : normale du plan de coupe, inclinable jusqu'à 60° de l'axe (têtes, tentacules) ; nulle → l'axe.
#   - Membre k = points du côté N du plan passant par J, avec dot(P - J, axe) < L et distance à l'axe < R.
#   - Morceau = 0 : le corps cache tous les membres coupés. Morceau = k (1..6) : la copie qui tombe ne garde que le membre k.
#   Le résultat multiplie le masque d'opacité existant (les trous).
# Idempotent. Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
N = 6
CODE = r"""
float4 A[6] = { A0, A1, A2, A3, A4, A5 };
float4 B[6] = { B0, B1, B2, B3, B4, B5 };
float4 C[6] = { C0, C1, C2, C3, C4, C5 };
int k = (int)round(Morceau);
float dedans = 0;
float garde = 1;
for (int i = 0; i < 6; i++) {
  if (B[i].w > 0) {
    float3 d = P - A[i].xyz;
    float t = dot(d, B[i].xyz);
    float3 n = dot(C[i].xyz, C[i].xyz) > 0.01 ? C[i].xyz : B[i].xyz;
    float in_ = (dot(d, n) > 0 && t < B[i].w && length(d - t * B[i].xyz) < A[i].w) ? 1 : 0;
    if (k == 0 && in_ > 0) { garde = 0; }
    if (k == i + 1) { dedans = in_; }
  }
}
return k == 0 ? garde : dedans;
"""
m = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
L.append("blend %s" % m.get_editor_property("blend_mode"))
ex = MEL.get_material_expressions(m)
par = {}
for e in ex:
    try: par[str(e.get_editor_property("parameter_name"))] = e
    except Exception: pass
interp = [e for e in ex if e.get_class().get_name() == "MaterialExpressionVertexInterpolator"][0]
for i in range(N):
    for s in "ABC":
        n = "Coupe%d%s" % (i, s)
        if n not in par:
            v = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1500, 1000 + (i * 3 + "ABC".index(s)) * 70)
            v.set_editor_property("parameter_name", n); v.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 0))
            par[n] = v
if "Morceau" not in par:
    v = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1500, 1900)
    v.set_editor_property("parameter_name", "Morceau"); v.set_editor_property("default_value", 0.0)
    par["Morceau"] = v
cu = [e for e in ex if e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Coupes"]
neuf = not cu
if neuf:
    cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -700, 1300)
    cu.set_editor_property("description", "EON_Coupes")
else:
    cu = cu[0]
cu.set_editor_property("code", CODE)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
arr = []
for n in ["P"] + ["A%d" % i for i in range(N)] + ["B%d" % i for i in range(N)] + ["C%d" % i for i in range(N)] + ["Morceau"]:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); arr.append(ci)
cu.set_editor_property("inputs", arr)
ok = [MEL.connect_material_expressions(interp, "", cu, "P")]
for i in range(N):
    ok.append(MEL.connect_material_expressions(par["Coupe%dA" % i], "RGBA", cu, "A%d" % i))
    ok.append(MEL.connect_material_expressions(par["Coupe%dB" % i], "RGBA", cu, "B%d" % i))
    ok.append(MEL.connect_material_expressions(par["Coupe%dC" % i], "RGBA", cu, "C%d" % i))
ok.append(MEL.connect_material_expressions(par["Morceau"], "", cu, "Morceau"))
L.append("EON_Coupes liens %d/%d" % (sum(ok), len(ok)))
if neuf:
    src = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_OPACITY_MASK)
    L.append("masque actuel %s" % (src.get_class().get_name() if src else None))
    mu = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 1300)
    out = MEL.get_material_property_input_node_output_name(m, unreal.MaterialProperty.MP_OPACITY_MASK) if src else ""
    L.append("A %s" % (MEL.connect_material_expressions(src, out or "", mu, "A") if src else "pas de source"))
    if not src: mu.set_editor_property("const_a", 1.0)
    L.append("B %s" % MEL.connect_material_expressions(cu, "", mu, "B"))
    L.append("masque %s" % MEL.connect_material_property(mu, "", unreal.MaterialProperty.MP_OPACITY_MASK))
MEL.recompile_material(m)
L.append("sauvé %s" % unreal.EditorAssetLibrary.save_loaded_asset(m))
open(os.path.join(unreal.Paths.project_saved_dir(), "coupe_membres.txt"), "w", encoding="utf-8").write("\n".join(L))
