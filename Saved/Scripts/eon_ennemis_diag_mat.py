# EON — diagnostic du matériau des ennemis : entrées de M_UnityLit, paramètres des MI_<Nom>, profil toon,
# volumes de post-process des cartes. Sortie : Saved/Logs/eon_ennemis_diag_mat.txt
import unreal, os
MEL = unreal.MaterialEditingLibrary
L = []
m = unreal.load_asset("/Game/EON/Unity/Materials/M_UnityLit")
L.append("M_UnityLit shading=%s blend=%s" % (m.get_editor_property("shading_model"), m.get_editor_property("blend_mode")))
for p in ("MP_BASE_COLOR", "MP_NORMAL", "MP_EMISSIVE_COLOR", "MP_ROUGHNESS", "MP_METALLIC", "MP_SPECULAR", "MP_AMBIENT_OCCLUSION"):
    n = MEL.get_material_property_input_node(m, getattr(unreal.MaterialProperty, p))
    L.append("  %s <- %s" % (p, n.get_class().get_name() + " " + (n.get_editor_property("parameter_name") if hasattr(n, "parameter_name") else "") if n else "-"))
for name in MEL.get_texture_parameter_names(m):
    L.append("  tex param %s = %s" % (name, MEL.get_material_default_texture_parameter_value(m, name)))
for name in MEL.get_scalar_parameter_names(m):
    L.append("  scalar %s = %s" % (name, MEL.get_material_default_scalar_parameter_value(m, name)))
for nom in ("Nuke", "Onibi", "Kama", "Ikiryo", "Ubume"):
    for mi in unreal.EditorAssetLibrary.list_assets("/Game/EON/Ennemis/" + nom, recursive=False):
        a = unreal.load_asset(mi)
        if isinstance(a, unreal.MaterialInstanceConstant):
            L.append("%s : tex %s ; scal %s ; toon %s" % (a.get_name(),
                [(str(t.parameter_info.name), t.parameter_value.get_name() if t.parameter_value else None) for t in a.get_editor_property("texture_parameter_values")],
                [(str(s.parameter_info.name), s.parameter_value) for s in a.get_editor_property("scalar_parameter_values")],
                a.get_editor_property("toon_profile_override") if hasattr(a, "toon_profile_override") else "?"))
    sk = unreal.load_asset("/Game/EON/Ennemis/%s/SK_%s" % (nom, nom))
    if sk:
        imp = sk.get_editor_property("asset_import_data")
        L.append("SK_%s import=%s" % (nom, imp.get_class().get_name() if imp else None))
tp = unreal.load_asset("/Game/EON/Materials/TP_EON_Anime")
if tp:
    s = tp.get_editor_property("settings")
    L.append("TP_EON_Anime : %s" % [(p, s.get_editor_property(p)) for p in ("diffuse_indirect_scale", "specular_indirect_scale", "diffuse_ramp_include_shadow") if hasattr(s, p)])
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for carte in ("/Game/EON/Maps/L_Etage", "/Game/EON/Maps/L_HUB"):
    les.load_level(carte)
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.PostProcessVolume):
            st = a.get_editor_property("settings")
            on = [p for p in dir(st) if p.startswith("override_") and st.get_editor_property(p)]
            L.append("%s %s unbound=%s actif=%s : %s" % (carte, a.get_name(), a.get_editor_property("unbound"), a.get_editor_property("enabled"),
                ", ".join("%s=%s" % (p[9:], st.get_editor_property(p[9:])) for p in on if hasattr(st, p[9:]))))
            L.append("   blendables %s" % [(b.object.get_name() if b.object else None, b.weight) for b in st.get_editor_property("weighted_blendables").get_editor_property("array")])
open(os.path.join(unreal.Paths.project_dir(), "Saved", "Logs", "eon_ennemis_diag_mat.txt"), "w", encoding="utf-8").write("\n".join(L))
