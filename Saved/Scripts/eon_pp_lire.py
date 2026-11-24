# EON — lecture des réglages des PostProcessVolume de L_HUB et L_Etage (bloom, saleté d'objectif, vignettage...).
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L = []
for carte in ("/Game/EON/Maps/L_HUB", "/Game/EON/Maps/L_Etage"):
    les.load_level(carte)
    for v in [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume)]:
        s = v.settings
        L.append("== %s %s unbound=%s enabled=%s prio=%s" % (carte, v.get_actor_label(), v.unbound, v.enabled, v.priority))
        for nm in dir(s):
            if nm.startswith("override_") and getattr(s, nm):
                k = nm[len("override_"):]
                try: L.append("  %s = %s" % (k, getattr(s, k)))
                except Exception as e: L.append("  %s ?" % k)
        for k in ("bloom_method", "bloom_convolution_texture", "bloom_dirt_mask", "bloom_dirt_mask_intensity", "vignette_intensity", "lens_flare_intensity"):
            L.append("  [%s] %s" % (k, getattr(s, k)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_pp_lire.txt", "w", encoding="utf-8").write("\n".join(L))
