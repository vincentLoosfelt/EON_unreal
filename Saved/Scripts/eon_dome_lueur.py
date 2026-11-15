# EON — M_EON_ChampForce, sur place : lueur du dôme baissée (Glow 0,17 -> 0,07) pour le HUB assombri du 15/11 :
# le dôme est non éclairé (additif), il gardait sa luminosité et écrasait la scène.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_dome_lueur.py
import unreal
MEL = unreal.MaterialEditingLibrary
GLOW = 0.07
m = unreal.load_asset("/Game/EON/Materials/M_EON_ChampForce")
vu, pile, n = set(), [MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_EMISSIVE_COLOR)], 0
while pile:
    e = pile.pop()
    if e is None or e.get_name() in vu:
        continue
    vu.add(e.get_name())
    if isinstance(e, unreal.MaterialExpressionScalarParameter) and str(e.get_editor_property("parameter_name")) == "Glow":
        e.set_editor_property("default_value", GLOW); n += 1
    pile += [i for i in MEL.get_inputs_for_material_expression(m, e) if i]
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log("EON_LUEUR Glow = %.2f (%d nœud)" % (GLOW, n))
