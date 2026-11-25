# EON — dôme / boule de charge des explosions façon Akira (Vincent, 24/11 : « une petite boule qui grossit légèrement
# sur 2 secondes puis explose drastiquement »). Lancer : UnrealEditor-Cmd EON.uproject -run=pythonscript -script=<ce fichier>
#
#   M_EON_Dome : non éclairé, masqué, posé sur l'icosphère SM_EON_FumeeSphere par EONFx::Charge (AEONFxPiece).
#     Âge = 1 - Opacite (AEONFxPiece). La boule de charge garde Opacite à 1 : cœur blanc, fin bord orange.
#     Couleur : cœur blanc aveuglant, bord coloré (seuil franc sur le fresnel) qui envahit la boule en vieillissant.
#     Surface en bulles (bruit 3D world space fixe, comme la fumée), érosion à bords nets à la fin (pas de fondu).
#     Émission × EyeAdaptationInverse : la couleur à l'écran ne dépend pas de l'exposition de la salle.
import unreal, os
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
DOS = "/Game/EON/VFX"
log = []


def ok(b, what):
    log.append(("OK   " if b else "ERR  ") + what)


BRUIT = r"""
float r = 0; float amp = 0.5; float3 q = P;
for (int o = 0; o < 3; o++) {
  float3 i = floor(q); float3 f = frac(q); f = f * f * (3 - 2 * f);
  #define H(o3) frac(sin(dot(i + o3, float3(127.1,311.7,74.7))) * 43758.5453)
  float v = lerp(lerp(lerp(H(float3(0,0,0)), H(float3(1,0,0)), f.x), lerp(H(float3(0,1,0)), H(float3(1,1,0)), f.x), f.y),
                 lerp(lerp(H(float3(0,0,1)), H(float3(1,0,1)), f.x), lerp(H(float3(0,1,1)), H(float3(1,1,1)), f.x), f.y), f.z);
  #undef H
  r += v * amp; amp *= 0.5; q = q * 2.03 + 17.1;
}
return r / 0.875;
"""
BULLE_WPO = r"""
float3 q = P * Freq; float3 ic = floor(q); float best = 9.0;
[unroll] for (int x = -1; x <= 1; x++) [unroll] for (int y = -1; y <= 1; y++) [unroll] for (int z = -1; z <= 1; z++) {
  float3 c = ic + float3(x, y, z);
  float3 h = frac(sin(float3(dot(c, float3(127.1, 311.7, 74.7)), dot(c, float3(269.5, 183.3, 246.1)), dot(c, float3(113.5, 271.9, 124.6)))) * 43758.5453);
  best = min(best, length(q - (c + 0.15 + h * 0.7)));
}
float bump = sqrt(saturate(1.0 - (best / Rayon) * (best / Rayon)));
return normalize(N) * bump * Amp;
"""
# Couleur : a = âge 0..1. Bord coloré = fresnel au-dessus d'un seuil qui descend avec l'âge, bruité pour un bord
# irrégulier dessiné. Retour : rgb = couleur × intensité.
COUL = r"""
float a = saturate(Age);
float f = 1.0 - saturate(dot(normalize(N), normalize(V)));
float s = lerp(Seuil, -0.2, saturate(a / Envahit));
float bord = step(s, f + (Bruit - 0.5) * 0.35);
return lerp(Coeur * LumCoeur, Bord * LumBord, bord);
"""
EROS = r"""
float a = saturate(Age);
float k = saturate((a - Debut) / (1.0 - Debut)) * 1.1 - 0.05;
return Bruit - k;
"""

P = DOS + "/M_EON_Dome"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
m = AT.create_asset("M_EON_Dome", DOS, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("opacity_mask_clip_value", 0.0001)
E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)


def scal(name, v, x, y):
    e = E(unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", v); return e


def vecp(name, c, x, y):
    e = E(unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", c); return e


def custom(code, ins, x, y, out=unreal.CustomMaterialOutputType.CMOT_FLOAT1, desc="EON"):
    c = E(unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code); c.set_editor_property("output_type", out); c.set_editor_property("description", desc)
    arr = []
    for n in ins:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); arr.append(ci)
    c.set_editor_property("inputs", arr)
    return c


def link(a, b, pin, out=""):
    r = MEL.connect_material_expressions(a, out, b, pin)
    ok(r, "lien %s -> %s.%s" % (a.get_class().get_name(), b.get_class().get_name(), pin))
    return r


# âge = 1 - Opacite (posé par AEONFxPiece)
op = scal("Opacite", 1.0, -1500, 0)
age = E(unreal.MaterialExpressionOneMinus, -1300, 0)
link(op, age, "")
# bruit sur la position locale (la boule grossit : le dessin du bord grossit avec elle)
wp = E(unreal.MaterialExpressionWorldPosition, -1700, 250)
obj = E(unreal.MaterialExpressionObjectPositionWS, -1700, 350)
loc = E(unreal.MaterialExpressionSubtract, -1500, 300)
link(wp, loc, "A"); link(obj, loc, "B")
osc = E(unreal.MaterialExpressionObjectRadius, -1700, 450)
dv = E(unreal.MaterialExpressionDivide, -1350, 300)
link(loc, dv, "A"); link(osc, dv, "B")
sc = E(unreal.MaterialExpressionMultiply, -1200, 300)
link(dv, sc, "A"); link(scal("BruitEchelle", 3.0, -1350, 400), sc, "B")
nz = custom(BRUIT, ["P"], -1000, 300, desc="EON_DomeBruit")
link(sc, nz, "P")

col = custom(COUL, ["N", "V", "Age", "Bruit", "Seuil", "Envahit", "Coeur", "LumCoeur", "Bord", "LumBord"], -600, -100,
             unreal.CustomMaterialOutputType.CMOT_FLOAT3, "EON_DomeCouleur")
link(E(unreal.MaterialExpressionVertexNormalWS, -900, -300), col, "N")
link(E(unreal.MaterialExpressionCameraVectorWS, -900, -250), col, "V")
link(age, col, "Age"); link(nz, col, "Bruit")
link(scal("BordSeuil", 0.3, -900, -150), col, "Seuil")
link(scal("BordEnvahit", 0.45, -900, -100), col, "Envahit")
link(vecp("Coeur", unreal.LinearColor(1, 1, 1, 1), -900, -50), col, "Coeur", "RGB")
link(scal("LumCoeur", 1.6, -900, 0), col, "LumCoeur")
link(vecp("Bord", unreal.LinearColor(1.0, 0.28, 0.03, 1), -900, 50), col, "Bord", "RGB")
link(scal("LumBord", 1.0, -900, 100), col, "LumBord")
# couleur à l'écran indépendante de l'exposition (salles en exposition physique)
eai = E(unreal.MaterialExpressionEyeAdaptationInverse, -400, 50)
em = E(unreal.MaterialExpressionMultiply, -250, -50)
link(col, em, "A"); link(eai, em, "B")
ok(MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "dôme : émission")

ce = custom(EROS, ["Bruit", "Age", "Debut"], -600, 350, desc="EON_DomeErosion")
link(nz, ce, "Bruit"); link(age, ce, "Age"); link(scal("ErosionDebut", 0.5, -900, 400), ce, "Debut")
ok(MEL.connect_material_property(ce, "", unreal.MaterialProperty.MP_OPACITY_MASK), "dôme : masque")

wpa = E(unreal.MaterialExpressionWorldPosition, -1200, 700)
try:
    wpa.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
except Exception as ex:
    log.append("ERR  position sans offset : %s" % ex)
wpo = custom(BULLE_WPO, ["N", "P", "Freq", "Rayon", "Amp"], -600, 650, unreal.CustomMaterialOutputType.CMOT_FLOAT3, "EON_DomeBulles")
link(E(unreal.MaterialExpressionVertexNormalWS, -900, 600), wpo, "N"); link(wpa, wpo, "P")
link(scal("BulleFreq", 0.022, -900, 700), wpo, "Freq"); link(scal("BulleRayon", 0.85, -900, 760), wpo, "Rayon")
link(scal("BulleAmp", 16.0, -900, 820), wpo, "Amp")
ok(MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET), "dôme : bulles")

MEL.recompile_material(m)
ok(EAL.save_loaded_asset(m), "M_EON_Dome")
open(os.path.join(unreal.Paths.project_saved_dir(), "dome_build.txt"), "w", encoding="utf-8").write(chr(10).join(log))
