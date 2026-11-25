# EON — boule de feu des explosions (Vincent, 24/11 : « reprendre le principe de fumée déjà travaillé »).
# Même principe que eon_fumee_build.py : amas de sphères Niagara (noyau + lobes soudés), bulles en world space fixe,
# normales de masse, érosion à bords nets, 12 i/s et trait extérieur (C++) ; mais non éclairé : cœur blanc aveuglant,
# bord à la couleur de l'explosion (couleur de la particule) qui envahit l'amas. La fumée toon sombre prend le relais.
# Lancer : UnrealEditor-Cmd EON.uproject -run=pythonscript -script=<ce fichier>
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
WOR = r"""
// Worley 3D (cellules de taille 1) : distance au germe le plus proche et position de ce germe
float3 q = P * Freq; float3 ic = floor(q); float best = 9.0; float3 bp = q;
[unroll] for (int x = -1; x <= 1; x++) [unroll] for (int y = -1; y <= 1; y++) [unroll] for (int z = -1; z <= 1; z++) {
  float3 c = ic + float3(x, y, z);
  float3 h = frac(sin(float3(dot(c, float3(127.1, 311.7, 74.7)), dot(c, float3(269.5, 183.3, 246.1)), dot(c, float3(113.5, 271.9, 124.6)))) * 43758.5453);
  float3 fp = c + 0.15 + h * 0.7; float dd = length(q - fp);
  if (dd < best) { best = dd; bp = fp; }
}
// bulle : demi-sphère de rayon Rayon autour du germe
float bump = sqrt(saturate(1.0 - (best / Rayon) * (best / Rayon)));
"""
# Couleur : âge = 1 - alpha de la particule. Fresnel sur la normale de la MASSE (centre = paramètre dynamique 0, comme
# la fumée) : le bord coloré entoure l'amas entier, pas chaque boule ; il envahit le blanc en vieillissant.
COUL = r"""
float a = saturate(Age);
float3 dc = (P - Centre) * float3(1, 1, 1.6);
float3 n0 = normalize(N);
float3 n = normalize(lerp(n0, length(dc) > 1 ? normalize(dc) : n0, Masse));
float f = 1.0 - saturate(dot(n, normalize(V)));
float s = lerp(Seuil, -0.2, saturate(a / Envahit));
float bord = step(s, f + (Bruit - 0.5) * 0.35);
return lerp(Coeur * LumCoeur, Bord * LumBord, bord);
"""
EROS = r"""
float a = saturate(Age);
float k = saturate((a - Debut) / (1.0 - Debut)) * 1.1 - 0.05;
return Bruit - k;
"""
BULLE_WPO = WOR + r"""
return normalize(N) * bump * Amp;
"""

P = DOS + "/M_EON_FeuToon"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
m = AT.create_asset("M_EON_FeuToon", DOS, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
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
    ok(r, "lien %s -> %s.%s" % (a.get_class().get_name(), b.get_class().get_name(), pin))
    return r


pc = E(unreal.MaterialExpressionParticleColor, -1600, -300)
age = E(unreal.MaterialExpressionOneMinus, -1400, -200)
link(pc, age, "", "A")
# bruit sur la position locale de la sphère + décalage par particule (comme la fumée)
rnd = E(unreal.MaterialExpressionParticleRandom, -1600, 550)
wp = E(unreal.MaterialExpressionWorldPosition, -1800, 200)
pp = E(unreal.MaterialExpressionParticlePositionWS, -1800, 300)
loc = E(unreal.MaterialExpressionSubtract, -1600, 250)
link(wp, loc, "A"); link(pp, loc, "B")
sc = E(unreal.MaterialExpressionMultiply, -1450, 250)
link(loc, sc, "A"); link(scal("BruitEchelle", 0.035, -1600, 330), sc, "B")
rv = E(unreal.MaterialExpressionMultiply, -1450, 550)
link(rnd, rv, "A"); rv.set_editor_property("const_b", 37.0)
ad = E(unreal.MaterialExpressionAdd, -1300, 300)
link(sc, ad, "A"); link(rv, ad, "B")
nz = custom(BRUIT, ["P"], -1150, 300, desc="EON_FeuBruit")
link(ad, nz, "P")

wpa = E(unreal.MaterialExpressionWorldPosition, -1400, 700)
try:
    wpa.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
except Exception as ex:
    log.append("ERR  position sans offset : %s" % ex)
dyn = E(unreal.MaterialExpressionDynamicParameter, -1400, 950)
col = custom(COUL, ["N", "V", "P", "Centre", "Masse", "Age", "Bruit", "Seuil", "Envahit", "Coeur", "LumCoeur", "Bord", "LumBord"], -700, -150,
             unreal.CustomMaterialOutputType.CMOT_FLOAT3, "EON_FeuCouleur")
link(E(unreal.MaterialExpressionVertexNormalWS, -1000, -400), col, "N")
link(E(unreal.MaterialExpressionCameraVectorWS, -1000, -350), col, "V")
link(wpa, col, "P")
link(dyn, col, "Centre", "RGB")
link(scal("Masse", 0.75, -1000, -300), col, "Masse")
link(age, col, "Age"); link(nz, col, "Bruit")
link(scal("BordSeuil", 0.3, -1000, -250), col, "Seuil")
link(scal("BordEnvahit", 0.45, -1000, -200), col, "Envahit")
coeur = E(unreal.MaterialExpressionVectorParameter, -1000, -150)
coeur.set_editor_property("parameter_name", "Coeur"); coeur.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
link(coeur, col, "Coeur", "RGB")
link(scal("LumCoeur", 1.6, -1000, -100), col, "LumCoeur")
link(pc, col, "Bord", "RGB")
link(scal("LumBord", 1.0, -1000, 0), col, "LumBord")
eai = E(unreal.MaterialExpressionEyeAdaptationInverse, -450, 50)
em = E(unreal.MaterialExpressionMultiply, -300, -50)
link(col, em, "A"); link(eai, em, "B")
ok(MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "feu : émission")

ce = custom(EROS, ["Bruit", "Age", "Debut"], -700, 350, desc="EON_FeuErosion")
link(nz, ce, "Bruit"); link(age, ce, "Age"); link(scal("ErosionDebut", 0.5, -1000, 400), ce, "Debut")
ok(MEL.connect_material_property(ce, "", unreal.MaterialProperty.MP_OPACITY_MASK), "feu : masque")

wpo = custom(BULLE_WPO, ["N", "P", "Freq", "Rayon", "Amp"], -700, 650, unreal.CustomMaterialOutputType.CMOT_FLOAT3, "EON_FeuBulles")
link(E(unreal.MaterialExpressionVertexNormalWS, -1000, 600), wpo, "N"); link(wpa, wpo, "P")
link(scal("BulleFreq", 0.045, -1000, 700), wpo, "Freq"); link(scal("BulleRayon", 0.85, -1000, 760), wpo, "Rayon")
link(scal("BulleAmp", 9.0, -1000, 820), wpo, "Amp")
ok(MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET), "feu : bulles")
MEL.recompile_material(m)
ok(EAL.save_loaded_asset(m), "M_EON_FeuToon")

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


def make(NAME, cfg, m):
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



# Feu : même structure que la fumée (noyau + lobes soudés), mais plus vif : gonfle en 2 images, pousse plus loin, vit peu.
make("NS_EON_Feu", dict(taille=(0.75, 1.05), rayon=14, origine=-20, vitesse=(30, 90), montee=6, turb=15, frein=4.0,
                        courbe=[0.55, 1.15, 1.25, 1.25, 1.15, 0.9, 0.5]), m)
make("NS_EON_FeuLobes", dict(taille=(0.25, 0.55), rayon=30, origine=-25, vitesse=(90, 230), montee=8, turb=25, frein=4.5,
                             courbe=[0.45, 1.0, 1.15, 1.15, 1.1, 0.9, 0.5]), m)
# Fumée d'explosion (Vincent, 24/11 : « le nuage est censé s'excentrer et se dissiper légèrement vers le haut », 2 fois plus
# de sphères) : même matériau toon que la poussière (M_EON_FumeeToon), mais les sphères partent du centre vers l'extérieur
# (origine au niveau du centre, à peine dessous), freinées pour s'ouvrir puis ralentir, peu de turbulence (plus de
# tremblement), dérive lente vers le haut ; elles grossissent jusqu'à la fin (l'érosion les défait).
# Instance du nuage d'explosion (Vincent, 24/11 : « les sphères restent trop sphériques ; augmenter le displacement et son
# scale ; rotations assez lentes des sphères sur elles-mêmes ») : grosses bosses ±30 cm (~80 cm) + bulles distinctes (~33 cm, 14 cm), et le
# champ de bulles tourne avec chaque sphère (BulleRotation, rad sur la vie). Lancer eon_fumee_build.py AVANT ce script.
MIP = DOS + "/MI_EON_FumeeExplo"
if EAL.does_asset_exist(MIP):
    EAL.delete_asset(MIP)
MF = AT.create_asset("MI_EON_FumeeExplo", DOS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
MEL.set_material_instance_parent(MF, unreal.load_asset("/Game/EON/VFX/M_EON_FumeeToon"))
for nom, v in (("BulleFreq", 0.03), ("BulleAmp", 14.0), ("BulleRayon", 0.6), ("BosseFreq", 0.012), ("BosseAmp", 30.0), ("BulleRotation", 2.2),
                ("FonduDebut", 0.15), ("FonduMax", 0.85)):   # version validée (Vincent, 25/11 : « la vidéo qui marchait correctement »)
    MEL.set_material_instance_scalar_parameter_value(MF, nom, v)      # renvoie False même quand ça marche : on relit
    ok(abs(MEL.get_material_instance_scalar_parameter_value(MF, nom) - v) < 1e-4, "MI_EON_FumeeExplo %s = %s" % (nom, v))
ok(EAL.save_loaded_asset(MF), "MI_EON_FumeeExplo")
make("NS_EON_FumeeExplo", dict(taille=(0.7, 1.0), rayon=16, origine=-8, vitesse=(50, 110), montee=22, turb=4, frein=1.8,
                               courbe=[0.45, 0.9, 1.1, 1.2, 1.25, 1.25, 1.2]), MF)
make("NS_EON_FumeeExploLobes", dict(taille=(0.25, 0.55), rayon=32, origine=-8, vitesse=(130, 280), montee=26, turb=6, frein=2.0,
                                    courbe=[0.35, 0.8, 1.0, 1.1, 1.15, 1.15, 1.1]), MF)
open(os.path.join(unreal.Paths.project_saved_dir(), "feu_build.txt"), "w", encoding="utf-8").write(chr(10).join(log))
