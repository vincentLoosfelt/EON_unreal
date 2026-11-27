# EON — voyants verts et rouges (matériaux Light_green / Light_red importés d'Unity) : émission 7,7 -> 1,54, soit 5 fois
# moins (Vincent, 25/11 : « beaucoup trop forts », d'abord ÷ 2,5 puis ÷ 5 au total, de façon globale pour tous les modules).
# Valeur fixe, pas une division : relancer le script ne baisse pas davantage.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_emissifs_voyants.py
import unreal
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
VAL = 7.7 / 5.0
COUL = {"light_green": (0.0, VAL, 0.0), "light_red": (VAL, 0.0, 0.0)}
for a in EAL.list_assets("/Game/EON/Unity/import/fbx", True, False):
    p = a.split(".")[0]
    n = p.rsplit("/", 1)[-1].lower()
    if n not in COUL:
        continue
    mi = unreal.load_asset(p)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        continue
    MEL.set_material_instance_vector_parameter_value(mi, "EmissiveColor", unreal.LinearColor(*COUL[n], 1.0))
    MEL.update_material_instance(mi)
    EAL.save_asset(p, only_if_is_dirty=False)
    unreal.log("EON_VOYANT %s -> %.2f" % (p, VAL))
