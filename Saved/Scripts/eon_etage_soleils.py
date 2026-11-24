# EON — lumière de l'étage ramenée à celle du HUB (Vincent, 24/11 : « la lumière est trop forte »). Soleil_principal
# 1,88 -> 0,60 et Soleil_2 1,76 -> 0,20, comme dans L_HUB. Sauvegarde : Saved/Sauvegardes/L_Etage_avant_rendu_20261124.umap.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_etage_soleils.py
import unreal
VALEURS = {"Soleil_principal": 0.60, "Soleil_2": 0.20}
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_Etage")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []
for a in eas.get_all_level_actors():
    if a.get_actor_label() in VALEURS and isinstance(a, unreal.DirectionalLight):
        c = a.light_component
        a.modify(); c.modify()
        LOG.append("%s : %.2f -> %.2f" % (a.get_actor_label(), c.intensity, VALEURS[a.get_actor_label()]))
        c.set_intensity(VALEURS[a.get_actor_label()])
LOG.append("sauvegarde %s" % les.save_current_level())
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_etage_soleils.txt", "w", encoding="utf-8").write("\n".join(LOG))
