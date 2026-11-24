# EON — bloom et grain sur L_HUB (Vincent, 23/11) : « le même bloom qu'Orbitals » = seules les sources lumineuses
# (néons, écrans, dôme) rayonnent, ni voile sur le décor sombre ni sur les personnages -> bloom Unreal à seuil
# (BLOOM_SEUIL, au-dessus du blanc d'affichage), BLOOM_INTENSITE ; grain de 2 px au niveau 0,15 (MI_PP_EON_Film, mis
# à la place de M_PP_EON_Film dans le volume). Commandlet.
import unreal
BLOOM_INTENSITE, BLOOM_SEUIL, BLOOM_TAILLE = 6.0, 0.8, 8.0   # Vincent, 23/11 : seuil 0,8, grand rayon (défaut Unreal 4) ; intensité 2, 4, puis 6
# (le clamp du bloom est dans M_PP_EON_Decor : paramètre BloomMax)
GRAIN = {"GrainStrength": 0.15, "GrainScale": 2.0, "ChromaPx": 2.0}   # aberration 2 px au bord en 4K (Vincent, 24/11 : écart divisé par 2)
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Film")
for k, v in GRAIN.items():
    MEL.set_material_instance_scalar_parameter_value(mi, k, v)
MEL.update_material_instance(mi)
unreal.EditorAssetLibrary.save_loaded_asset(mi)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []
for ppv in [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume)]:
    ppv.modify()
    s = ppv.get_editor_property("settings")
    for k, v in (("bloom_intensity", BLOOM_INTENSITE), ("bloom_threshold", BLOOM_SEUIL), ("bloom_size_scale", BLOOM_TAILLE)):
        s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
    bl = [unreal.WeightedBlendable(1.0, mi) if (b.get_editor_property("object") and b.get_editor_property("object").get_name() == "M_PP_EON_Film") else b
          for b in s.get_editor_property("weighted_blendables").get_editor_property("array")]
    s.set_editor_property("weighted_blendables", unreal.WeightedBlendables(bl))
    ppv.set_editor_property("settings", s)
    LOG.append("%s : %s" % (ppv.get_actor_label(), [b.get_editor_property("object").get_name() for b in bl if b.get_editor_property("object")]))
LOG.append("sauvegarde %s" % les.save_current_level())
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_hub_film.txt", "w").write("\n".join(LOG))
