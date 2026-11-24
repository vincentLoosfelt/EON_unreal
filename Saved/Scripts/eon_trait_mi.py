# EON — épaisseur globale du trait des celluloïds (Vincent, 24/11 : « baisse l'épaisseur globale de l'artline de 50 % »).
# M_PP_EON_Trait n'est pas touché : instance MI_PP_EON_Trait (SilWidth et InnerWidth × EON_TRAIT_K, 0,5 par défaut) mise à
# sa place dans les volumes de L_HUB et L_Etage.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_trait_mi.py
import os, unreal
K = float(os.environ.get("EON_TRAIT_K", "0.5"))
EAL, MEL = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
LOG = []
base = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")
p = "/Game/EON/Materials/MI_PP_EON_Trait"
mi = unreal.load_asset(p) if EAL.does_asset_exist(p) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "MI_PP_EON_Trait", "/Game/EON/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
MEL.set_material_instance_parent(mi, base)
for nm in ("SilWidth", "InnerWidth"):
    v = MEL.get_material_default_scalar_parameter_value(base, nm)
    MEL.set_material_instance_scalar_parameter_value(mi, nm, v * K)
    LOG.append("%s : %.2f -> %.2f px" % (nm, v, v * K))
MEL.update_material_instance(mi)
LOG.append("MI sauvegardée %s" % EAL.save_loaded_asset(mi, False))
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for carte in ("/Game/EON/Maps/L_HUB", "/Game/EON/Maps/L_Etage"):
    les.load_level(carte)
    for v in [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume)]:
        v.modify()
        s = v.get_editor_property("settings")
        bl = [unreal.WeightedBlendable(b.weight, mi) if (b.object and b.object.get_name() == "M_PP_EON_Trait") else b for b in s.weighted_blendables.array]
        s.set_editor_property("weighted_blendables", unreal.WeightedBlendables(bl))
        v.set_editor_property("settings", s)
        LOG.append("%s %s : %s" % (carte, v.get_actor_label(), [b.object.get_name() for b in v.settings.weighted_blendables.array if b.object]))
    LOG.append("sauvegarde %s" % les.save_current_level())
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_trait_mi.txt", "w", encoding="utf-8").write("\n".join(LOG))
