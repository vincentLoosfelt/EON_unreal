# EON — réglages des textures d'un ennemi (EON_SEUL=<Nom>) : taille source, max, groupe LOD, compression, mips, streaming.
import unreal, os
nom = os.environ.get("EON_SEUL", "Ikiryo")
L = []
for p in unreal.EditorAssetLibrary.list_assets("/Game/EON/Ennemis/%s/Textures" % nom, recursive=False):
    t = unreal.load_asset(p)
    if not isinstance(t, unreal.Texture2D): continue
    g = lambda k: t.get_editor_property(k)
    L.append("%s : source %dx%d, max %s, LOD %s, bias %s, compression %s, mips %s, filtre %s, never_stream %s, srgb %s" % (
        t.get_name(), t.blueprint_get_size_x(), t.blueprint_get_size_y(), g("max_texture_size"), g("lod_group"), g("lod_bias"),
        g("compression_settings"), g("mip_gen_settings"), g("filter"), g("never_stream"), g("srgb")))
mi = unreal.load_asset("/Game/EON/Ennemis/%s/MI_%s" % (nom, nom))
if mi:
    L.append("MI parent %s" % mi.get_editor_property("parent").get_name())
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_diag_texture.txt"), "w", encoding="utf-8").write("\n".join(L))
