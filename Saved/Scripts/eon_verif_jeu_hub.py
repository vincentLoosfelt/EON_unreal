# EON — vérifie en PIE que le jeu garde le rendu validé du HUB (24/11) : blendables et poids appliqués par le
# PlayerController, bloom, persos animés en paliers (intervalle de tick des squelettes = 1/CelAnimFps), capture.
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_verif_jeu_hub.py"
import unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_verif_jeu_hub.txt"
SHOT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Film/Verif_PIE_HUB.png"
LOG = []
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
st = {"n": 0}


def log(m):
    LOG.append(str(m)); unreal.log("EON_VERIF " + str(m))


def monde_pie():
    for w in unreal.EditorLevelLibrary.get_pie_worlds(False) if hasattr(unreal.EditorLevelLibrary, "get_pie_worlds") else []:
        return w
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def verifier():
    w = monde_pie()
    log("monde : %s" % (w.get_name() if w else None))
    for v in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.PostProcessVolume):
        s = v.settings
        log("%s actif=%s bloom=%.2f seuil=%.2f taille=%.2f" % (v.get_name(), v.enabled, s.bloom_intensity, s.bloom_threshold, s.bloom_size_scale))
        for b in s.weighted_blendables.array:
            log("  blendable %s poids %.2f" % (b.object.get_name() if b.object else None, b.weight))
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
        for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
            n += 1
            log("squelette %s.%s intervalle=%.4f s stencil=%s" % (a.get_name(), c.get_name(), c.get_component_tick_interval(), c.get_editor_property("custom_depth_stencil_value")))
    log("squelettes : %d" % n)


def tick(dt):
    st["n"] += 1
    n = st["n"]
    if n == 300:
        les.editor_request_begin_play()
    elif n == 900:
        try:
            verifier()
        except Exception as e:
            log("erreur : %s" % e)
        unreal.SystemLibrary.execute_console_command(monde_pie(), "HighResShot 1920x1080 filename=" + SHOT)
    elif n == 1100:
        les.editor_request_end_play()
    elif n == 1200:
        open(OUT, "w", encoding="utf-8").write("\n".join(LOG))
        unreal.unregister_slate_post_tick_callback(h)
        unreal.SystemLibrary.quit_editor()


h = unreal.register_slate_post_tick_callback(tick)
