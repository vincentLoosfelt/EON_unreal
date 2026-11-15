# EON — lens flare coupé sur tous les PostProcessVolume des cartes EON.
# Le lens flare d'Unreal (actif par défaut, intensité 1) reflète les sprites lumineux à l'opposé du centre
# de l'écran : taches rouges / vertes floues collées au bas de l'image. Unity n'en avait pas.
# Éditeur fermé : UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_sans_lens_flare.py
import unreal
for ad in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path("/Game/EON/Maps", recursive=True):
    if str(ad.asset_class_path.asset_name) != "World":
        continue
    path = str(ad.package_name)
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    n = 0
    for a in unreal.EditorLevelLibrary.get_all_level_actors():
        if isinstance(a, unreal.PostProcessVolume):
            s = a.settings
            s.set_editor_property("override_lens_flare_intensity", True)
            s.set_editor_property("lens_flare_intensity", 0.0)
            a.settings = s
            n += 1
    if n:
        unreal.EditorLoadingAndSavingUtils.save_current_level()
    unreal.log("EON_LENS %s : %d volume(s)" % (path, n))
