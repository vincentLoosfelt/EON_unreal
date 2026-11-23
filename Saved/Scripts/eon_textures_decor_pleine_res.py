# EON — textures du décor floues/pixelisées (Vincent, 23/11). Test A/B (eon_mips_test.py) : avec le streaming, le
# moteur garde un mip trop bas sur les rambardes (MI_BG_Barrier01 répète la texture, le streamer sous-estime la taille
# voulue) ; streaming coupé = net. Correctif : textures du DÉCOR hors streaming (never_stream), toujours à pleine
# résolution, mips générés normalement (filtrage trilinéaire/aniso par défaut). Personnages, ennemis et UI non touchés.
# Journal : Saved/Logs/eon_textures_decor_pleine_res.txt
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_textures_decor_pleine_res.py
import unreal
EAL = unreal.EditorAssetLibrary
AR = unreal.AssetRegistryHelpers.get_asset_registry()
DOSSIERS = ["/Game/EON/Unity/import/textures"]          # racine (Mur_*, …), enviro/*, Shaders/
EXCLUS = ("/chara", "/weapons", "/FX", "/xeno")          # personnages, armes, effets, ennemis : celluloïds, hors sujet
LOG = []
total = 0.0
for d in DOSSIERS:
    f = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "Texture2D")], package_paths=[d], recursive_paths=True)
    for ad in AR.get_assets(f):
        pkg = str(ad.package_name)
        if any(x.lower() in pkg.lower() for x in EXCLUS):
            continue
        t = ad.get_asset()
        avant = (t.get_editor_property("never_stream"), str(t.get_editor_property("mip_gen_settings")), str(t.get_editor_property("filter")))
        t.set_editor_property("never_stream", True)
        if "NO_MIPMAPS" in avant[1] or "LEAVE_EXISTING" in avant[1]:
            t.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
        if "NEAREST" in avant[2]:
            t.set_editor_property("filter", unreal.TextureFilter.TF_DEFAULT)
        w, h = t.blueprint_get_size_x(), t.blueprint_get_size_y()
        comp = str(t.get_editor_property("compression_settings"))
        bpp = 0.5 if ("DEFAULT" in comp and "TC_DEFAULT" in comp) else 1.0   # BC1 ~0,5 o/px, BC3/BC5 ~1 o/px (approx.)
        mo = w * h * bpp * 1.33 / 1048576.0
        total += mo
        ok = EAL.save_loaded_asset(t, False)
        LOG.append("%s | %dx%d | avant never_stream=%s mips=%s filtre=%s | ~%.0f Mo | enregistré=%s" % (pkg, w, h, avant[0], avant[1], avant[2], mo, ok))
LOG.append("TOTAL %d textures, ~%.0f Mo de mémoire vidéo en pleine résolution" % (len(LOG), total))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_textures_decor_pleine_res.txt", "w", encoding="utf-8").write("\n".join(LOG))
unreal.log("EON_TEXFIX " + LOG[-1])
