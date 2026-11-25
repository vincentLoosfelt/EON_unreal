# EON — police Bangers (OFL, SourceAssets/Fonts/Bangers) pour les chiffres de dégâts manga (Vincent, 25/11).
#   /Game/EON/UI/Fonts/FF_Bangers : la face (TTF importée). La police « runtime » est construite dessus par AEONHUD
#   (la police composite n'est pas réglable depuis Python).
# Lancer dans l'éditeur réel (la face de police a besoin de Slate, le commandlet plante) :
#   UnrealEditor <EON.uproject complet> -ExecutePythonScript=<ce fichier>  (l'éditeur se ferme à la fin)
import unreal, os
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/EON/UI/Fonts"
TTF = os.path.join(unreal.Paths.project_dir(), "SourceAssets", "Fonts", "Bangers", "Bangers-Regular.ttf")
# Import « automatisé » : sans lui, l'usine des polices ouvre un dialogue et le commandlet plante (25/11).
def faire():
    d = unreal.AutomatedAssetImportData()
    d.set_editor_property("filenames", [TTF])
    d.set_editor_property("destination_path", DST)
    d.set_editor_property("replace_existing", True)
    d.set_editor_property("factory_name", "FontFileImportFactory")
    AT.import_assets_automated(d)
    if not EAL.does_asset_exist(DST + "/FF_Bangers"):
        old = DST + "/Bangers-Regular"
        if EAL.does_asset_exist(old):
            EAL.rename_asset(old, DST + "/FF_Bangers")
    face = unreal.load_asset(DST + "/FF_Bangers")
    unreal.log("EON_POLICE face %s %s" % (face, face.get_class().get_name() if face else "-"))
    if face and not isinstance(face, unreal.FontFace):
        # l'import d'un TTF peut créer une UFont ; on y prend la face qu'elle contient
        unreal.log("EON_POLICE import = %s" % face.get_class().get_name())
    unreal.log("EON_POLICE sauve face %s" % EAL.save_loaded_asset(face))

try:
    faire()
except Exception as ex:
    unreal.log_error("EON_POLICE echec %s" % ex)
finally:
    unreal.SystemLibrary.quit_editor()
