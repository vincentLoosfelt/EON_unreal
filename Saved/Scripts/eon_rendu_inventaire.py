# EON — inventaire du rendu (lecture seule, rien n'est enregistré) : matériaux maîtres, instances et usages,
# profils toon, volumes de post-process et lumières de L_HUB / L_Etage.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_rendu_inventaire.py
import collections, unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_rendu_inventaire.txt"
L = []
ar = unreal.AssetRegistryHelpers.get_asset_registry()
MEL = unreal.MaterialEditingLibrary
opt = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)


def cls(ad):
    return str(ad.asset_class_path.asset_name)


mats, mis = [], []
for ad in ar.get_assets_by_path("/Game", recursive=True):
    c = cls(ad)
    if c == "Material":
        mats.append(ad)
    elif c == "MaterialInstanceConstant":
        mis.append(ad)

# Instance -> maître
child = collections.defaultdict(list)
for ad in mis:
    mi = ad.get_asset()
    base = mi.get_base_material()
    child[base.get_path_name().split(".")[0] if base else "-"].append(str(ad.package_name))


def users(pkg):
    r = ar.get_referencers(pkg, opt) or []
    return [str(x) for x in r]


def kind(pkg):
    a = ar.get_assets_by_package_name(pkg)
    return cls(a[0]) if a else "?"


L.append("=== MATÉRIAUX (%d) / INSTANCES (%d)" % (len(mats), len(mis)))
for ad in sorted(mats, key=lambda a: str(a.package_name)):
    pkg = str(ad.package_name)
    if not pkg.startswith("/Game/EON"):
        continue
    m = ad.get_asset()
    try:
        dom = m.get_editor_property("material_domain")
        blend = m.get_editor_property("blend_mode")
        sm = m.get_editor_property("shading_model")
        two = m.get_editor_property("two_sided")
    except Exception as e:
        dom = blend = sm = two = e
    root = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_FRONT_MATERIAL) if hasattr(unreal.MaterialProperty, "MP_FRONT_MATERIAL") else None
    tp = []
    try:
        for e in [x for x in (MEL.get_inputs_for_material_expression(m, root) if root else [])]:
            pass
    except Exception:
        pass
    params = "T[%s] V[%s] S[%s]" % (", ".join(str(n) for n in MEL.get_texture_parameter_names(m)),
                                     ", ".join(str(n) for n in MEL.get_vector_parameter_names(m)),
                                     ", ".join(str(n) for n in MEL.get_scalar_parameter_names(m)))
    L.append("\n## %s  dom=%s blend=%s sm=%s 2faces=%s racine=%s" % (pkg, str(dom).split(".")[-1], str(blend).split(".")[-1],
                                                                  str(sm).split(".")[-1], two, root.get_class().get_name() if root else "-"))
    L.append("   " + params)
    insts = child.get(pkg, [])
    L.append("   instances (%d) : %s" % (len(insts), ", ".join(p.split("/")[-1] for p in sorted(insts))))
    # Qui utilise le maître ou ses instances : meshes, BP, cartes
    use = collections.Counter(); exemples = collections.defaultdict(set)
    for p in [pkg] + insts:
        for u in users(p):
            if u in insts or u == pkg:
                continue
            k = kind(u)
            use[k] += 1
            exemples[k].add(u.split("/")[-1])
    for k, n in use.most_common():
        ex = sorted(exemples[k])
        L.append("   utilisé par %s ×%d : %s%s" % (k, len(ex), ", ".join(ex[:14]), " …" if len(ex) > 14 else ""))

L.append("\n=== PROFILS TOON")
for ad in ar.get_assets_by_path("/Game/EON", recursive=True):
    if "Toon" in cls(ad) and "Profile" in cls(ad):
        a = ad.get_asset()
        props = []
        for nm in dir(a):
            pass
        try:
            s = a.get_editor_property("settings")
            props.append(str(s))
        except Exception as e:
            props.append("settings ? %s" % e)
        L.append("%s : %s ; référencé par %s" % (ad.package_name, " ".join(props)[:600], [u.split("/")[-1] for u in users(str(ad.package_name))]))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
KEYS = ("auto_exposure_method", "auto_exposure_bias", "auto_exposure_apply_physical_camera_exposure", "bloom_intensity", "bloom_threshold",
        "scene_fringe_intensity", "tone_curve_amount", "expand_gamut", "blue_correction", "vignette_intensity", "film_grain_intensity",
        "dynamic_global_illumination_method", "reflection_method", "lumen_scene_lighting_quality", "lumen_final_gather_quality",
        "ambient_occlusion_intensity", "color_saturation", "color_contrast", "white_temp", "local_exposure_highlight_contrast_scale",
        "local_exposure_shadow_contrast_scale", "motion_blur_amount", "sharpen")
for carte in ("/Game/EON/Maps/L_HUB", "/Game/EON/Maps/L_Etage"):
    les.load_level(carte)
    L.append("\n=== CARTE %s" % carte)
    cnt = collections.Counter()
    for a in eas.get_all_level_actors():
        cnt[a.get_class().get_name()] += 1
        if isinstance(a, unreal.PostProcessVolume):
            s = a.get_editor_property("settings")
            ov = []
            for k in KEYS:
                try:
                    if s.get_editor_property("override_" + k):
                        ov.append("%s=%s" % (k, str(s.get_editor_property(k)).split(".")[-1]))
                except Exception:
                    pass
            bl = [(b.get_editor_property("object").get_name() if b.get_editor_property("object") else None, b.get_editor_property("weight"))
                  for b in s.get_editor_property("weighted_blendables").get_editor_property("array")]
            L.append("PPV %s unbound=%s actif=%s prio=%s\n   surchargés : %s\n   blendables : %s" % (a.get_actor_label(), a.get_editor_property("unbound"),
                     a.get_editor_property("enabled"), a.get_editor_property("priority"), ", ".join(ov), bl))
        elif isinstance(a, (unreal.DirectionalLight, unreal.SkyLight, unreal.PointLight, unreal.SpotLight, unreal.RectLight, unreal.ExponentialHeightFog)):
            comps = a.get_components_by_class(unreal.LightComponentBase) or a.get_components_by_class(unreal.ExponentialHeightFogComponent)
            for c in comps:
                info = []
                for k in ("intensity", "light_color", "cast_shadows", "mobility", "real_time_capture", "source_type", "fog_density", "volumetric_fog"):
                    try:
                        info.append("%s=%s" % (k, str(c.get_editor_property(k)).split(".")[-1]))
                    except Exception:
                        pass
                if cnt[a.get_class().get_name()] <= 4:
                    L.append("%s %s rot=%s : %s" % (a.get_class().get_name(), a.get_actor_label(), a.get_actor_rotation(), ", ".join(info)))
    L.append("acteurs : " + ", ".join("%s×%d" % kv for kv in cnt.most_common(18)))

open(OUT, "w", encoding="utf-8").write("\n".join(L))
unreal.log("EON_RINV fini %d lignes" % len(L))
