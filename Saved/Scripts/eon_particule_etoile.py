# EON — M_EON_Particule : plus de carré pixel (Vincent, 26/11 : « plus aucun pixel dans les VFX, version animé jap 90 »).
# Chaque particule des gerbes (NS_EON_Gerbe, NS_EON_GerbeDir) devient une étoile « kira » à 4 branches concaves
# (astroïde : sqrt|x| + sqrt|y| < seuil), cœur blanc, couleur de la particule autour, bord net. En fin de vie l'étoile
# rétrécit au lieu de s'effacer (seuil = saturate(1,5 × vie restante)), comme une étincelle dessinée.
# Le matériau est modifié en place : ses réglages (translucidité après DOF, Niagara) sont gardés.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
m = unreal.load_asset("/Game/EON/VFX/M_EON_Particule")
MEL.delete_all_material_expressions(m)
E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
C = MEL.connect_material_expressions

pc = E(unreal.MaterialExpressionParticleColor, -1400, -300)
rt = E(unreal.MaterialExpressionParticleRelativeTime, -1400, 400)
boost = E(unreal.MaterialExpressionScalarParameter, -600, -450)
boost.set_editor_property("parameter_name", "Boost")
boost.set_editor_property("default_value", 4.0)

# c = |uv × 2 - 1| ; s = sqrt(cx) + sqrt(cy)
uv = E(unreal.MaterialExpressionTextureCoordinate, -1400, 100)
m2 = E(unreal.MaterialExpressionMultiply, -1250, 100); m2.set_editor_property("const_b", 2.0); C(uv, "", m2, "A")
s1 = E(unreal.MaterialExpressionSubtract, -1130, 100); s1.set_editor_property("const_b", 1.0); C(m2, "", s1, "A")
ab = E(unreal.MaterialExpressionAbs, -1010, 100); C(s1, "", ab, "")
sq = E(unreal.MaterialExpressionSquareRoot, -900, 100); C(ab, "", sq, "")
mr = E(unreal.MaterialExpressionComponentMask, -780, 50)
mr.set_editor_property("r", True); mr.set_editor_property("g", False); mr.set_editor_property("b", False); mr.set_editor_property("a", False)
mg = E(unreal.MaterialExpressionComponentMask, -780, 160)
mg.set_editor_property("r", False); mg.set_editor_property("g", True); mg.set_editor_property("b", False); mg.set_editor_property("a", False)
C(sq, "", mr, ""); C(sq, "", mg, "")
s = E(unreal.MaterialExpressionAdd, -650, 100); C(mr, "", s, "A"); C(mg, "", s, "B")

# seuil = saturate(1,5 × (1 - âge))
inv = E(unreal.MaterialExpressionOneMinus, -1250, 400); C(rt, "", inv, "")
k = E(unreal.MaterialExpressionMultiply, -1130, 400); k.set_editor_property("const_b", 1.5); C(inv, "", k, "A")
thr = E(unreal.MaterialExpressionSaturate, -1010, 400); C(k, "", thr, "")

def net(seuil_mul, x, y):
    """saturate((seuil × mul - s) × 60) : 1 dans la forme, 0 dehors, bord net."""
    t = E(unreal.MaterialExpressionMultiply, x, y); t.set_editor_property("const_b", seuil_mul); C(thr, "", t, "A")
    d = E(unreal.MaterialExpressionSubtract, x + 120, y); C(t, "", d, "A"); C(s, "", d, "B")
    g = E(unreal.MaterialExpressionMultiply, x + 240, y); g.set_editor_property("const_b", 60.0); C(d, "", g, "A")
    r = E(unreal.MaterialExpressionSaturate, x + 360, y); C(g, "", r, "")
    return r

forme = net(1.0, -500, 300)
coeur = net(0.5, -500, 0)

# émissif = lerp(couleur, blanc, cœur) × Boost
blanc = E(unreal.MaterialExpressionConstant3Vector, -500, -200); blanc.set_editor_property("constant", unreal.LinearColor(1, 1, 1, 1))
lp = E(unreal.MaterialExpressionLinearInterpolate, -100, -200)
C(pc, "RGB", lp, "A"); C(blanc, "", lp, "B"); C(coeur, "", lp, "Alpha")
em = E(unreal.MaterialExpressionMultiply, 60, -250); C(lp, "", em, "A"); C(boost, "", em, "B")
MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
op = E(unreal.MaterialExpressionMultiply, 60, 300); C(forme, "", op, "A"); C(pc, "A", op, "B")
MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)

MEL.recompile_material(m)
unreal.log("EON_ETOILE sauvegarde %s pass=%s" % (EAL.save_loaded_asset(m), m.get_editor_property("translucency_pass")))
