# Rampe toon BG d'Unity dans TP_EON_BG (ombre = 0,49/1,49 de l'éclairé).
import unreal
tp = unreal.load_asset("/Game/EON/Materials/TP_EON_BG")
unreal.EONStatics.configure_toon_profile(tp, 0.682, 0.085, unreal.LinearColor(0.49 / 1.49, 0.49 / 1.49, 0.49 / 1.49, 1.0), 0.35, True)
unreal.EditorAssetLibrary.save_loaded_asset(tp)
