# EON — prototype de décor peint (Vincent, 23/11 : « un matériel pour les murs et les sols du HUB ; des coups de pinceau,
# une brush texture qui change la façon dont les ombres se diffusent ; 3 teintes en triplanar avec des trames de papier ou
# de diffusion aquarelle ; une variation ; ne détruis rien ; puis les ombres teintées, les arêtes, et le petit trait blanc
# de highlight sur les modèles du fond »). Référence : Orbitals (paint shader du décor).
# NE DÉTRUIT RIEN : tout est neuf, dans /Game/EON/Materials/Peint ; M_EON_BG, ses MI_BG_* et L_HUB ne sont pas modifiés.
#  - M_EON_DecorPeint (éclairé, Lumen) : mêmes entrées que M_EON_BG (BaseTex, Triplanar, TriSize, BC1, EmisAdd), puis
#    3 teintes (claire chaude / moyenne / sombre froide) réparties en triplanar par les taches de lavis + auréoles,
#    les coups de pinceau et le grain du papier (Pigment01, trame « mid » du shader Unity de Vincent).
#  - MI_Peint_<pièce>_A (gouache) et _B (aquarelle sur papier) : une par MI_BG_* (sauf Room_light), paramètres copiés.
#  - M_PP_EON_Decor (post-process, avant le trait) sur le décor seulement (jamais sur les celluloïds) :
#    lumière reçue = couleur de scène / couleur de base, ramenée à 3 valeurs ; les seuils sont déformés par les coups de
#    pinceau (texture accrochée au monde, en triplanar) ; ombre teintée froide ; arêtes rentrantes et bords d'objets en
#    liseré sombre ; arêtes saillantes tournées vers la lumière en liseré clair (le highlight blanc d'Orbitals).
#    MI_PP_EON_Decor_A / _B.
#   py -3 Saved/Scripts/eon_decor_textures.py      (textures, une fois)
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_decor_peint.py
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Materials/Peint"
GEN = "D:/_PROJETS/briscard/EON_UE/Saved/Generated/DecorPeint/"
# Textures faites à la main (Vincent, 23/11 : « qu'on puisse les mettre à jour et mettre des textures faites à la main ») :
# un PNG déposé ici sous le même nom PASSE AVANT la version générée ; T_EON_Papier.png remplace Pigment01.
#   T_EON_Pinceau.png   R = valeur de chaque coup (gris franc par coup : déforme les seuils d'ombre et éclaircit la teinte)
#                       G = poils / brosse sèche   B = couverture du coup (blanc dedans, noir hors des coups ; casse les liserés)
#   T_EON_Aquarelle.png R = taches de lavis (assombrit vers la teinte sombre)   G = auréoles au bord des taches   B = granulation
#   T_EON_Papier.png    grain du papier (niveaux de gris, moyenne quelconque : normalisée dans le matériau)
# Tout format carré raccordable (1024 à 4096). Relancer ce script réimporte ; dans l'éditeur, clic droit > Reimport suffit aussi.
MAIN = "D:/_PROJETS/briscard/EON_UE/Textures_Main/DecorPeint/"
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_PEINT " + str(m))


# ---------------- textures ----------------
tex = {}
import os
for nm in ("T_EON_Pinceau", "T_EON_Aquarelle", "T_EON_Papier"):
    src = MAIN + nm + ".png" if os.path.exists(MAIN + nm + ".png") else GEN + nm + ".png"
    if not os.path.exists(src):
        continue
    log("%s <- %s" % (nm, src))
    t = unreal.AssetImportTask()
    for k, v in (("filename", src), ("destination_path", DIR + "/Textures"), ("destination_name", nm),
                 ("replace_existing", True), ("automated", True), ("save", False)):
        t.set_editor_property(k, v)
    AT.import_asset_tasks([t])
    a = unreal.load_asset(DIR + "/Textures/" + nm)
    a.set_editor_property("srgb", nm == "T_EON_Papier")
    a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    EAL.save_loaded_asset(a)
    tex[nm] = a
tex["Papier"] = tex.get("T_EON_Papier") or unreal.load_asset("/Game/EON/Unity/import/textures/Shaders/Pigment01")
log("textures : %s" % {k: v.get_name() if v else None for k, v in tex.items()})


def new_material(name):
    p = DIR + "/" + name
    if EAL.does_asset_exist(p):
        m = unreal.load_asset(p); MEL.delete_all_material_expressions(m)
    else:
        m = AT.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())
    return m


def mk(m):
    def node(cls, x, y, **kw):
        n = MEL.create_material_expression(m, cls, x, y)
        for k, v in kw.items():
            n.set_editor_property(k, v)
        return n
    return node


def custom(m, node, code, desc, out_type, links, scal, vecs, x=-300, y=0):
    cu = node(unreal.MaterialExpressionCustom, x, y, code=code, output_type=out_type, description=desc)
    names = [n for n, _ in links] + [s[0] for s in scal] + [v[0] for v in vecs]
    ins = []
    for nm in names:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); ins.append(ci)
    cu.set_editor_property("inputs", ins)
    for nm, n in links:
        if not MEL.connect_material_expressions(n, "", cu, nm):
            log("ÉCHEC lien " + nm)
    for i, (nm, v) in enumerate(scal):
        MEL.connect_material_expressions(node(unreal.MaterialExpressionScalarParameter, x - 400, 600 + i * 60, parameter_name=nm, default_value=v), "", cu, nm)
    for i, (nm, v) in enumerate(vecs):
        MEL.connect_material_expressions(node(unreal.MaterialExpressionVectorParameter, x - 700, 600 + i * 120, parameter_name=nm,
                                              default_value=unreal.LinearColor(*v, 1.0)), "", cu, nm)
    return cu


TRI = r'''
float3 w = pow(abs(VN), 4.0); w /= dot(w, float3(1, 1, 1));
#define TRI(T, s) (Texture2DSample(T, T##Sampler, WP.yz / (s)) * w.x + Texture2DSample(T, T##Sampler, WP.xz / (s)) * w.y + Texture2DSample(T, T##Sampler, WP.xy / (s)) * w.z)
#define TRIL(T, s, l) (Texture2DSampleLevel(T, T##Sampler, WP.yz / (s), l) * w.x + Texture2DSampleLevel(T, T##Sampler, WP.xz / (s), l) * w.y + Texture2DSampleLevel(T, T##Sampler, WP.xy / (s), l) * w.z)
#define LUM(c) dot(c, float3(0.299, 0.587, 0.114))
'''

# ---------------- matériau de surface ----------------
m = new_material("M_EON_DecorPeint")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
for _u in ("used_with_instanced_static_meshes", "used_with_skeletal_mesh"):
    m.set_editor_property(_u, True)
node = mk(m)
bt = node(unreal.MaterialExpressionTextureObjectParameter, -2200, -300, parameter_name="BaseTex",
          texture=unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
uvs = node(unreal.MaterialExpressionTextureSample, -1900, -450)
MEL.connect_material_expressions(bt, "", uvs, "Tex")
tri = node(unreal.MaterialExpressionMaterialFunctionCall, -1900, -200)
tri.set_editor_property("material_function", unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture"))
MEL.connect_material_expressions(bt, "", tri, "TextureObject")
MEL.connect_material_expressions(node(unreal.MaterialExpressionScalarParameter, -2200, -100, parameter_name="TriSize", default_value=303.0), "", tri, "TextureSize")
sw = node(unreal.MaterialExpressionStaticSwitchParameter, -1600, -300, parameter_name="Triplanar", default_value=False)
MEL.connect_material_expressions(tri, "XYZ Texture", sw, "True")
MEL.connect_material_expressions(uvs, "RGB", sw, "False")
links = [("BT", sw), ("WP", node(unreal.MaterialExpressionWorldPosition, -1600, -100)),
         ("VN", node(unreal.MaterialExpressionVertexNormalWS, -1600, 0))]
for nm in ("Pinceau", "Aquarelle", "Papier"):
    links.append((nm, node(unreal.MaterialExpressionTextureObjectParameter, -1600, 100 + len(links) * 120, parameter_name=nm,
                           texture=tex["T_EON_" + nm] if nm != "Papier" else tex["Papier"])))
SURF = TRI + r'''
float keep = 0.0;
float3 base = BT * BC1.rgb;
float4 pin = TRI(Pinceau, PinceauTaille);
float4 aq = TRI(Aquarelle, AquaTaille);
float pap = LUM(TRI(Papier, PapierTaille).rgb) / max(LUM(TRIL(Papier, PapierTaille, 10).rgb), 1e-3);
float3 tClair = base * ClairMul * TeinteClaire.rgb;
float3 tSombre = base * SombreMul * TeinteSombre.rgb;
// 3 teintes : le lavis (taches + auréoles au bord + granulation) assombrit, les coups de pinceau éclaircissent.
float sombre = saturate(aq.r * AquaForce + aq.g * AureoleForce + (aq.b - 0.5) * GranForce);
// coup de pinceau signé : valeur claire -> teinte claire, valeur sombre -> vers la teinte sombre (plus doux)
float coup = clamp((pin.r - 0.5) * 2.0 * PinceauForce * pin.b, -1.0, 1.0);
float3 c = lerp(base, tSombre, sombre);
c = coup > 0.0 ? lerp(c, tClair, coup) : lerp(c, tSombre, -coup * 0.6);
c *= lerp(1.0, pap, PapierForce);
return max(c, 0.0);
'''
SURF_SCAL = [("PinceauTaille", 220.0), ("AquaTaille", 450.0), ("PapierTaille", 120.0), ("ClairMul", 1.12), ("SombreMul", 0.85),
             ("AquaForce", 0.1), ("AureoleForce", 0.12), ("GranForce", 0.05), ("PinceauForce", 0.4), ("PapierForce", 0.08)]   # 23/11 : « beaucoup plus subtil »
SURF_VEC = [("BC1", (1, 1, 1)), ("TeinteClaire", (1.06, 1.0, 0.88)), ("TeinteSombre", (0.82, 0.95, 1.18))]
cs = custom(m, node, SURF, "Décor peint EON : 3 teintes", unreal.CustomMaterialOutputType.CMOT_FLOAT3, links, SURF_SCAL, SURF_VEC)
P = unreal.MaterialProperty
MEL.connect_material_property(cs, "", P.MP_BASE_COLOR)
MEL.connect_material_property(node(unreal.MaterialExpressionScalarParameter, -300, 300, parameter_name="Roughness", default_value=0.85), "", P.MP_ROUGHNESS)
MEL.connect_material_property(node(unreal.MaterialExpressionConstant, -300, 400, r=0.0), "", P.MP_METALLIC)
MEL.connect_material_property(node(unreal.MaterialExpressionVectorParameter, -300, 500, parameter_name="EmisAdd",
                                   default_value=unreal.LinearColor(0, 0, 0, 0)), "", P.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
log("M_EON_DecorPeint prêt")

# ---------------- post-process du décor ----------------
g = new_material("M_PP_EON_Decor")
g.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
g.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)
g.set_editor_property("blendable_priority", -10)   # avant M_PP_EON_Trait (priorité 0)
node = mk(g)
st = []
for i, idx in enumerate((unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, unreal.SceneTextureId.PPI_WORLD_NORMAL,
                         unreal.SceneTextureId.PPI_CUSTOM_STENCIL, unreal.SceneTextureId.PPI_SCENE_DEPTH,
                         unreal.SceneTextureId.PPI_BASE_COLOR, unreal.SceneTextureId.PPI_CUSTOM_DEPTH)):
    st.append(("ST%d" % i, node(unreal.MaterialExpressionSceneTexture, -1300, -300 + i * 150, scene_texture_id=idx)))
links = st + [("WP", node(unreal.MaterialExpressionWorldPosition, -1300, 700)),
              ("Pinceau", node(unreal.MaterialExpressionTextureObjectParameter, -1300, 850, parameter_name="Pinceau", texture=tex["T_EON_Pinceau"])),
              ("EA", node(unreal.MaterialExpressionEyeAdaptation, -1300, 1000))]
PP = r'''
float2 uv = GetViewportUV(Parameters);
float keep = (ST0.r + ST1.r + ST2.r + ST3.r + ST4.r + ST5.r) * 0.0;
float2 px = View.ViewSizeAndInvSize.zw;
#define LK(u, id) SceneTextureLookup(ClampSceneTextureUV(ViewportUVToSceneTextureUV(u, id), id), id, false)
#define CEL(u) (LK(u, 25).r > 0.5 && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0)
float3 sc = LK(uv, 14).rgb;
float d0 = LK(uv, 1).r;
// Étalonnage séparé (Vincent, 23/11, étude Orbitals) : celluloïds plus clairs que le décor, décor moins saturé et plus sombre.
// Neutre par défaut (CelExpo 0, CelSat 1, FondExpo 0, FondSat 1, FondFroid 0).
if (d0 > 500000.0) return sc + keep;                 // ciel : intact
// Clamp du bloom (Vincent, 23/11) : aucune couleur au-dessus de BloomMax × le blanc d'affichage (1 / exposition), teinte
// gardée -> le bloom, qui part de cette image, ne peut pas exploser sur les émissions très fortes. 0 = sans clamp.
#define CLAMPB(x) ((x) * (BloomMax > 0.0 ? min(1.0, BloomMax / max(EA, 1e-6) / max(max(max((x).r, (x).g), (x).b), 1e-6)) : 1.0))
if (CEL(uv)) { float3 cc = sc * exp2(CelExpo); cc = lerp(dot(cc, float3(0.299, 0.587, 0.114)).xxx, cc, CelSat); return CLAMPB(cc) + keep; }
float3 bc = LK(uv, 5).rgb;
float3 VN = normalize(LK(uv, 8).rgb);
''' + TRI + r'''
float4 br = TRI(Pinceau, BrushSize);
float lb = LUM(bc);
// Lumière reçue (en diaphs) = scène / couleur de base ; ramenée à 3 valeurs, seuils déformés par le coup de pinceau.
float lv = log2(max(LUM(sc) / max(lb, 0.02) * Expo, 1e-5));
float bo = (br.r - 0.5) * 2.0 * BrushAmp;
float k1 = smoothstep(Seuil1 + bo - Soft, Seuil1 + bo + Soft, lv);
float k2 = smoothstep(Seuil2 + bo - Soft, Seuil2 + bo + Soft, lv);
float lq = lerp(lerp(OmbreStops, MoyenStops, k1), ClairStops, k2) + (br.r - 0.5) * 2.0 * BrushVal * br.b;   // le coup module aussi la lumière dans l'aplat
float g = (lb > 0.025 ? 1.0 : 0.0) * (1.0 - smoothstep(ClairStops + 1.5, ClairStops + 3.0, lv));   // néons épargnés
float3 c = sc * exp2((lerp(lv, lq, Aplat * g)) - lv);
float3 tO = TeinteOmbre.rgb / max(LUM(TeinteOmbre.rgb), 1e-3);
float om = saturate((1.0 - k1) + (1.0 - k2) * MoyenTeinte);
c = lerp(c, c * tO, om * OmbreForce * g);
if (DebugView > 0.5 && DebugView < 1.5) return float3(1.0 - k1, k1 * (1.0 - k2), k2) * 0.6 + keep;
if (DebugView > 1.5 && DebugView < 2.5) { float h = saturate((floor(lv) + 6.0) / 9.0) * 0.8; return saturate(abs(frac(h + float3(0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0) * 0.6 + keep; }
// Arêtes : rentrantes / bords d'objets -> liseré sombre ; saillantes tournées vers la lumière -> liseré clair.
float P00 = View.ViewToClip[0][0], P11 = View.ViewToClip[1][1];
#define VPOS(u, d) float3(((u).x * 2.0 - 1.0) * (d) / P00, (1.0 - (u).y * 2.0) * (d) / P11, (d))
float3 V0 = VPOS(uv, d0);
float3 Nv = mul(VN, (float3x3)View.TranslatedWorldToView);
float3 Hd = normalize(LiserDir.rgb);
float h0 = dot(VN, Hd);
float rH = 99.0, rA = 99.0;
float maxR = max(LiserLarg, AreteLarg) + 1.0;
[unroll] for (int k = 0; k < 8; k++) {
    float2 dir = float2(cos(k * 0.785398), sin(k * 0.785398));
    [loop] for (float r = 1.0; r <= maxR; r += 1.0) {
        float2 q = uv + dir * px * r;
        if (CEL(q)) break;
        float dq = LK(q, 1).r;
        if (dq > d0 * (1.0 + DepthTol) + 2.0) { rA = min(rA, r - 0.5); break; }   // bord d'objet : côté proche
        if (dq < d0 * (1.0 - DepthTol) - 2.0) break;
        float3 Nq = normalize(LK(q, 8).rgb);
        if (dot(VN, Nq) < CreaseCos) {
            float3 Vq = VPOS(q, dq);
            // saillante : le voisin passe sous le plan tangent (seuil bien plus fin qu'un pixel) ; seule la face tournée vers
            // la lumière reçoit le liseré clair, l'autre rien ; rentrante -> liseré sombre.
            if (dot(Nv, Vq - V0) < -0.00001 * d0) { if (h0 >= dot(Nq, Hd)) rH = min(rH, r - 0.5); }
            else rA = min(rA, r - 0.5);
            break;
        }
    }
}
float brk = TRI(Pinceau, BrushSize * 0.37).b;
float aH = saturate(LiserLarg + 0.5 - rH) * smoothstep(LiserCasse - 0.15, LiserCasse + 0.15, brk);
float aA = saturate(AreteLarg + 0.5 - rA) * AreteForce;
if (DebugView > 2.5) return float3(aA, aH, 0) + keep;
c = lerp(c, c * AreteValeur * tO, aA);
// lisere clair en ADDITIF (Vincent, 23/11 : additif 10 % ; « encore beaucoup trop visible » en blanc d'affichage) :
// + LiserAdd de la valeur locale, + LiserBlanc du blanc d'affichage (1 / exposition), teinte LiserCouleur.
c += LiserCouleur.rgb * (dot(c, float3(0.2126, 0.7152, 0.0722)) * LiserAdd + LiserBlanc / max(EA, 1e-6)) * aH;
float3 froid = float3(0.85, 0.95, 1.25) / LUM(float3(0.85, 0.95, 1.25));
c = lerp(LUM(c).xxx, c, FondSat) * exp2(FondExpo) * lerp(1.0.xxx, froid, FondFroid * g);
return CLAMPB(c) + keep;
'''
PP_SCAL = [("Expo", 11.0), ("BrushSize", 180.0), ("BrushAmp", 0.9), ("Seuil1", -2.2), ("Seuil2", -0.8), ("Soft", 0.08),
           ("OmbreStops", -2.8), ("MoyenStops", -1.4), ("ClairStops", -0.2), ("Aplat", 0.75), ("BrushVal", 0.12), ("OmbreForce", 0.6), ("MoyenTeinte", 0.25),
           ("CreaseCos", 0.8), ("DepthTol", 0.02), ("AreteLarg", 1.2), ("AreteForce", 0.55), ("AreteValeur", 0.45),
           ("LiserLarg", 2.5), ("LiserAdd", 0.5), ("LiserBlanc", 0.0),
           ("CelExpo", 0.0), ("CelSat", 1.0), ("FondExpo", 0.0), ("FondSat", 1.0), ("FondFroid", 0.0), ("BloomMax", 8.0), ("LiserCasse", 0.5), ("DebugView", 0.0)]
PP_VEC = [("TeinteOmbre", (0.55, 0.75, 1.0)), ("LiserCouleur", (1.0, 0.95, 0.85)), ("LiserDir", (0.3, 0.2, 1.0))]
cu = custom(g, node, PP, "Décor peint EON : valeurs, ombre teintée, arêtes, liserés", unreal.CustomMaterialOutputType.CMOT_FLOAT3, links, PP_SCAL, PP_VEC)
MEL.connect_material_property(cu, "", P.MP_EMISSIVE_COLOR)
MEL.recompile_material(g)
EAL.save_loaded_asset(g)
log("M_PP_EON_Decor prêt")

# ---------------- variantes ----------------
VAR = {
    "A": dict(surf=dict(), pp=dict()),   # gouache : défauts ci-dessus
    "B": dict(surf=dict(ClairMul=1.08, SombreMul=0.88, AquaForce=0.15, AureoleForce=0.1, GranForce=0.1, PinceauForce=0.15, PapierForce=0.15,
                        TeinteSombre=(0.85, 0.85, 1.15)),
              pp=dict(Aplat=0.5, Soft=0.25, BrushAmp=0.6, OmbreForce=0.5, TeinteOmbre=(0.75, 0.65, 1.0), AreteForce=0.45, BrushVal=0.08,
                      # P5 de l'étude Orbitals (Vincent, 23/11) : personnages plus clairs, décor plus sombre, moins saturé, bleuté
                      CelExpo=2.0, CelSat=0.9, FondExpo=-0.3, FondSat=0.6, FondFroid=0.7)),
}


def setp(mi, k, v):
    if isinstance(v, tuple):
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(*v, 1.0))
    else:
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)


def make_mi(name, parent):
    p = DIR + "/" + name
    if EAL.does_asset_exist(p):
        mi = unreal.load_asset(p); MEL.clear_all_material_instance_parameters(mi)
    else:
        mi = AT.create_asset(name, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    return mi


reg = unreal.AssetRegistryHelpers.get_asset_registry()
bgs = [ad.get_asset() for ad in reg.get_assets_by_path("/Game/EON/Materials/BG", recursive=False)]
bgs = [b for b in bgs if isinstance(b, unreal.MaterialInstanceConstant) and b.get_name() != "MI_BG_Room_light"]
for v, prm in VAR.items():
    for b in bgs:
        mi = make_mi(b.get_name().replace("MI_BG_", "MI_Peint_") + "_" + v, m)
        t = MEL.get_material_instance_texture_parameter_value(b, "BaseTex")
        if t:
            MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", t)
        MEL.set_material_instance_vector_parameter_value(mi, "BC1", MEL.get_material_instance_vector_parameter_value(b, "BC1"))
        MEL.set_material_instance_vector_parameter_value(mi, "EmisAdd", MEL.get_material_instance_vector_parameter_value(b, "EmisAdd"))
        if MEL.get_material_instance_static_switch_parameter_value(b, "Triplanar"):
            MEL.set_material_instance_static_switch_parameter_value(mi, "Triplanar", True)
            MEL.set_material_instance_scalar_parameter_value(mi, "TriSize", MEL.get_material_instance_scalar_parameter_value(b, "TriSize"))
        for k, val in prm["surf"].items():
            setp(mi, k, val)
        MEL.update_material_instance(mi)
        EAL.save_loaded_asset(mi)
    mp = make_mi("MI_PP_EON_Decor_" + v, g)
    for k, val in prm["pp"].items():
        setp(mp, k, val)
    MEL.update_material_instance(mp)
    EAL.save_loaded_asset(mp)
    log("variante %s : %d MI_Peint + MI_PP_EON_Decor_%s" % (v, len(bgs), v))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_decor_peint.txt", "w", encoding="utf-8").write("\n".join(LOG))
unreal.log("EON_PEINT fini")
