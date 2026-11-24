# EON — essai : DebugView de MI_PP_EON_Trait (EON_DBG, 0 = normal, 2 = masque du trait).
import os, unreal
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Trait")
v = float(os.environ.get("EON_DBG", "0"))
if v > 0: MEL.set_material_instance_scalar_parameter_value(mi, "DebugView", v)
else: MEL.set_material_instance_scalar_parameter_value(mi, "DebugView", 0.0)
MEL.update_material_instance(mi)
unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_trait_debug.txt", "w").write("DebugView=%s SilWidth=%s" % (v, MEL.get_material_instance_scalar_parameter_value(mi, "SilWidth")))
