# EON — Étincelles d'impact en traînées orange (Vincent, 26/11 : « les étincelles doivent être des trails orange »).
# M_EON_Trainee : aiguille (losange étiré |x| + |y| < seuil), cœur blanc-jaune, bord net ; rétrécit en fin de vie.
# NS_EON_Etincelles : copie de NS_EON_GerbeDir, sprites alignés sur la vitesse (la longueur du sprite suit le
# mouvement) ; User.Size = (largeur, longueur) en cm.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
# (Unreal plante à la fermeture après « Python script executed successfully » : sans effet, tout est sauvé.)
import unreal
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
L = unreal.EONVfxLib
DOS = "/Game/EON/VFX"

# ---------------------------------------------------------------- matériau
mp = DOS + "/M_EON_Trainee"
if EAL.does_asset_exist(mp):
    m = unreal.load_asset(mp); MEL.delete_all_material_expressions(m)
else:
    m = AT.create_asset("M_EON_Trainee", DOS, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("two_sided", True)
m.set_editor_property("used_with_niagara_sprites", True)
m.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_AFTER_DOF)
E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
C = MEL.connect_material_expressions

pc = E(unreal.MaterialExpressionParticleColor, -1400, -300)
rt = E(unreal.MaterialExpressionParticleRelativeTime, -1400, 400)
boost = E(unreal.MaterialExpressionScalarParameter, -600, -450)
boost.set_editor_property("parameter_name", "Boost"); boost.set_editor_property("default_value", 5.0)
uv = E(unreal.MaterialExpressionTextureCoordinate, -1400, 100)
m2 = E(unreal.MaterialExpressionMultiply, -1250, 100); m2.set_editor_property("const_b", 2.0); C(uv, "", m2, "A")
s1 = E(unreal.MaterialExpressionSubtract, -1130, 100); s1.set_editor_property("const_b", 1.0); C(m2, "", s1, "A")
ab = E(unreal.MaterialExpressionAbs, -1010, 100); C(s1, "", ab, "")
mr = E(unreal.MaterialExpressionComponentMask, -880, 50)
mr.set_editor_property("r", True); mr.set_editor_property("g", False); mr.set_editor_property("b", False); mr.set_editor_property("a", False)
mg = E(unreal.MaterialExpressionComponentMask, -880, 160)
mg.set_editor_property("r", False); mg.set_editor_property("g", True); mg.set_editor_property("b", False); mg.set_editor_property("a", False)
C(ab, "", mr, ""); C(ab, "", mg, "")
s = E(unreal.MaterialExpressionAdd, -750, 100); C(mr, "", s, "A"); C(mg, "", s, "B")
inv = E(unreal.MaterialExpressionOneMinus, -1250, 400); C(rt, "", inv, "")
k = E(unreal.MaterialExpressionMultiply, -1130, 400); k.set_editor_property("const_b", 1.5); C(inv, "", k, "A")
thr = E(unreal.MaterialExpressionSaturate, -1010, 400); C(k, "", thr, "")

def net(mul, x, y):
    t = E(unreal.MaterialExpressionMultiply, x, y); t.set_editor_property("const_b", mul); C(thr, "", t, "A")
    d = E(unreal.MaterialExpressionSubtract, x + 120, y); C(t, "", d, "A"); C(s, "", d, "B")
    g = E(unreal.MaterialExpressionMultiply, x + 240, y); g.set_editor_property("const_b", 30.0); C(d, "", g, "A")
    r = E(unreal.MaterialExpressionSaturate, x + 360, y); C(g, "", r, "")
    return r

forme = net(1.0, -500, 300)
coeur = net(0.45, -500, 0)
clair = E(unreal.MaterialExpressionConstant3Vector, -500, -200); clair.set_editor_property("constant", unreal.LinearColor(1, 0.9, 0.6, 1))
lp = E(unreal.MaterialExpressionLinearInterpolate, -100, -200)
C(pc, "RGB", lp, "A"); C(clair, "", lp, "B"); C(coeur, "", lp, "Alpha")
em = E(unreal.MaterialExpressionMultiply, 60, -250); C(lp, "", em, "A"); C(boost, "", em, "B")
MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
op = E(unreal.MaterialExpressionMultiply, 60, 300); C(forme, "", op, "A"); C(pc, "A", op, "B")
MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
unreal.log("EON_TRAINEE materiau %s" % EAL.save_loaded_asset(m))

# ---------------------------------------------------------------- système
sp = DOS + "/NS_EON_Etincelles"
if EAL.does_asset_exist(sp):
    EAL.delete_asset(sp)
sy = AT.duplicate_asset("NS_EON_Etincelles", DOS, unreal.load_asset(DOS + "/NS_EON_GerbeDir"))
e = L.get_stateless_emitter(sy, 0)
for p, v in (("RendererProperties[0].Material", m.get_path_name()),
             ("RendererProperties[0].Alignment", "VelocityAligned"),
             ("RendererProperties[0].FacingMode", "FaceCamera")):
    unreal.log("EON_TRAINEE %s = %s : %s" % (p, v, L.set_prop(e, p, v)))
L.finish_system(sy, e)
unreal.log("EON_TRAINEE systeme %s" % EAL.save_loaded_asset(sy))
