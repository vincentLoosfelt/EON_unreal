# EON — pas de motion blur (Vincent, 24/11). Volumes PP_HUB et PP_Etage : quantité forcée à 0 (en plus de
# r.DefaultFeature.MotionBlur=False dans DefaultEngine.ini).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_sans_motion_blur.py
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []
for carte in ("/Game/EON/Maps/L_HUB", "/Game/EON/Maps/L_Etage"):
    les.load_level(carte)
    for v in [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PostProcessVolume)]:
        v.modify()
        s = v.get_editor_property("settings")
        avant = (s.override_motion_blur_amount, s.motion_blur_amount)
        s.set_editor_property("override_motion_blur_amount", True); s.set_editor_property("motion_blur_amount", 0.0)
        v.set_editor_property("settings", s)
        LOG.append("%s %s : motion blur %s -> 0" % (carte, v.get_actor_label(), avant))
    LOG.append("sauvegarde %s" % les.save_current_level())
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_sans_motion_blur.txt", "w", encoding="utf-8").write("\n".join(LOG))
