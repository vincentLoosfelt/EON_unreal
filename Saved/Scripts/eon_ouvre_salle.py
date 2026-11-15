# EON — ouvre une salle dessinée dans l'éditeur de Blueprint (lancé par -ExecutePythonScript au démarrage de l'éditeur).
import unreal
bp = unreal.load_asset("/Game/EON/Salles/BP_Salle_Combat_A")
if bp:
    unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([bp])
    unreal.log("EON : BP_Salle_Combat_A ouvert")
