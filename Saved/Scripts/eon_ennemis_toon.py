# EON — matériau des ennemis sans normal map (Vincent, 23/11) : M_EON_ToonEnnemi = copie opaque, une face,
# de M_EON_ToonMasked (celui de Jira : Substrate Toon, BaseTex × Tint, sans entrée Normal). Les MI_<Nom> y passent,
# profil toon TP_EON_Anime gardé.
import unreal, os
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
L = []
SRC, DST = "/Game/EON/Materials/M_EON_ToonMasked", "/Game/EON/Materials/M_EON_ToonEnnemi"
if not EAL.does_asset_exist(DST):
    EAL.duplicate_asset(SRC, DST)
m = unreal.load_asset(DST)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
m.set_editor_property("two_sided", False)
m.set_editor_property("used_with_skeletal_mesh", True)
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
L.append("%s : opaque, une face ; textures %s" % (DST, MEL.get_texture_parameter_names(m)))
tp = unreal.load_asset("/Game/EON/Materials/TP_EON_Anime")
for nom in ("Nuke", "Onibi", "Kama", "Ikiryo", "Ubume"):
    for p in EAL.list_assets("/Game/EON/Ennemis/" + nom, recursive=False):
        mi = unreal.load_asset(p)
        if isinstance(mi, unreal.MaterialInstanceConstant):
            tex = MEL.get_material_instance_texture_parameter_value(mi, "BaseTex")
            mi.set_editor_property("parent", m)
            MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", tex)
            unreal.EONStatics.set_instance_toon_profile(mi, tp)
            EAL.save_loaded_asset(mi)
            L.append("%s -> M_EON_ToonEnnemi, BaseTex %s" % (mi.get_name(), tex.get_name() if tex else None))
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_ennemis_toon.txt"), "w", encoding="utf-8").write("\n".join(L))
