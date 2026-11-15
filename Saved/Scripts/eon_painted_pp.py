# EON — passe « peinture » d'Unity (PaintingEffect.shader + PaintingEffectMaterial.mat) en post-process Unreal.
# Avant le tonemapping (Unity : BeforeRenderingPostProcessing, HDR). Valeurs du .mat :
#   flou gaussien 5×5 brushSize 0,186 ; distorsion wet_normal 0,1, mapping monde XZ tiling 0,437 ;
#   Sobel sur normales largeur 0,19 force 0,1 (×8), teinte adaptée à la luminance ; grain 0,00174 échelle 3 ;
#   décalage de rendu (-2,9 ; -0,3) texels. Masque « celluloïd » (ennemis) : CustomStencil > 0 -> image d'origine.
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
TEX = "/Game/EON/Textures/PostProcess"
SRC = "D:/_PROJETS/briscard/GIT/EON_Project/Assets/"


def imp(src, name):
    t = unreal.AssetImportTask()
    t.filename = SRC + src; t.destination_path = TEX; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    AT.import_asset_tasks([t])
    a = unreal.load_asset(TEX + "/" + name)
    a.set_editor_property("srgb", False)
    a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    EAL.save_loaded_asset(a)
    return a


wet = imp("Settings/wet_normal (1).png", "T_EON_WetNormal")
grain = imp("import/textures/FX/FilmGrainNoise.png", "T_EON_FilmGrain")

path = "/Game/EON/Materials/M_PP_EON_Peinture"
if EAL.does_asset_exist(path):
    EAL.delete_asset(path)
m = AT.create_asset("M_PP_EON_Peinture", "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)


def node(cls, x, y, **kw):
    n = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        n.set_editor_property(k, v)
    return n


# Nœuds SceneTexture présents pour activer PostProcessInput0, WorldNormal, CustomStencil, profondeur dans le Custom
st = {}
for i, (nm, idx) in enumerate((("PPI0", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0), ("WN", unreal.SceneTextureId.PPI_WORLD_NORMAL),
                               ("CS", unreal.SceneTextureId.PPI_CUSTOM_STENCIL), ("SD", unreal.SceneTextureId.PPI_SCENE_DEPTH))):
    st[nm] = node(unreal.MaterialExpressionSceneTexture, -900, -300 + i * 150, scene_texture_id=idx)

wp = node(unreal.MaterialExpressionWorldPosition, -900, 350)
tw = node(unreal.MaterialExpressionTextureObjectParameter, -900, 500, parameter_name="BrushTex", texture=wet)
tg = node(unreal.MaterialExpressionTextureObjectParameter, -900, 650, parameter_name="NoiseTex", texture=grain)
tm = node(unreal.MaterialExpressionTime, -900, 800)

CODE = r'''
float2 uv = GetDefaultSceneTextureUV(Parameters, 14);
float keep = (ST0.r + ST1.r + ST2.r + ST3.r) * 0.0;   // garde les SceneTexture dans le graphe
float2 px = View.BufferSizeAndInvSize.zw;
float3 orig = SceneTextureLookup(uv, 14, false).rgb;
if (SceneTextureLookup(uv, 25, false).r > 0.5) return orig;      // celluloïd : ennemis, sbires
float hdrBoost = max(1.0, max(orig.r, max(orig.g, orig.b)));
// distorsion « wet » : mapping monde XZ d'Unity = XY d'Unreal, unités Unity = m
float2 buv = WP.xy * 0.01 * BrushTiling * 0.1;
float2 dist = (Texture2DSample(BrushTex, BrushTexSampler, buv).xy * 2.0 - 1.0) * BrushDistortion * px * 10.0;
float2 edist = dist * EdgeBrushDistortion;
float2 roff = RenderOffset * px;
float3 sum = 0; float tw = 0;
[unroll] for (int x = -2; x <= 2; x++) [unroll] for (int y = -2; y <= 2; y++) {
    float2 o = float2(x, y) * px * BrushSize;
    float w = exp(-dot(o, o) * 2.0);
    sum += SceneTextureLookup(uv + o + dist + roff, 14, true).rgb * w; tw += w; }
float3 painted = sum / tw * hdrBoost;
// Sobel sur normales, anti-crénelé (5 échantillons)
float e = 0;
float2 ho = px * 0.5;
float2 offs[5] = { float2(0,0), float2(ho.x,0), float2(0,ho.y), float2(-ho.x,0), float2(0,-ho.y) };
float wts[5] = { 2, 1, 1, 1, 1 };
[unroll] for (int k = 0; k < 5; k++) {
    float2 c = uv + offs[k] + edist; float2 t = px * EdgeWidth;
    float3 n00 = SceneTextureLookup(c + float2(-1,-1)*t, 8, false).rgb, n01 = SceneTextureLookup(c + float2(0,-1)*t, 8, false).rgb, n02 = SceneTextureLookup(c + float2(1,-1)*t, 8, false).rgb;
    float3 n10 = SceneTextureLookup(c + float2(-1,0)*t, 8, false).rgb, n12 = SceneTextureLookup(c + float2(1,0)*t, 8, false).rgb;
    float3 n20 = SceneTextureLookup(c + float2(-1,1)*t, 8, false).rgb, n21 = SceneTextureLookup(c + float2(0,1)*t, 8, false).rgb, n22 = SceneTextureLookup(c + float2(1,1)*t, 8, false).rgb;
    float3 sx = n00 + 2*n10 + n20 - n02 - 2*n12 - n22; float3 sy = n00 + 2*n01 + n02 - n20 - 2*n21 - n22;
    e += sqrt(dot(sx,sx) + dot(sy,sy)) * wts[k]; }
float edge = saturate(e / 6.0 * 8.0);
float2 lo = px * 2.0; float3 L = float3(0.299, 0.587, 0.114);
float avgL = (dot(SceneTextureLookup(uv + float2(-lo.x,0), 14, false).rgb, L) + dot(SceneTextureLookup(uv + float2(lo.x,0), 14, false).rgb, L)
            + dot(SceneTextureLookup(uv + float2(0,-lo.y), 14, false).rgb, L) + dot(SceneTextureLookup(uv + float2(0,lo.y), 14, false).rgb, L)) * 0.25;
float3 adapt = lerp(0, 1, smoothstep(0.25, 0.75, avgL));
float3 tint = lerp(painted * 0.3, adapt, EdgeLumAdapt);
float3 col = lerp(orig, lerp(painted, tint, saturate(edge * EdgeStrength)), PaintStrength);
// grain
float2 sp = uv * View.BufferSizeAndInvSize.xy;
float2 nuv = (sp + frac(T) * 1000.0) * NoiseScale / View.BufferSizeAndInvSize.y;
float nz = (Texture2DSample(NoiseTex, NoiseTexSampler, nuv).r - 0.5) * 2.0;
col += nz * NoiseStrength * lerp(1.0, 1.0 - edge, NoiseMask);
return col + keep;
'''
cu = node(unreal.MaterialExpressionCustom, -300, 0, code=CODE, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="Peinture EON (Unity)")
# Les SceneTexture doivent être branchées, sinon le compilateur les retire et SceneTextureLookup n'existe pas.
params = [("WP", wp), ("BrushTex", tw), ("NoiseTex", tg), ("T", tm), ("ST0", st["PPI0"]), ("ST1", st["WN"]), ("ST2", st["CS"]), ("ST3", st["SD"])]
scal = [("BrushTiling", 0.437), ("BrushDistortion", 0.1), ("EdgeBrushDistortion", 0.613), ("BrushSize", 0.186), ("EdgeWidth", 0.19),
        ("EdgeStrength", 0.1), ("EdgeLumAdapt", 1.0), ("PaintStrength", 1.0), ("NoiseScale", 3.0), ("NoiseStrength", 0.00174), ("NoiseMask", 1.0)]
inputs = []
for nm, _ in params + [(s, 0) for s, _ in scal] + [("RenderOffset", 0)]:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", nm); inputs.append(ci)
cu.set_editor_property("inputs", inputs)
for nm, n in params:
    MEL.connect_material_expressions(n, "", cu, nm)
for i, (nm, v) in enumerate(scal):
    MEL.connect_material_expressions(node(unreal.MaterialExpressionScalarParameter, -650, 900 + i * 70, parameter_name=nm, default_value=v), "", cu, nm)
ro = node(unreal.MaterialExpressionConstant2Vector, -650, 1700, r=-2.9, g=-0.3)
MEL.connect_material_expressions(ro, "", cu, "RenderOffset")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
unreal.log("EON_PP M_PP_EON_Peinture créé")
