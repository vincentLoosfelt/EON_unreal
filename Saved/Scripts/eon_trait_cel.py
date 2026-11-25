# EON — trait des celluloïds (Vincent, 23/11 ; référence Orbitals, blog Unreal du 03/09/2026) : line art propre,
# anticrénelé, adaptatif, brun foncé, sur les seuls éléments animés (CustomStencil = 1, posé par UEONStatics::MarkCel).
# Post-process avant le DOF : les effets translucides « après DOF » restent par-dessus ; TSR lisse ensuite.
# v2 (banc rapproché) : distance au bord le plus proche, cherchée sur 12 directions par pas de 0,5 px ->
#   alpha = saturate(W + 0,5 - d) : épaisseur exacte en px et bord de trait anticrénelé sur 1 px.
#   Bords cherchés : silhouette (fond ou nettement plus loin -> épaisseur SilWidth) ; intérieurs = « zones de
#   surface » d'Orbitals, ici plis (normales du GBuffer) ou changement de TEINTE de la couleur de base -> InnerWidth ; v6d (Vincent) :
#   jamais de trait sur un changement de ton (ombres peintes, frontière ombre/lumière).
#   Épaisseur modulée par une pression de main ancrée au monde (Jitter) ;
#   cassures de trait par bruit (Breaks). Couleur : encre multipliée, voir v6.
#   v3 (23/11, capture d'Orbitals) : Soft = largeur du bord du trait en px (1 net, 2-3 flou/baveux).
#   v4 (Vincent) : jamais de trait sur le décor placé devant un personnage (test de visibilité scène/custom depth).
#   v5 (Vincent) : ombre décalée du cel sur le décor, toujours en bas à droite à l'écran (ShadowStrength, ShadowOffX/Y px,
#   ShadowSoft).
#   v6 (Vincent, 23/11) : encre brun très foncé, valeur ≈ 10 % (pas du noir) : un trait seul est posé à la valeur
#   absolue InkValue (teinte InkHue), quel que soit l'éclairage du dessous ; puis les couches se CUMULENT en
#   multiplication (CumulMul par couche) : silhouette + intérieur, zones denses de traits (part des 12 directions qui
#   trouvent un bord, Dense) -> presque noir. L'ancien trait « couleur d'ombre » (LineColor, ColorMix, ShadowMul) est supprimé.
#   v6c (Vincent, 23/11) : « le line art est uniquement là pour détourer les formes » : AUCUNE modulation par l'ombre
#   (épaississement / amincissement / encre cumulée dans l'ombre supprimés, avec la mesure de lumière reçue).
#   v7 (Vincent, 24/11) : silhouette = 2 × intérieur (InnerRatio 0,5 ; InnerWidth supprimé) ; épaisseur relative au zoom de la
#   caméra (Zoom = paramètre TraitZoom de MPC_EON_Rendu, écrit par AEONPlayerCameraManager) et bornée : la silhouette reste
#   entre SilMin et SilMax px, l'intérieur suit à la moitié.
#   DebugView 1 = couleur de base, 2 = masque du trait.
#   Les 3 tons ne sont PAS ici : Substrate Toon BSDF des celluloïds, rampe diffuse du Toon Profile TP_EON_Anime (Vincent, 23/11).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_trait_cel.py
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Materials"
path = DIR + "/M_PP_EON_Trait"
if EAL.does_asset_exist(path):
    g = unreal.load_asset(path); MEL.delete_all_material_expressions(g)
else:
    g = AT.create_asset("M_PP_EON_Trait", DIR, unreal.Material, unreal.MaterialFactoryNew())
g.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
g.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)


def node(cls, x, y, **kw):
    n = MEL.create_material_expression(g, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n


st = {}
for i, (nm, idx) in enumerate((("PPI0", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0), ("WN", unreal.SceneTextureId.PPI_WORLD_NORMAL),
                               ("CS", unreal.SceneTextureId.PPI_CUSTOM_STENCIL), ("SD", unreal.SceneTextureId.PPI_SCENE_DEPTH),
                               ("BC", unreal.SceneTextureId.PPI_BASE_COLOR), ("CD", unreal.SceneTextureId.PPI_CUSTOM_DEPTH))):
    st[nm] = node(unreal.MaterialExpressionSceneTexture, -900, -300 + i * 150, scene_texture_id=idx)
wp = node(unreal.MaterialExpressionWorldPosition, -900, 500)
tn = node(unreal.MaterialExpressionTextureObjectParameter, -900, 650, parameter_name="NoiseTex",
          texture=unreal.load_asset("/Game/EON/Textures/PostProcess/T_EON_WetNormal"))
MPC_PATH = DIR + "/MPC_EON_Rendu"
if EAL.does_asset_exist(MPC_PATH):
    mpc = unreal.load_asset(MPC_PATH)
else:
    mpc = AT.create_asset("MPC_EON_Rendu", DIR, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
sp = [x for x in mpc.get_editor_property("scalar_parameters") if str(x.get_editor_property("parameter_name")) != "TraitZoom"]
tz = unreal.CollectionScalarParameter(); tz.set_editor_property("parameter_name", "TraitZoom"); tz.set_editor_property("default_value", 1.0)
mpc.set_editor_property("scalar_parameters", sp + [tz])
EAL.save_loaded_asset(mpc, False)
zm = node(unreal.MaterialExpressionCollectionParameter, -900, 950, collection=mpc, parameter_name="TraitZoom")
ih = node(unreal.MaterialExpressionVectorParameter, -900, 800, parameter_name="InkHue", default_value=unreal.LinearColor(1.0, 0.62, 0.40, 1))

CODE = r'''
float2 uv = GetViewportUV(Parameters);
float keep = (ST0.r + ST1.r + ST2.r + ST3.r + ST4.r + ST5.r) * 0.0;
float2 px = View.ViewSizeAndInvSize.zw;
#define LK(u, id) SceneTextureLookup(ClampSceneTextureUV(ViewportUVToSceneTextureUV(u, id), id), id, false)
// v7 : épaisseur de silhouette relative au zoom, bornée ; l'intérieur en est la moitié (InnerRatio).
float SW = clamp(SilWidth * Zoom, SilMin, SilMax);
float3 under = LK(uv, 14).rgb;
if (DebugView > 0.5 && DebugView < 1.5) return LK(uv, 5).rgb + keep;
// Celluloïd VISIBLE seulement : le stencil s'écrit aussi derrière le décor ; on exige profondeur de scène ≈ profondeur
// custom (sinon un élément de décor est devant : ni trait ni traitement cel sur lui).
// Stencil 1 seulement : le stencil 2 (projectiles, Vincent 24/11) reçoit l'étalonnage cel du décor mais pas de trait.
#define VIS(u) ((abs(LK(u, 25).r - 1.0) < 0.5 || abs(LK(u, 25).r - 3.0) < 0.5) && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0 && (abs(LK(u, 25).r - 3.0) > 0.5 || (LK(u, 1).r < LK(u, 13).r * 1.0002 + 0.3 && LK(u, 1).r > LK(u, 13).r * 0.9998 - 0.3)))
// Fumée toon (stencil 3) : ses lobes forment une seule masse, détourée seulement à l'extérieur.
#define FUM(u) (abs(LK(u, 25).r - 3.0) < 0.5)
if (!VIS(uv)) {
    // Ombre du cel sur le décor (prise de vue des années 90 : le cel posé sur la peinture, son épaisseur et
    // l'éclairage de face portent une légère ombre). Toujours décalée en bas à droite À L'ÉCRAN, quelle que soit
    // l'orientation : on cherche le cel en haut à gauche. Seulement sur ce qui est derrière lui (jamais sur un décor
    // placé devant le personnage). Bord adouci par 9 lectures.
    float sh = 0;
    if (ShadowStrength > 0.001) {
        float dHere = LK(uv, 1).r;
        float2 off = float2(ShadowOffX, ShadowOffY) * px;
        [unroll] for (int j = 0; j < 9; j++) {
            float2 q = uv - off + float2(j % 3 - 1, j / 3 - 1) * px * ShadowSoft;
            sh += (VIS(q) && LK(q, 1).r < dHere) ? 1.0 : 0.0;
        }
        sh /= 9.0;
    }
    // v6e (Vincent : doigts fins noyés d'encre) : trait de silhouette CENTRÉ sur le bord ; sa moitié extérieure est posée
    // ici, sur le décor qui touche le cel (même presque à sa profondeur : sol sous un pied), jamais sur un décor devant.
    float dHO = LK(uv, 1).r;
    float2 nuvO = WP.xy * 0.01 * HandScale;
    float wO = max(SW * (1.0 + Jitter * (Texture2DSample(NoiseTex, NoiseTexSampler, nuvO).r * 2.0 - 1.0)), 0.3) * 0.5;
    float dO = 99;
    [unroll] for (int k = 0; k < 12; k++) {
        float2 dir = float2(cos(k * 0.5235988), sin(k * 0.5235988));
        [loop] for (float r = 0.5; r <= wO + Soft + 0.5; r += 0.5) {
            float2 q = uv + dir * px * r;
            if (VIS(q) && LK(q, 1).r < dHO * (1.0 + OutTol)) { dO = min(dO, r - 0.25); break; }
        }
    }
    float brO = Texture2DSample(NoiseTex, NoiseTexSampler, nuvO * 3.7 + 0.31).g;
    float aO = saturate((wO + 0.5 * Soft - dO) / Soft) * saturate(1.0 - Breaks * saturate((brO - 0.55) * 6.0)) * Opacity;
    if (DebugView > 1.5) return float3(aO, 0, sh) + keep;
    float3 dec = under * (1.0 - ShadowStrength * sh);
    float3 inkO = InkValue * InkHue.rgb / max(dot(InkHue.rgb, float3(0.299, 0.587, 0.114)), 1e-4);
    return lerp(dec, min(dec, inkO), aO) + keep;
}   // décor : intact (hors ombre portée et moitié extérieure du trait)
// Épaisseur : seulement la pression de main (le trait détoure les formes, il ne suit pas l'ombre).
float2 nuv = WP.xy * 0.01 * HandScale;
float hand = Texture2DSample(NoiseTex, NoiseTexSampler, nuv).r;
float wMul = 1.0 + Jitter * (hand * 2.0 - 1.0);
float Ws = max(SW * wMul, 0.3), Wi = max(SW * InnerRatio * wMul, 0.0);
bool fum = FUM(uv);
if (fum) Wi = 0.0;
float Wmax = max(Ws, Wi) + 0.5 + Soft;
float d0 = LK(uv, 1).r;
float3 n0 = normalize(LK(uv, 8).rgb);
// Bord de COULEUR = changement de teinte (chromaticité de la couleur de base), jamais de ton : un même ton plus clair
// ou plus sombre (ombre peinte dans la texture, frontière ombre/lumière) ne déclenche pas de trait (Vincent, 23/11).
float3 c0 = LK(uv, 5).rgb;
float s0 = c0.r + c0.g + c0.b;
float3 ch0 = c0 / max(s0, 1e-3);
float dS = 99, dI = 99, hits = 0;
[unroll] for (int k = 0; k < 12; k++) {
    float a = k * 0.5235988;
    float2 dir = float2(cos(a), sin(a));
    [loop] for (float r = 0.5; r <= Wmax; r += 0.5) {
        float2 q = uv + dir * px * r;
        float dq = LK(q, 1).r;
        // Bord = voisin plus loin (fond ou partie cel derrière). Un décor PLUS PROCHE (devant le personnage) n'est
        // pas un bord : le cel passe dessous, sans trait le long de l'élément qui le cache.
        // v6e (Vincent : pieds / doigts non détourés) : tout voisin hors celluloïd qui n'est pas DEVANT (sol, mur, fond)
        // est un bord de silhouette, même à profondeur presque égale (pied posé sur le sol).
        if (dq < d0 * (1.0 - DepthTol) && !VIS(q)) break;
        if ((dq > d0 * (1.0 + DepthTol) && !(fum && FUM(q))) || !VIS(q)) { dS = min(dS, r - 0.25); hits += 1; break; }
        if (!fum && r <= Wi + Soft) {
            float3 nq = normalize(LK(q, 8).rgb);
            // Trait intérieur = pli (normales du GBuffer, normal maps comprises) ou changement de teinte ; jamais de ton.
            float3 cq = LK(q, 5).rgb;
            float sq = cq.r + cq.g + cq.b;
            float chd = (min(s0, sq) > 0.06) ? length(cq / max(sq, 1e-3) - ch0) : 0.0;   // teinte, ignorée sur le quasi-noir
            if (VIS(q) && (chd > ChromaTol || dot(n0, nq) < CreaseCos)) { dI = min(dI, r - 0.25); hits += 1; break; }
        }
    }
}
float aS = saturate((Ws * 0.5 + 0.5 * Soft - dS) / Soft);   // v6e : moitié intérieure (l'autre est sur le décor)
float aI = saturate((Wi * 0.5 + 0.5 * Soft - dI) / Soft) * InnerAlpha;     // trait intérieur centré : la moitié de chaque côté
// Cassures de trait (bruit monde, plus fréquent)
float br = Texture2DSample(NoiseTex, NoiseTexSampler, nuv * 3.7 + 0.31).g;
float bf = saturate(1.0 - Breaks * saturate((br - 0.55) * 6.0));
aS *= bf * Opacity; aI *= bf * Opacity;
float a = max(aS, aI);
if (DebugView > 1.5) return a.xxx + keep;   // 2 = masque du trait
// Encre (v6) : un trait = brun très foncé de luminance absolue InkValue ; les couches se cumulent jusqu'au quasi-noir.
float3 ink = InkValue * InkHue.rgb / max(dot(InkHue.rgb, float3(0.299, 0.587, 0.114)), 1e-4);
float3 c = lerp(under, min(under, ink), a);
float dens = saturate((hits / 12.0 - DenseLo) / max(1.0 - DenseLo, 1e-3));
float over = aS * aI + a * dens * Dense;
return c * pow(CumulMul, over) + keep;
'''
cu = node(unreal.MaterialExpressionCustom, -300, 0, code=CODE, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="Trait celluloïd EON v6 encre")
params = [("WP", wp), ("NoiseTex", tn), ("InkHue", ih), ("Zoom", zm), ("ST0", st["PPI0"]), ("ST1", st["WN"]), ("ST2", st["CS"]), ("ST3", st["SD"]), ("ST4", st["BC"]), ("ST5", st["CD"])]
# Valeurs du 23/11 : base E1·D3 choisie par Vincent, grossie ×1,5 (G3, retenu), plus de modulation par l'ombre,
# traits intérieurs plus faciles à détecter  ; ombre portée sur le décor à 50 %, décalée de 4 px (Vincent, 23/11) ; pression de main ±27 % (trait moins régulier).
scal = [("SilWidth", 3.9), ("InnerRatio", 0.5), ("SilMin", 2.0), ("SilMax", 3.0), ("InnerAlpha", 1.0), ("DepthTol", 0.01), ("OutTol", 0.03), ("CreaseCos", 0.7), ("ChromaTol", 0.12),
        ("Jitter", 0.27), ("HandScale", 0.5),
        ("Breaks", 0.1), ("Soft", 1.2), ("InkValue", 0.02), ("CumulMul", 0.25), ("Dense", 1.0), ("DenseLo", 0.55), ("Opacity", 1.0), ("DebugView", 0.0),
        ("ShadowStrength", 0.5), ("ShadowOffX", 4.0), ("ShadowOffY", 4.0), ("ShadowSoft", 1.0)]
inputs = []
for nm in [p[0] for p in params] + [s[0] for s in scal]:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); inputs.append(ci)
cu.set_editor_property("inputs", inputs)
for nm, n in params:
    MEL.connect_material_expressions(n, "", cu, nm)
for i, (nm, v) in enumerate(scal):
    MEL.connect_material_expressions(node(unreal.MaterialExpressionScalarParameter, -650, 900 + i * 70, parameter_name=nm, default_value=v), "", cu, nm)
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(g)
EAL.save_loaded_asset(g)
unreal.log("EON_TRAIT M_PP_EON_Trait v6 prêt")
