# EON — passe « film » finale (Vincent, 23/11, d'après Orbitals : 3 niveaux = décor, celluloïds par-dessus, puis
# traitement final). Après le tonemapping (image d'affichage 0-1) :
#   - bave des couleurs claires : flou des parties au-dessus de BleedThreshold, ajouté en « écran » (BleedStrength,
#     BleedRadius px) -> les personnages clairs débordent doucement sur ce qui les entoure ;
#   - halation : même lueur, teintée rouge-orangé, plus large (HalationStrength) ;
#   - aberration chromatique légère, radiale (ChromaPx au bord de l'image) ;
#   - grain de film : texture T_EON_FilmGrain, repositionnée 24 fois par seconde (GrainStrength, GrainScale), plus
#     présent dans les tons moyens.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_film.py
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Materials"
path = DIR + "/M_PP_EON_Film"
if EAL.does_asset_exist(path):
    g = unreal.load_asset(path); MEL.delete_all_material_expressions(g)
else:
    g = AT.create_asset("M_PP_EON_Film", DIR, unreal.Material, unreal.MaterialFactoryNew())
g.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
g.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)


def node(cls, x, y, **kw):
    n = MEL.create_material_expression(g, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n


st0 = node(unreal.MaterialExpressionSceneTexture, -900, -300, scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
tg = node(unreal.MaterialExpressionTextureObjectParameter, -900, 0, parameter_name="GrainTex",
          texture=unreal.load_asset("/Game/EON/Textures/PostProcess/T_EON_FilmGrain"))
tm = node(unreal.MaterialExpressionTime, -900, 150)

CODE = r'''
float2 uv = GetViewportUV(Parameters);
float keep = ST0.r * 0.0;
float2 px = View.ViewSizeAndInvSize.zw;
#define LK(u) SceneTextureLookup(ClampSceneTextureUV(ViewportUVToSceneTextureUV(u, 14), 14), 14, false).rgb
// Aberration chromatique radiale
float2 fromC = uv - 0.5;
float2 ca = fromC * 2.0 * ChromaPx * px;
float3 c = float3(LK(uv + ca).r, LK(uv).g, LK(uv - ca).b);
// Bave + halation : 2 anneaux de 12 lectures, parties claires seulement
float3 glow = 0, halo = 0; float wg = 0, wh = 0;
[unroll] for (int k = 0; k < 12; k++) {
    float a = k * 0.5235988 + 0.26;
    float2 d = float2(cos(a), sin(a));
    [unroll] for (int r = 1; r <= 2; r++) {
        float3 s = LK(uv + d * px * BleedRadius * r * 0.5);
        float3 b = max(s - BleedThreshold, 0.0) / max(1.0 - BleedThreshold, 0.01);
        float w = r == 1 ? 1.0 : 0.6;
        glow += b * w; wg += w;
        float3 sh = LK(uv + d * px * BleedRadius * r * 1.25);
        halo += max(dot(sh, float3(0.333, 0.333, 0.333)) - BleedThreshold, 0.0) * w; wh += w;
    }
}
glow /= wg; halo /= wh;
float3 add = glow * BleedStrength + halo * HalationStrength * float3(1.0, 0.35, 0.15);
c = 1.0 - (1.0 - c) * (1.0 - saturate(add));
// Grain de film, nouvelle position 24 fois par seconde
float f = floor(T * 24.0);
float2 jit = frac(float2(f * 0.6180339, f * 0.4142135)) * 7.0;
float2 g2 = uv * View.ViewSizeAndInvSize.xy / (256.0 * GrainScale) + jit;
float n = Texture2DSample(GrainTex, GrainTexSampler, g2).r - 0.5;
float lum = dot(c, float3(0.299, 0.587, 0.114));
c += n * GrainStrength * (0.35 + 0.65 * (1.0 - abs(lum * 2.0 - 1.0)));
return saturate(c) + keep;
'''
cu = node(unreal.MaterialExpressionCustom, -300, 0, code=CODE, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="Film EON")
params = [("GrainTex", tg), ("T", tm), ("ST0", st0)]
# Valeurs retenues par Vincent le 23/11 : F4 (bave moyenne + grain moyen + halation + aberration chromatique).
scal = [("BleedThreshold", 0.5), ("BleedStrength", 0.4), ("BleedRadius", 6.0), ("HalationStrength", 0.25),
        ("ChromaPx", 1.5), ("GrainStrength", 0.07), ("GrainScale", 1.0)]
inputs = []
for nm in [p[0] for p in params] + [s[0] for s in scal]:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); inputs.append(ci)
cu.set_editor_property("inputs", inputs)
for nm, n in params:
    MEL.connect_material_expressions(n, "", cu, nm)
for i, (nm, v) in enumerate(scal):
    MEL.connect_material_expressions(node(unreal.MaterialExpressionScalarParameter, -650, 300 + i * 70, parameter_name=nm, default_value=v), "", cu, nm)
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(g)
EAL.save_loaded_asset(g)
unreal.log("EON_FILM M_PP_EON_Film prêt")
