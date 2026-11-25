# EON — fumée toon façon Akira (Vincent, 24/11) : sphères Niagara + shader toon 2 tons qui s'érode.
# Lancer : UnrealEditor-Cmd EON.uproject -run=pythonscript -script=<ce fichier>
#
#   M_EON_FumeeToon : toon éclairé (Substrate Toon BSDF, profil TP_EON_Anime), masqué ; couleur = couleur de la particule. Dissipation par érosion : un bruit 3D sur la
#     position locale de la sphère, le seuil monte avec l'âge, les bords restent nets (pas de fondu). Trait : post-process
#     celluloïd (stencil posé par le C++).
#   NS_EON_Fumee : émetteur Lightweight, rendu en sphères. User.Count, User.Color, User.Life. Échelle d'ensemble =
#     échelle du composant (ApplyOwnerScaleToAttributes). Montée lente, turbulence (curl noise), sphères qui gonflent.
import unreal, os
L = unreal.EONVfxLib
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
DOS = "/Game/EON/VFX"
log = []


def ok(b, what):
    log.append(("OK   " if b else "ERR  ") + what)


BRUIT = r"""
// bruit de valeur 3D, 3 octaves, résultat 0..1
float r = 0; float amp = 0.5; float3 q = P;
for (int o = 0; o < 3; o++) {
  float3 i = floor(q); float3 f = frac(q); f = f * f * (3 - 2 * f);
  float n000 = frac(sin(dot(i + float3(0,0,0), float3(127.1,311.7,74.7))) * 43758.5453);
  float n100 = frac(sin(dot(i + float3(1,0,0), float3(127.1,311.7,74.7))) * 43758.5453);
  float n010 = frac(sin(dot(i + float3(0,1,0), float3(127.1,311.7,74.7))) * 43758.5453);
  float n110 = frac(sin(dot(i + float3(1,1,0), float3(127.1,311.7,74.7))) * 43758.5453);
  float n001 = frac(sin(dot(i + float3(0,0,1), float3(127.1,311.7,74.7))) * 43758.5453);
  float n101 = frac(sin(dot(i + float3(1,0,1), float3(127.1,311.7,74.7))) * 43758.5453);
  float n011 = frac(sin(dot(i + float3(0,1,1), float3(127.1,311.7,74.7))) * 43758.5453);
  float n111 = frac(sin(dot(i + float3(1,1,1), float3(127.1,311.7,74.7))) * 43758.5453);
  float v = lerp(lerp(lerp(n000, n100, f.x), lerp(n010, n110, f.x), f.y), lerp(lerp(n001, n101, f.x), lerp(n011, n111, f.x), f.y), f.z);
  r += v * amp; amp *= 0.5; q = q * 2.03 + 17.1;
}
return r / 0.875;
"""
# Displacement (Vincent, 24-25/11 : « toujours trop sphérique et sans displacement »). Deux étages, sur une hauteur H(p) :
#   - grosses bosses en creux ET en relief (bruit de valeur, BosseFreq/BosseAmp), à l'échelle de la sphère : elles cassent
#     la silhouette ronde ;
#   - bulles distinctes (Worley, profil en cloche, BulleFreq/BulleRayon/BulleAmp) par-dessus.
#   Les anciennes bulles en dôme couvraient toute leur cellule : la surface montait partout d'autant, la sphère gonflait
#   sans changer de forme.
# Bruit en world space FIXE (poussière : BulleRotation = 0). Nuage d'explosion : le champ tourne lentement avec chaque
# sphère (rotation autour du centre de la particule, axe et sens au hasard, angle = BulleRotation × âge).
# Normale = celle de la surface déplacée, par différences finies sur H.
DISP = r"""
struct EONBulles {
  float3 PP; float Spin; float Rnd; float Age; float Freq; float Rayon; float Amp; float LFreq; float LAmp;
  float3 Tourne(float3 p) {
    if (Spin <= 0.0) return p;
    float ang = Spin * Age * (0.5 + Rnd) * (frac(Rnd * 7.13) < 0.5 ? -1.0 : 1.0);
    float3 ax = normalize(frac(float3(Rnd * 12.9898, Rnd * 78.233, Rnd * 37.719) * 43758.5453) - 0.5 + 0.001);
    float cs = cos(ang), sn = sin(ang); float3 d = p - PP;
    return PP + d * cs + cross(ax, d) * sn + ax * dot(ax, d) * (1.0 - cs);
  }
  float VN(float3 z) {
    float3 i = floor(z); float3 f = frac(z); f = f * f * (3 - 2 * f);
    #define HH(o3) frac(sin(dot(i + o3, float3(127.1,311.7,74.7))) * 43758.5453)
    float v = lerp(lerp(lerp(HH(float3(0,0,0)), HH(float3(1,0,0)), f.x), lerp(HH(float3(0,1,0)), HH(float3(1,1,0)), f.x), f.y),
                   lerp(lerp(HH(float3(0,0,1)), HH(float3(1,0,1)), f.x), lerp(HH(float3(0,1,1)), HH(float3(1,1,1)), f.x), f.y), f.z);
    #undef HH
    return v;
  }
  float H(float3 p) {
    float3 pr = Tourne(p);
    float l = 0.0;
    if (LAmp > 0.0) l = (VN(pr * LFreq) * 0.65 + VN(pr * LFreq * 2.1 + 5.3) * 0.35 - 0.5) * 2.0 * LAmp;
    float3 q = pr * Freq; float3 ic = floor(q); float best = 9.0;
    [unroll] for (int x = -1; x <= 1; x++) [unroll] for (int y = -1; y <= 1; y++) [unroll] for (int z = -1; z <= 1; z++) {
      float3 c = ic + float3(x, y, z);
      float3 h = frac(sin(float3(dot(c, float3(127.1, 311.7, 74.7)), dot(c, float3(269.5, 183.3, 246.1)), dot(c, float3(113.5, 271.9, 124.6)))) * 43758.5453);
      best = min(best, length(q - (c + 0.15 + h * 0.7)));
    }
    float b = saturate(1.0 - best / Rayon); b = b * b * (3.0 - 2.0 * b);
    return l + b * Amp;
  }
};
EONBulles E;
E.PP = PP; E.Spin = Spin; E.Rnd = Rnd; E.Age = Age; E.Freq = Freq; E.Rayon = Rayon; E.Amp = Amp; E.LFreq = LFreq; E.LAmp = LAmp;
"""
BULLE_WPO = DISP + r"""
return normalize(N) * E.H(P);
"""
# Normale du toon = celle de la surface déplacée, puis courbée vers celle de la MASSE (centre de la bouffée, paramètre
# dynamique 0) + léger bruit world space (Force) : l'ombre se pose sur le volume d'ensemble.
BULLE_N = DISP + r"""
float3 n = normalize(N);
float3 t1 = normalize(cross(n, abs(n.z) < 0.9 ? float3(0, 0, 1) : float3(1, 0, 0)));
float3 t2 = cross(n, t1);
float e = 1.0;
float h0 = E.H(P), h1 = E.H(P + t1 * e), h2 = E.H(P + t2 * e);
float3 nd = normalize(n - t1 * (h1 - h0) / e - t2 * (h2 - h0) / e);
float3 dc = (P - Centre) * float3(1, 1, 1.6);          // masse plus large que haute
float3 nm = length(dc) > 1 ? normalize(dc) : n;
float3 v = float3(0, 0, 0);
float3 z0 = E.Tourne(P) * NFreq;
for (int c = 0; c < 3; c++) v[c] = E.VN(z0 + c * 31.7) - 0.5;
return normalize(lerp(nd, nm, Masse) + v * Force * 2.0);
"""
EROS = r"""
// érosion : le seuil monte avec l'âge (carré : la fumée tient puis se défait vite), bords nets
// (réf. Akira de Vincent : la masse s'ouvre et ses lobes rétrécissent ; les trous n'arrivent qu'à la toute fin)
float a = saturate(Age);
float k = saturate((a - 0.62) / 0.38) * 1.1 - 0.05;
// Fondu de l'ensemble (Vincent, 25/11 : « opaque à la naissance de la boule, transparent à la fin » ; toon 2 tons gardé,
// donc matériau masqué) : trame en espace écran, la MÊME pour toutes les sphères -> l'union du nuage s'éclaircit d'un bloc,
// sans surépaisseur là où elles se chevauchent. Bruit de gradient entrelacé qui change à chaque image (8 phases) :
// l'antialiasing temporel en fait une transparence lisse (une trame fixe se voyait). Les sphères d'une bouffée naissent
// ensemble avec la même vie : leur âge est celui du nuage. FonduMax 0 = opaque (poussière) ; MI_EON_FumeeExplo le règle.
float t = saturate((a - FDebut) / max(1.0 - FDebut, 1e-3)) * FMax;
float2 px = floor(SP * View.ViewSizeAndInvSize.xy) + 5.588238 * float(View.StateFrameIndexMod8);
float ign = frac(52.9829189 * frac(dot(px, float2(0.06711056, 0.00583715))));
return ign < t ? -1.0 : Bruit - k;
"""

P = DOS + "/M_EON_FumeeToon"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
m = AT.create_asset("M_EON_FumeeToon", DOS, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
m.set_editor_property("opacity_mask_clip_value", 0.0001)
for f in ("used_with_niagara_mesh_particles", "used_with_niagara_sprites"):
    try:
        m.set_editor_property(f, True)
    except Exception as ex:
        log.append("flag %s : %s" % (f, ex))
E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)


def scal(name, v, x, y):
    e = E(unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", v); return e


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
    if not r:
        log.append("ERR  lien %s -> %s.%s" % (a.get_class().get_name(), b.get_class().get_name(), pin))
    return r


pc = E(unreal.MaterialExpressionParticleColor, -1400, -200)
rnd = E(unreal.MaterialExpressionParticleRandom, -1400, 550)
wp = E(unreal.MaterialExpressionWorldPosition, -1600, 200)
pp = E(unreal.MaterialExpressionParticlePositionWS, -1600, 300)
loc = E(unreal.MaterialExpressionSubtract, -1400, 250)
link(wp, loc, "A"); link(pp, loc, "B")
# position locale ramenée à l'échelle de la sphère (~ 1 sur le rayon) + décalage aléatoire par particule
sc = E(unreal.MaterialExpressionMultiply, -1250, 250)
link(loc, sc, "A"); link(scal("BruitEchelle", 0.035, -1400, 330), sc, "B")
rv = E(unreal.MaterialExpressionMultiply, -1250, 550)
link(rnd, rv, "A"); rv.set_editor_property("const_b", 37.0)
ad = E(unreal.MaterialExpressionAdd, -1100, 300)
link(sc, ad, "A"); link(rv, ad, "B")
nz = custom(BRUIT, ["P"], -950, 300, desc="EON_FumeeBruit")
link(ad, nz, "P")
# Toon shader éclairé (Vincent, 24/11 : « pas en émissif, un toon shader ») : Substrate Toon BSDF + profil des persos
# TP_EON_Anime (seuil 0,5 franc → 2 tons), lumière de la salle. Couleur = couleur de la particule × Teinte.
toon = E(unreal.MaterialExpressionSubstrateToonBSDF, -300, -150)
toon.set_editor_property("toon_profile", unreal.load_asset("/Game/EON/Materials/TP_EON_Anime"))
tnt = E(unreal.MaterialExpressionVectorParameter, -900, -100)
tnt.set_editor_property("parameter_name", "Teinte"); tnt.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
bc = E(unreal.MaterialExpressionMultiply, -600, -150)
link(pc, bc, "A", "RGB"); link(tnt, bc, "B", "RGB")
for pin in ("Base Color", "BaseColor"):
    if MEL.connect_material_expressions(bc, "", toon, pin):
        log.append("OK   base -> " + pin); break
link(scal("Roughness", 1.0, -600, 50), toon, "Roughness")
link(scal("Specular", 0.0, -600, 110), toon, "Specular")
wpa = E(unreal.MaterialExpressionWorldPosition, -1200, 700)
try:
    wpa.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
except Exception as ex:
    log.append("ERR  position sans offset : %s" % ex)
bfreq = scal("BulleFreq", 0.045, -1200, 800); bray = scal("BulleRayon", 0.85, -1200, 860); bamp = scal("BulleAmp", 9.0, -900, 700)
nw = custom(BULLE_N, ["N", "P", "Freq", "Rayon", "Amp", "Centre", "Masse", "NFreq", "Force", "PP", "Rnd", "Age", "Spin", "LFreq", "LAmp"], -600, 200, unreal.CustomMaterialOutputType.CMOT_FLOAT3, "EON_FumeeBulleNormale")
link(E(unreal.MaterialExpressionVertexNormalWS, -900, 200), nw, "N"); link(wpa, nw, "P")
link(bfreq, nw, "Freq"); link(bray, nw, "Rayon"); link(bamp, nw, "Amp")
dyn = E(unreal.MaterialExpressionDynamicParameter, -1200, 950)      # paramètre dynamique 0 = centre de la bouffée (monde)
link(dyn, nw, "Centre", "RGB") or link(dyn, nw, "Centre")
link(scal("Masse", 0.6, -900, 380), nw, "Masse"); link(scal("NormaleBruitFreq", 0.02, -900, 440), nw, "NFreq")
link(scal("NormaleBruit", 0.3, -900, 500), nw, "Force")
wpo = custom(BULLE_WPO, ["N", "P", "Freq", "Rayon", "Amp", "PP", "Rnd", "Age", "Spin", "LFreq", "LAmp"], -600, 600, unreal.CustomMaterialOutputType.CMOT_FLOAT3, "EON_FumeeBulleWPO")
link(E(unreal.MaterialExpressionVertexNormalWS, -900, 600), wpo, "N"); link(wpa, wpo, "P")
link(bfreq, wpo, "Freq"); link(bray, wpo, "Rayon"); link(bamp, wpo, "Amp")
ok(MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET), "fumée : displacement bulles")
m.set_editor_property("tangent_space_normal", False)
link(nw, toon, "Normal")
ok(MEL.connect_material_property(toon, "", unreal.MaterialProperty.MP_FRONT_MATERIAL), "fumée : toon")
ce = custom(EROS, ["Bruit", "Age", "SP", "FDebut", "FMax"], -600, 350, desc="EON_FumeeErosion")
# âge = 1 - alpha de la couleur (ScaleColor fait descendre l'alpha de 1 à 0 sur la vie ; le temps relatif n'arrive pas
# au matériau avec l'émetteur Lightweight)
age = E(unreal.MaterialExpressionOneMinus, -800, 450)
link(pc, age, "", "A")
link(nz, ce, "Bruit"); link(age, ce, "Age")
link(E(unreal.MaterialExpressionScreenPosition, -900, 300), ce, "SP")
link(scal("FonduDebut", 0.0, -900, 250), ce, "FDebut"); link(scal("FonduMax", 0.0, -900, 200), ce, "FMax")
# rotation des bulles (0 = poussière, bruit world space fixe ; MI_EON_FumeeExplo la règle)
spin = scal("BulleRotation", 0.0, -900, 1000)
for cn in (nw, wpo):
    link(pp, cn, "PP"); link(rnd, cn, "Rnd"); link(age, cn, "Age"); link(spin, cn, "Spin")
# grosses bosses (0 = poussière : bulles seules ; MI_EON_FumeeExplo les règle)
bfq = scal("BosseFreq", 0.012, -900, 1060); bam = scal("BosseAmp", 0.0, -900, 1120)
for cn in (nw, wpo):
    link(bfq, cn, "LFreq"); link(bam, cn, "LAmp")
ok(MEL.connect_material_property(ce, "", unreal.MaterialProperty.MP_OPACITY_MASK), "fumée : masque")
MEL.recompile_material(m)
ok(EAL.save_loaded_asset(m), "M_EON_FumeeToon")

# ---------------------------------------------------------------- systèmes
# Structure (Vincent, 24/11 : « plus de sphères, étudier leur déplacement et leur lien logique ») :
#   NS_EON_Fumee      = noyau : quelques grosses sphères qui gonflent vite puis rétrécissent les premières
#                       (le centre se vide, la masse s'ouvre en anneau comme dans Akira) ;
#   NS_EON_FumeeLobes = des dizaines de lobes nés dans le noyau et poussés radialement depuis un point sous le centre,
#                       freinés : ils roulent vers l'extérieur à la vitesse de la croissance du noyau et restent soudés
#                       à sa surface (chou-fleur), puis durent plus longtemps que lui.
# icosphère dense (5 120 triangles, rayon 50 cm, eon_fumee_sphere_import.py) : les bulles se lisent sur la silhouette
SPHERE = unreal.load_asset("/Game/EON/VFX/SM_EON_FumeeSphere")
# rayon de BasicShapes/Sphere = 50 cm : les tailles du script sont calées dessus
RATIO = 50.0 / max(1.0, SPHERE.get_bounds().sphere_radius)
log.append("maillage %s ratio %.4f" % (SPHERE.get_path_name(), RATIO))
TEMPLATE = unreal.load_asset("/Niagara/DefaultAssets/Templates/Systems/DirectionalBurstLightweight")


def rng(a, b):
    return "(Min=%s,Max=%s,Mode=UniformRange,ChannelConstantsAndRanges=(%s,%s))" % (a, b, a, b)


def cst(a):
    return "(Min=%s,Max=%s,Mode=UniformConstant,ChannelConstantsAndRanges=(%s))" % (a, a, a)


def vec(x, y, z):
    return "(Min=(X=%s,Y=%s,Z=%s),Max=(X=%s,Y=%s,Z=%s),Mode=NonUniformConstant,ChannelConstantsAndRanges=(%s,%s,%s))" % (x, y, z, x, y, z, x, y, z)


def curve(vals):
    return "(Values=(%s),ValuesTimeRange=(X=0,Y=1),Mode=NonUniformCurve)" % ",".join("(X=%s,Y=%s,Z=%s)" % (v, v, v) for v in vals)


def make(NAME, cfg):
    if EAL.does_asset_exist(DOS + "/" + NAME):
        EAL.delete_asset(DOS + "/" + NAME)
    s = AT.duplicate_asset(NAME, DOS, TEMPLATE)
    e = L.get_stateless_emitter(s, 0)
    ok(e is not None, NAME + " émetteur")

    def Pr(p, v):
        ok(L.set_prop(e, p, v), "%s %s = %s" % (NAME, p, v))

    def B(p, user, t):
        ok(L.bind_user(s, e, p, user, t), "%s %s -> User.%s" % (NAME, p, user))

    ok(L.use_mesh_renderer(e, SPHERE, m), NAME + " rendu en sphères")
    Pr("bForceOutputAllAttributes", "True")
    Pr("FixedBounds", "(Min=(X=-700,Y=-700,Z=-200),Max=(X=700,Y=700,Z=900),IsValid=True)")
    Pr("EmitterState.LoopDuration", cst(12))          # boucle unique plus longue que toute vie (fumée d explosion : 4 s)
    B("SpawnInfos[0].Amount", "Count", "int")
    B("Modules[0].LifetimeDistribution", "Life", "float")
    B("Modules[0].ColorDistribution", "Color", "color")
    Pr("Modules[0].MeshScaleDistribution", "(Min=(X=%s,Y=%s,Z=%s),Max=(X=%s,Y=%s,Z=%s),Mode=UniformRange,ChannelConstantsAndRanges=(%s,%s))"
       % tuple([round(cfg["taille"][0] * RATIO, 5)] * 3 + [round(cfg["taille"][1] * RATIO, 5)] * 3 + [round(t * RATIO, 5) for t in cfg["taille"]]))
    Pr("Modules[0].MassDistribution", cst(1))
    Pr("Modules[1].bModuleEnabled", "True")          # orientation au hasard : le bruit ne se répète pas d'une sphère à l'autre
    Pr("Modules[2].bModuleEnabled", "True")
    Pr("Modules[2].ShapePrimitive", "Sphere")
    Pr("Modules[2].SphereRadius", cst(cfg["rayon"]))
    Pr("Modules[2].ShapeScale", vec(1, 1, 0.65))      # masse posée au sol, plus large que haute
    Pr("Modules[3].bModuleEnabled", "True")
    Pr("Modules[3].VelocityType", "FromPoint")
    Pr("Modules[3].PointOrigin", "(X=0,Y=0,Z=%s)" % cfg["origine"])
    Pr("Modules[3].PointVelocityDistribution", rng(*cfg["vitesse"]))
    Pr("Modules[4].bModuleEnabled", "True")          # montée lente
    Pr("Modules[4].AccelerationDistribution", vec(0, 0, cfg["montee"]))
    Pr("Modules[5].bModuleEnabled", "True")          # turbulence
    Pr("Modules[5].NoiseStrength", str(cfg["turb"]))
    Pr("Modules[5].NoiseFrequency", "120")
    Pr("Modules[6].bModuleEnabled", "True")
    Pr("Modules[6].DragDistribution", cst(cfg["frein"]))
    Pr("Modules[7].bModuleEnabled", "False")
    Pr("Modules[8].bModuleEnabled", "True")          # échelle du composant = taille d'ensemble
    Pr("Modules[16].bModuleEnabled", "True")         # alpha 1 -> 0 = âge pour l'érosion du matériau
    Pr("Modules[16].ScaleDistribution", "(Values=((R=1,G=1,B=1,A=1),(R=1,G=1,B=1,A=0)),ValuesTimeRange=(X=0,Y=1),Mode=NonUniformCurve)")
    for i in (19, 21):
        Pr("Modules[%d].bModuleEnabled" % i, "False")
    Pr("Modules[20].bModuleEnabled", "True")
    Pr("Modules[20].ScaleDistribution", curve(cfg["courbe"]))
    # paramètre dynamique 0 = centre de la bouffée (User.CX/CY/CZ, posés par EONFx::Fumee) pour les normales de masse
    Pr("Modules[12].bModuleEnabled", "True")
    Pr("Modules[12].bParameter0Enabled", "True")
    for ch, u in (("X", "CX"), ("Y", "CY"), ("Z", "CZ")):
        B("Modules[12].Parameter0.%sChannelDistribution" % ch, u, "float")
    L.finish_system(s, e)
    ok(EAL.save_loaded_asset(s), NAME)


make("NS_EON_Fumee", dict(taille=(0.7, 1.0), rayon=12, origine=-20, vitesse=(20, 70), montee=15, turb=20, frein=3.0,
                          # gonfle vite, tient, puis rétrécit le premier (0 à 75 % de la vie) : le centre se vide
                          courbe=[0.3, 1.0, 1.25, 1.2, 0.6, 0.0, 0.0]))
make("NS_EON_FumeeLobes", dict(taille=(0.22, 0.5), rayon=30, origine=-25, vitesse=(60, 170), montee=22, turb=35, frein=3.2,
                               # naissent petits dans le noyau, gonflent en roulant vers l'extérieur, se défont à la fin
                               courbe=[0.25, 0.8, 1.05, 1.15, 1.1, 0.8, 0.0]))
open(os.path.join(unreal.Paths.project_saved_dir(), "fumee_build.txt"), "w", encoding="utf-8").write(chr(10).join(log))
