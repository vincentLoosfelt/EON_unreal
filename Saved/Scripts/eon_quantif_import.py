# EON — import des doublons base color en aplats (eon_quantif_couleurs.py, Vincent 26/11) :
#   T_<X>_BaseColor_Quant à côté de chaque T_<X>_BaseColor, et MI_<X>[_Partie]_Quant = copie de la MI d'origine
#   (même parent, même profil toon, mêmes réglages) avec BaseTex sur le doublon. Les originaux ne sont pas touchés.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os, glob
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.path.join(unreal.Paths.project_dir(), "SourceAssets", "Ennemis")
L = []
for fn in sorted(glob.glob(os.path.join(SRC, "*", "*_BaseColor_Quant.png"))):
    nom = os.path.basename(os.path.dirname(fn))
    dest = "/Game/EON/Ennemis/%s/Textures" % nom
    tn = os.path.splitext(os.path.basename(fn))[0]
    t = unreal.AssetImportTask()
    t.filename = fn; t.destination_path = dest; t.destination_name = tn
    t.automated = True; t.replace_existing = True; t.save = False
    AT.import_asset_tasks([t])
    tex = unreal.load_asset(dest + "/" + tn)
    if not tex:
        L.append("ECHEC import " + tn); continue
    orig = unreal.load_asset(dest + "/" + tn[:-6])          # mêmes réglages que l'original
    if orig:
        for p in ("srgb", "compression_settings", "lod_group", "address_x", "address_y"):
            tex.set_editor_property(p, orig.get_editor_property(p))
    L.append("%s : %s" % (tn, EAL.save_loaded_asset(tex)))
    # MI d'origine : celle dont BaseTex pointe sur la texture d'origine
    for mp in EAL.list_assets("/Game/EON/Ennemis/%s" % nom, recursive=False):
        mp = mp.split(".")[0]
        if not os.path.basename(mp).startswith("MI_") or mp.endswith("_Quant"):
            continue
        mi = unreal.load_asset(mp)
        bt = MEL.get_material_instance_texture_parameter_value(mi, "BaseTex")
        if not bt or bt.get_name() != tn[:-6]:
            continue
        dp = mp + "_Quant"
        if EAL.does_asset_exist(dp):
            EAL.delete_asset(dp)
        d = EAL.duplicate_asset(mp, dp)
        MEL.set_material_instance_texture_parameter_value(d, "BaseTex", tex)
        L.append("  %s -> %s : %s" % (os.path.basename(mp), os.path.basename(dp), EAL.save_loaded_asset(d)))
open(os.path.join(unreal.Paths.project_saved_dir(), "quantif_import.txt"), "w", encoding="utf-8").write("\n".join(L))
