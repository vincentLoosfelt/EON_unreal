# EON — émissions bleues du HUB renforcées (Vincent, 23/11 : « plus d'intensité et plus saturé pour faire ressortir un
# halo bleu grâce au bloom »). Les matériaux d'origine ne sont pas touchés : instances enfants MI_HUB_*_Bleu dans
# /Game/EON/Materials/HUB, mises à leur place dans L_HUB. EON_NEON_RETOUR=1 remet les originaux.
import os, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DIR = "/Game/EON/Materials/HUB"
K = float(os.environ.get("EON_NEON_K", "4"))   # facteur d'intensité
RETOUR = os.environ.get("EON_NEON_RETOUR", "0") == "1"
# (original, nouvelle instance, paramètres vecteur, paramètres scalaires) — bleu plus pur : vert réduit, rouge nul
BLEUS = [("/Game/EON/Unity/import/fbx/enviro/electro", "MI_HUB_electro_Bleu", {"EmissiveColor": (0.0, 0.45 * K, 5.0 * K)}, {}),
         ("/Game/EON/Unity/import/fbx/enviro/light", "MI_HUB_light_Bleu", {"EmissiveColor": (0.0, 0.9 * K, 6.0 * K)}, {}),
         ("/Game/EON/Unity/Materials/MI_Unity_Porte", "MI_HUB_Porte_Bleu", {"EmisColor": (0.25, 0.55, 1.0)}, {"EmisStrength": 20.0 * K})]
LOG = []
swap = {}
for src, nom, vec, sca in BLEUS:
    par = unreal.load_asset(src)
    p = DIR + "/" + nom
    mi = unreal.load_asset(p) if EAL.does_asset_exist(p) else AT.create_asset(nom, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, par)
    for k, v in vec.items():
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in sca.items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    if RETOUR:
        swap[mi.get_path_name()] = par
    else:
        swap[par.get_path_name()] = mi
    LOG.append("%s -> %s %s %s" % (src, nom, vec, sca))
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.MeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m and m.get_path_name() in swap:
                a.modify(); c.modify(); c.set_material(i, swap[m.get_path_name()]); n += 1
LOG.append("emplacements changés : %d ; sauvegarde %s" % (n, les.save_current_level()))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_hub_neons_bleus.txt", "w", encoding="utf-8").write("\n".join(LOG))
