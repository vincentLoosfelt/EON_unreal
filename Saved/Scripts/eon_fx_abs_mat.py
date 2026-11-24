# EON — matériaux des effets « absents » (Vincent, 24/11 : coder tous les effets ABS du PDF des effets spéciaux).
#   M_EON_Glow (nouveau) : non éclairé, additif, double face. Une seule formule (nœud Custom) dessine toutes les formes
#     sur un plan UV 0..1 : 0 uni, 1 rond doux, 2 traînée (dégradé vers +U), 3 anneau fin, 4 disque d'annonce,
#     5 cône d'annonce (axe +U, demi-angle Angle en rad), 6 ligne d'annonce, 7 flaque (bord irrégulier, Graine).
#     Paramètres : Color, Intensite, Opacite, Forme, Angle, Remplissage (0..1, partie pleine depuis le centre),
#     Clignote (Hz, 0 = fixe), Graine.
#   M_EON_ToonMasked (Jira) : veines de mutation JIR-V24 = bruit en ligne de crête sur la position avant skinning,
#     mélangé vers VeinesCouleur selon Veines (0..1) et une pulsation (VeinesVitesse).
#   M_EON_ToonEnnemi (ennemis) : vrais trous COM-V9 = masque d'opacité ; Trou0..Trou7 (xyz position avant skinning en cm,
#     w rayon en cm ; w = 0 : pas de trou). Le matériau passe de opaque à masqué.
# Lancer : UnrealEditor-Cmd EON.uproject -run=pythonscript -script=<ce fichier>
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
L = []
OUT = os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_fx_abs_mat.txt")


def save():
    open(OUT, "w", encoding="utf-8").write("\n".join(L))


def custom(m, code, ins, x, y, out_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1, desc="EON"):
    c = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", out_type)
    c.set_editor_property("description", desc)
    arr = []
    for n in ins:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        arr.append(ci)
    c.set_editor_property("inputs", arr)
    return c


def scal(m, name, val, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", val)
    return e


def vec(m, name, val, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", val)
    return e


def link(a, b, pin):
    ok = MEL.connect_material_expressions(a, "", b, pin)
    if not ok:
        L.append("  ECHEC lien %s -> %s.%s" % (a.get_class().get_name(), b.get_class().get_name(), pin))
    return ok


# ---------------------------------------------------------------- M_EON_Glow
GLOW = r"""
float2 p = UV * 2 - 1;
float r = length(p);
float ang = abs(atan2(p.y, p.x));
float a = 0;
float f = Forme;
if (f < 0.5) { a = 1; }
else if (f < 1.5) { a = saturate(1 - r); a = a * a; }
else if (f < 2.5) { a = pow(saturate(UV.x), 1.6) * saturate(1 - abs(p.y)); }
else if (f < 3.5) { a = saturate(1 - abs(r - 0.88) / 0.1); }
else if (f < 4.5) { a = r < 1 ? (r < Remplissage ? 0.55 : 0.22) + 0.78 * step(0.88, r) : 0; }
else if (f < 5.5) { a = (r < 1 && ang < Angle) ? (r < Remplissage ? 0.55 : 0.22) + 0.78 * saturate(step(0.9, r) + step(Angle - 0.07, ang)) : 0; }
else if (f < 6.5) { float e = max(step(0.8, abs(p.y)), step(0.96, abs(p.x))); a = (UV.x < Remplissage ? 0.55 : 0.22) + 0.78 * e; }
else {
  float th = atan2(p.y, p.x);
  float n = sin(th * 5 + Graine) * 0.07 + sin(th * 9 + Graine * 2.3) * 0.05 + sin(th * 3 + Graine * 0.7) * 0.06;
  float rr = 0.82 + n;
  a = r < rr ? 0.45 + 0.4 * smoothstep(rr - 0.12, rr, r) : 0;
}
if (Clignote > 0) { a *= 0.45 + 0.55 * step(0, sin(Temps * Clignote * 6.2832)); }
return a;
"""
P = "/Game/EON/Materials/M_EON_Glow"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
m = AT.create_asset("M_EON_Glow", "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("two_sided", True)
for flag in ("used_with_skeletal_mesh", "used_with_static_lighting"):
    try:
        m.set_editor_property(flag, True)
    except Exception as ex:
        L.append("flag %s : %s" % (flag, ex))
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
tm = MEL.create_material_expression(m, unreal.MaterialExpressionTime, -1200, 300)
c = custom(m, GLOW, ["UV", "Forme", "Angle", "Remplissage", "Clignote", "Graine", "Temps"], -700, 0)
link(uv, c, "UV")
link(scal(m, "Forme", 0.0, -1000, 80), c, "Forme")
link(scal(m, "Angle", 0.6, -1000, 140), c, "Angle")
link(scal(m, "Remplissage", 0.0, -1000, 200), c, "Remplissage")
link(scal(m, "Clignote", 0.0, -1000, 260), c, "Clignote")
link(scal(m, "Graine", 0.0, -1000, 320), c, "Graine")
link(tm, c, "Temps")
col = vec(m, "Color", unreal.LinearColor(1, 1, 1, 1), -700, -250)
k = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, -100)
link(col, k, "A"); link(c, k, "B")
k2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, -100)
link(k, k2, "A"); link(scal(m, "Intensite", 1.0, -400, 60), k2, "B")
k3 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, -100)
link(k2, k3, "A"); link(scal(m, "Opacite", 1.0, -250, 60), k3, "B")
L.append("Glow emissif : %s" % MEL.connect_material_property(k3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
MEL.recompile_material(m)
L.append("M_EON_Glow : %s" % EAL.save_loaded_asset(m))
save()


def find(mat, cls_name):
    return [e for e in MEL.get_material_expressions(mat) if e.get_class().get_name() == cls_name]


def pins(e):
    try:
        return list(MEL.get_material_expression_input_names(e))
    except Exception as ex:
        return ["? %s" % ex]


# ---------------------------------------------------------------- Veines de Jira (M_EON_ToonMasked)
VEINES = r"""
float t = 1 - abs(N);
float v = pow(saturate(t), 14);
float pulse = 0.7 + 0.3 * sin(Temps * Vitesse);
return saturate(v * Veines * pulse * 1.6);
"""
jm = unreal.load_asset("/Game/EON/Materials/M_EON_ToonMasked")
toon = find(jm, "MaterialExpressionSubstrateToonBSDF")
mul = find(jm, "MaterialExpressionMultiply")
L.append("ToonMasked : toon %d, multiply %d, entrées toon %s" % (len(toon), len(mul), pins(toon[0]) if toon else None))
if any(e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Veines" for e in MEL.get_material_expressions(jm)):
    L.append("veines déjà posées")
elif toon and mul:
    T, Mu = toon[0], mul[0]
    names = pins(T)
    base_pin = next((n for n in names if "base" in n.lower() or "albedo" in n.lower() or "color" in n.lower()), names[0] if names else "")
    psp = MEL.create_material_expression(jm, unreal.MaterialExpressionPreSkinnedPosition, -1600, 700)
    sc = MEL.create_material_expression(jm, unreal.MaterialExpressionMultiply, -1400, 700)
    link(psp, sc, "A"); link(scal(jm, "VeinesEchelle", 0.09, -1600, 800), sc, "B")
    nz = MEL.create_material_expression(jm, unreal.MaterialExpressionNoise, -1200, 700)
    nz.set_editor_property("output_min", -1.0)
    nz.set_editor_property("output_max", 1.0)
    nz.set_editor_property("levels", 3)
    nz.set_editor_property("turbulence", False)
    link(sc, nz, "Position")
    tm = MEL.create_material_expression(jm, unreal.MaterialExpressionTime, -1200, 900)
    cv = custom(jm, VEINES, ["N", "Veines", "Vitesse", "Temps"], -900, 700, desc="EON_Veines")
    link(nz, cv, "N"); link(scal(jm, "Veines", 0.0, -1200, 1000), cv, "Veines")
    link(scal(jm, "VeinesVitesse", 6.0, -1200, 1080), cv, "Vitesse"); link(tm, cv, "Temps")
    lp = MEL.create_material_expression(jm, unreal.MaterialExpressionLinearInterpolate, -600, 400)
    link(Mu, lp, "A"); link(vec(jm, "VeinesCouleur", unreal.LinearColor(0.85, 0.02, 0.08, 1), -900, 500), lp, "B"); link(cv, lp, "Alpha")
    L.append("veines -> toon.%s : %s" % (base_pin, link(lp, T, base_pin)))
    MEL.recompile_material(jm)
    L.append("M_EON_ToonMasked : %s" % EAL.save_loaded_asset(jm))
save()

# ---------------------------------------------------------------- Trous des ennemis (M_EON_ToonEnnemi)
TROUS = r"""
float4 T[8] = { T0, T1, T2, T3, T4, T5, T6, T7 };
float m = 1;
for (int i = 0; i < 8; i++) { if (T[i].w > 0 && distance(P, T[i].xyz) < T[i].w) { m = 0; } }
return m;
"""
em = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")
if any(e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == "EON_Trous" for e in MEL.get_material_expressions(em)):
    L.append("trous déjà posés")
else:
    em.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    em.set_editor_property("opacity_mask_clip_value", 0.5)
    psp = MEL.create_material_expression(em, unreal.MaterialExpressionPreSkinnedPosition, -1400, 900)
    ct = custom(em, TROUS, ["P", "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7"], -900, 900, desc="EON_Trous")
    link(psp, ct, "P")
    for i in range(8):
        v = vec(em, "Trou%d" % i, unreal.LinearColor(0, 0, 0, 0), -1200, 1000 + i * 70)
        if not MEL.connect_material_expressions(v, "RGBA", ct, "T%d" % i):
            L.append("  ECHEC lien Trou%d RGBA" % i)
    L.append("trous -> masque : %s" % MEL.connect_material_property(ct, "", unreal.MaterialProperty.MP_OPACITY_MASK))
    MEL.recompile_material(em)
    L.append("M_EON_ToonEnnemi : %s" % EAL.save_loaded_asset(em))
save()
