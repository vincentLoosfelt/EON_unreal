# EON — instance MI_PP_EON_Film de M_PP_EON_Film (Vincent, 23/11 : « bloom et film grain par-dessus l'ensemble »),
# pour régler le grain sans toucher le matériau. Commandlet.
import unreal
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
p = "/Game/EON/Materials/MI_PP_EON_Film"
if not EAL.does_asset_exist(p):
    mi = AT.create_asset("MI_PP_EON_Film", "/Game/EON/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, unreal.load_asset("/Game/EON/Materials/M_PP_EON_Film"))
    EAL.save_loaded_asset(mi)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_film_mi.txt", "w").write("ok %s" % EAL.does_asset_exist(p))
