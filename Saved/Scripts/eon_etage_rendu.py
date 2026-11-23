# EON — rendu validé du HUB appliqué à L_Etage (Vincent, 24/11 : « 1 » = tout pareil que le HUB). Post-process PP_Etage :
# MI_PP_EON_Decor_B en tête (aquarelle, P5, plafond du bloom), M_PP_EON_Trait, MI_PP_EON_Film (grain, aberration) à la
# place de M_PP_EON_Film ; bloom 6 / seuil 0,8 / taille 8 comme PP_HUB. Les matériaux des salles passent sur
# MI_Peint_*_B dans EONTileRoom.cpp. Sauvegarde : Saved/Sauvegardes/L_Etage_avant_rendu_20261124.umap.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_etage_rendu.py
import unreal
BLOOM = (("bloom_intensity", 6.0), ("bloom_threshold", 0.8), ("bloom_size_scale", 8.0))
decor = unreal.load_asset("/Game/EON/Materials/Peint/MI_PP_EON_Decor_B")
trait = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")
film = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Film")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_Etage")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []
for ppv in [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume)]:
    ppv.modify()
    s = ppv.get_editor_property("settings")
    for k, v in BLOOM:
        s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
    s.set_editor_property("weighted_blendables", unreal.WeightedBlendables([unreal.WeightedBlendable(1.0, decor), unreal.WeightedBlendable(1.0, trait), unreal.WeightedBlendable(1.0, film)]))
    ppv.set_editor_property("settings", s)
    LOG.append("%s : %s" % (ppv.get_actor_label(), [b.object.get_name() for b in ppv.settings.weighted_blendables.array]))
LOG.append("sauvegarde %s" % les.save_current_level())
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_etage_rendu.txt", "w", encoding="utf-8").write("\n".join(LOG))
