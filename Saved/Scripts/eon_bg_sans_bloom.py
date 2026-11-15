# EON — M_EON_BG : émission du décor (aplat + liseré) limitée à 1, sur place (MI et cartes intactes).
# Avant : saturate(aplat) + saturate(liseré) montait à 2 ; avec l'exposition, le décor dépassait le seuil du bloom.
# En bas de l'écran (faces vues presque de profil, liseré allumé partout) le bloom en faisait des taches rouges
# (orange² des caisses à rayures) et vertes. EmisAdd (bandeaux lumineux Room_light) reste au-dessus de 1 : bloom voulu.
# Éditeur fermé : UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_bg_sans_bloom.py
import unreal
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/EON/Materials/M_EON_BG")
outer = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
ins = MEL.get_inputs_for_material_expression(m, outer)
names = [i.get_class().get_name() if i else None for i in ins]
unreal.log("EON_BGSB entrées de l'émission : %s" % names)
if ins and ins[0] and isinstance(ins[0], unreal.MaterialExpressionSaturate):
    unreal.log("EON_BGSB déjà fait")
else:
    inner = ins[0]
    sat = MEL.create_material_expression(m, unreal.MaterialExpressionSaturate, -80, -100)
    MEL.connect_material_expressions(inner, "", sat, "")
    MEL.connect_material_expressions(sat, "", outer, "A")
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    unreal.log("EON_BGSB saturate inséré : %s" % [i.get_class().get_name() if i else None for i in MEL.get_inputs_for_material_expression(m, outer)])
