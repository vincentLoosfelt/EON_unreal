# EON — grain du film (MI_PP_EON_Film, HUB et étage). Vincent, 24/11 : « le film grain est trop gros et trop fort ».
# 23/11 : 0,15 à 2 px ; 24/11 : EON_GRAIN_FORCE / EON_GRAIN_TAILLE (défaut 0,07 à 1 px).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_film_grain.py
import os, unreal
FORCE = float(os.environ.get("EON_GRAIN_FORCE", "0.07"))
TAILLE = float(os.environ.get("EON_GRAIN_TAILLE", "1.0"))
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset("/Game/EON/Materials/MI_PP_EON_Film")
avant = (MEL.get_material_instance_scalar_parameter_value(mi, "GrainStrength"), MEL.get_material_instance_scalar_parameter_value(mi, "GrainScale"))
MEL.set_material_instance_scalar_parameter_value(mi, "GrainStrength", FORCE)
MEL.set_material_instance_scalar_parameter_value(mi, "GrainScale", TAILLE)
MEL.update_material_instance(mi)
ok = unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_film_grain.txt", "w", encoding="utf-8").write(
    "avant force %.3f taille %.2f ; après force %.3f taille %.2f ; sauvegarde %s" % (avant[0], avant[1], FORCE, TAILLE, ok))
