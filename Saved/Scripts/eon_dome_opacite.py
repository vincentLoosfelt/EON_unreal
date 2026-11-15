# EON — dôme du C.O.M : -30 % d'opacité (Vincent, 16/11). Matériau additif : Glow (multiplie toute l'émission) 1,0 -> 0,7.
import unreal
m = unreal.load_asset("/Game/EON/Materials/M_EON_ChampForce")
n = 0
for e in unreal.MaterialEditingLibrary.get_material_expressions(m) if hasattr(unreal.MaterialEditingLibrary, "get_material_expressions") else []:
    pass
it = unreal.ObjectIterator(unreal.MaterialExpressionScalarParameter) if hasattr(unreal, "ObjectIterator") else []
for e in it:
    if e.get_outer() == m and str(e.get_editor_property("parameter_name")) == "Glow":
        unreal.log("DOME Glow avant %.3f" % e.get_editor_property("default_value"))
        e.set_editor_property("default_value", 0.7)
        n += 1
unreal.MaterialEditingLibrary.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log("DOME Glow modifie : %d" % n)
