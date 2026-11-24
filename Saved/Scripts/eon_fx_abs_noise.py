import unreal, os
MEL = unreal.MaterialEditingLibrary
jm = unreal.load_asset("/Game/EON/Materials/M_EON_ToonMasked")
ex = MEL.get_material_expressions(jm)
nz = [e for e in ex if e.get_class().get_name() == "MaterialExpressionNoise"][0]
mu = [e for e in ex if e.get_class().get_name() == "MaterialExpressionMultiply"]
sc = [m for m in mu if any(i and i.get_class().get_name() == "MaterialExpressionPreSkinnedPosition" for i in MEL.get_inputs_for_material_expression(jm, m))][0]
names = list(MEL.get_material_expression_input_names(nz))
ok = MEL.connect_material_expressions(sc, "", nz, names[0])
MEL.recompile_material(jm)
s = unreal.EditorAssetLibrary.save_loaded_asset(jm)
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_fx_abs_noise.txt"), "w").write("%s %s %s" % (names, ok, s))
