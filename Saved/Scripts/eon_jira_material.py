# EON — matériau de Jira = Captain.mat d'Unity (AnimeShader.shadergraph) :
#   texture captain_v0.1b_Material.001_AlbedoTransparency ; Toon_Ramp_Tint (1, 0,80, 0,51) ; seuil 0,5, transition franche.
#   Unity : éclairé = tex × (1 + tint), ombre = tex × tint.  Unreal : Tint de la MI = (1 + tint) / 1,49 (le soleil donne ×1,49),
#   rampe d'ombre par canal = tint / (1 + tint).
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
LOG = []
tint = (1.0, 0.80146426, 0.50628924)
tp_path = "/Game/EON/Materials/TP_EON_Anime"
tp = unreal.load_asset(tp_path) if EAL.does_asset_exist(tp_path) else AT.create_asset("TP_EON_Anime", "/Game/EON/Materials", unreal.ToonProfile, unreal.ToonProfileFactory())
unreal.EONStatics.configure_toon_profile(tp, 0.5, 0.01, unreal.LinearColor(*[t / (1 + t) for t in tint], 1.0), 0.35, True)
EAL.save_loaded_asset(tp)

DIR = "/Game/EON/Characters/Jira"
p = DIR + "/MI_Jira"
mi = unreal.load_asset(p) if EAL.does_asset_exist(p) else AT.create_asset("MI_Jira", DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mi.set_editor_property("parent", unreal.load_asset("/Game/EON/Unity/Materials/M_UnityLit"))
reg = unreal.AssetRegistryHelpers.get_asset_registry()
alb = None
for ad in reg.get_assets_by_path("/Game/EON/Unity/import/textures/chara", recursive=True):
    if str(ad.asset_name).startswith("captain_v0_1b_Material_001_AlbedoTransparency"):
        alb = ad.get_asset()
LOG.append("albédo : %s" % (alb.get_path_name() if alb else None))
MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", alb)
MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*[(1 + t) / 1.49 for t in tint], 1.0))
ok = unreal.EONStatics.set_instance_toon_profile(mi, tp)
LOG.append("profil toon surchargé : %s" % ok)
EAL.save_loaded_asset(mi)

sm = unreal.load_asset("/Game/EON/Unity/import/fbx/chara/captain_v0_2")
mats = list(sm.get_editor_property("materials"))
for s in mats:
    LOG.append("emplacement %s : %s -> MI_Jira" % (s.get_editor_property("material_slot_name"), s.get_editor_property("material_interface").get_name() if s.get_editor_property("material_interface") else None))
    s.set_editor_property("material_interface", mi)
sm.set_editor_property("materials", mats)
EAL.save_loaded_asset(sm)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_jira_material.txt", "w", encoding="utf-8").write("\n".join(LOG))
