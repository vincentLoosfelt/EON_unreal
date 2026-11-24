# EON — correctif des effets ABS (24/11) : PreSkinnedPosition n'existe que dans le vertex shader.
# On le fait passer par un VertexInterpolator dans M_EON_ToonMasked (veines) et M_EON_ToonEnnemi (trous).
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
L = []
OUT = os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_fx_abs_vi.txt")
for path, cons in (("/Game/EON/Materials/M_EON_ToonMasked", None), ("/Game/EON/Materials/M_EON_ToonEnnemi", "EON_Trous")):
    m = unreal.load_asset(path)
    ex = MEL.get_material_expressions(m)
    if any(e.get_class().get_name() == "MaterialExpressionVertexInterpolator" for e in ex):
        L.append("%s : déjà fait" % path); continue
    psp = [e for e in ex if e.get_class().get_name() == "MaterialExpressionPreSkinnedPosition"]
    if not psp:
        L.append("%s : pas de PreSkinnedPosition" % path); continue
    p = psp[0]
    pos = p.get_editor_property("material_expression_editor_x"), p.get_editor_property("material_expression_editor_y")
    vi = MEL.create_material_expression(m, unreal.MaterialExpressionVertexInterpolator, pos[0] + 150, pos[1])
    L.append("psp -> VI : %s" % MEL.connect_material_expressions(p, "", vi, ""))
    if cons:
        tgt = [e for e in ex if e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == cons][0]
        L.append("VI -> %s.P : %s" % (cons, MEL.connect_material_expressions(vi, "", tgt, "P")))
    else:
        # le Multiply des veines : celui dont l'entrée B est VeinesEchelle
        for e in ex:
            if e.get_class().get_name() != "MaterialExpressionMultiply":
                continue
            ins = list(MEL.get_inputs_for_material_expression(m, e))
            if ins and ins[0] == p:
                L.append("VI -> Multiply.A : %s" % MEL.connect_material_expressions(vi, "", e, "A"))
    MEL.recompile_material(m)
    L.append("%s : %s" % (path, EAL.save_loaded_asset(m)))
open(OUT, "w", encoding="utf-8").write("\n".join(L))
