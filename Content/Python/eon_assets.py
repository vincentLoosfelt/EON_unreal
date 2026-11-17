# EON — bibliothèque d'assets de l'éditeur de salles.
#
# Chaque asset posable est un Blueprint d'acteur /Game/EON/RoomAssets/<chemin>/BPA_<nom> : on peut y ajouter scripts,
# colliders, lumières... sans toucher aux salles.
#   - Assets FBX : pour chaque FBX de SourceAssets/FBX (copie des FBX d'Unity), le Blueprint est créé s'il n'existe pas
#     (un StaticMesh, pivot recentré : origine = centre de l'empreinte, au sol) ; un Blueprint existant n'est jamais modifié.
#   - Assets dynamiques : Blueprints faits à la main dans /Game/EON/RoomAssets/Dynamiques (scripts, lumières, colliders...),
#     par exemple les lumières (AEONLampe). Leurs fichiers vont dans SourceAssets/Dynamiques/<chemin>.
#   <nom>.eonasset (JSON) et <nom>.eonapercu (géométrie avec UV, matériaux, lumières) sont mesurés dans le Blueprint.
#   Les textures de base sont exportées en PNG dans SourceAssets/Textures ; SourceAssets/Salle.eontheme donne le sol et
#   les murs des salles. L'éditeur de salles ouvre le dossier SourceAssets entier.
#
#   import eon_assets
#   eon_assets.generate_all()
#
# .eonasset :
#   id          chemin relatif sans extension, unique : "moniteur/Computer_hitech_V1", "Dynamiques/Lumieres/Neon_mural"
#   nom, categorie, tags            réglés par l'artiste, gardés d'une génération à l'autre
#   collision   "solide" (les tuiles sous l'objet bloquent comme une caisse) ou "aucune" ; gardé aussi
#   rotations   lacets permis en degrés ; gardé aussi
#   pose        "sol" (tuiles de sol) ou "mur" (tuile de mur, face tournée vers le sol) ; gardé aussi
#   dynamique   true pour un Blueprint fait à la main
#   lumiere     réglages par défaut d'une lumière (mode fixe/clignote/vacille, couleur, rayon_px, intensite) ; modifiables
#               pour chaque lumière posée dans l'éditeur
#   fbx         FBX source (à côté du fichier) ; mesh : le StaticMesh importé (assets FBX)
#   unreal      le Blueprint posé dans les salles
#   empreinte   [largeur, profondeur] en tuiles de 1 m, à rotation 0
#   taille_cm   [x, y, z] boîte englobante ; centre_cm [x, y] son centre et sol_cm son bas, dans le repère du Blueprint
#   apercu      fichier .eonapercu
import json
import math
import os

import unreal

FORMAT = "eonasset"
VERSION = 1
MESH_ROOT = "/Game/EON/Unity/import/fbx"
BP_ROOT = "/Game/EON/RoomAssets"
DYN_ROOT = BP_ROOT + "/Dynamiques"
MAX_TRIS = 12000
KEEP = ("nom", "categorie", "tags", "collision", "rotations", "pose", "lumiere")

CATEGORIES = {
    "": "Cuves", "Barriere": "Barrières", "boss": "Boss", "enviro": "Environnement", "enviro/wall": "Murs",
    "highttech/Assets_Hitech_big": "High-tech grands", "highttech/Assets_Hitech_medium": "High-tech moyens",
    "highttech/Assets_Hitech_small": "High-tech petits", "lootable": "Ramassables", "moniteur": "Moniteurs", "weapons": "Armes",
    "Dynamiques/Lumieres": "Lumières",
}
# Matériaux des salles du jeu (AEONTileRoom) pour l'aperçu de l'éditeur.
THEME = {"sol": "/Game/EON/Materials/MI_Sol_Eclaire", "mur": "/Game/EON/Materials/BG/MI_BG_walll_painted",
         "caisse": "/Game/EON/Materials/BG/MI_BG_Barrier01", "cuve": "/Game/EON/Materials/BG/MI_BG_pipe_1",
         "porte": "/Game/EON/Materials/BG/MI_BG_pipe", "vide": "/Game/EON/Materials/BG/MI_BG_Grille01"}


def source_dir():
    return os.path.normpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceAssets"))


def assets_dir():
    return os.path.join(source_dir(), "FBX")


def dyn_dir():
    return os.path.join(source_dir(), "Dynamiques")


def textures_dir():
    return os.path.join(source_dir(), "Textures")


def _split(rel_no_ext):
    d, n = os.path.split(rel_no_ext.replace("\\", "/"))
    return d, n.replace(" ", "_")


def mesh_path(rel_no_ext):
    d, n = _split(rel_no_ext)
    return MESH_ROOT + "/" + (d + "/" if d else "") + n


def bp_path(rel_no_ext):
    d, n = _split(rel_no_ext)
    return BP_ROOT + "/" + (d + "/" if d else "") + "BPA_" + n


# ---------------------------------------------------------------- Blueprint d'asset
def _sub():
    return unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def _templates(bp):
    out = []
    for h in _sub().k2_gather_subobject_data_for_blueprint(bp):
        o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        if o is not None:
            out.append((h, o))
    return out


def ensure_bp(rel_no_ext, sm):
    """Crée le Blueprint d'asset s'il n'existe pas. Renvoie (blueprint, créé)."""
    path = bp_path(rel_no_ext)
    eal = unreal.EditorAssetLibrary
    if eal.does_asset_exist(path):
        return unreal.load_asset(path), False
    folder, name = path.rsplit("/", 1)
    fac = unreal.BlueprintFactory()
    fac.set_editor_property("parent_class", unreal.Actor)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, fac)
    if not bp:
        raise RuntimeError("création impossible : " + path)
    sub = _sub()
    root = sub.k2_gather_subobject_data_for_blueprint(bp)[0]
    h, fail = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.StaticMeshComponent, blueprint_context=bp))
    if not fail.is_empty():
        raise RuntimeError("mesh de %s : %s" % (path, fail))
    sub.rename_subobject(h, unreal.Text("Mesh"))
    comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    comp.set_editor_property("static_mesh", sm)
    b = sm.get_bounding_box()
    # Origine du Blueprint = centre de l'empreinte, au sol.
    comp.set_editor_property("relative_location", unreal.Vector(-(b.min.x + b.max.x) * 0.5, -(b.min.y + b.max.y) * 0.5, -b.min.z))
    # La salle bloque par ses tuiles (O) ; le mesh ne fait que l'image. Un collider ajouté dans le Blueprint reste possible.
    comp.set_collision_profile_name("NoCollision")
    comp.set_editor_property("can_ever_affect_navigation", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not eal.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError("enregistrement de %s impossible (jeu EON lancé ?)" % path)
    return bp, True


# ---------------------------------------------------------------- matériaux : couleur, texture, émission, opacité
_NAMED = [("glass", "#9fd8e6"), ("verre", "#9fd8e6"), ("screen", "#4fe0ff"), ("green", "#3fbf6a"), ("red", "#d84a3a"),
          ("yellow", "#e8c23a"), ("blue", "#3a78d8"), ("white", "#e6e8ea"), ("very_light", "#cfd3d6"), ("light_grey", "#aab0b5"),
          ("dark", "#3b4045"), ("black", "#1c1e21"), ("grey", "#7d848a"), ("light", "#f2e6a0"), ("metal", "#8a9096"), ("wall", "#5d7a74")]
MEL = unreal.MaterialEditingLibrary
TEX_PARAMS = ("BaseTex", "DiffuseColorMap", "BaseColorTexture", "Albedo", "MainTex")
COL_PARAMS = ("BC1", "Tint", "DiffuseColor", "Color", "Couleur", "BaseColor", "Base Color")


def _srgb(v):
    v = max(0.0, v)
    return 12.92 * v if v <= 0.0031308 else 1.055 * (v ** (1.0 / 2.4)) - 0.055


def _hex(c, linear=True):
    f = _srgb if linear else (lambda v: max(0.0, v))
    return "#%02x%02x%02x" % tuple(max(0, min(255, int(round(f(v) * 255)))) for v in (c.r, c.g, c.b))


_exported = {}


def export_texture(tex):
    """Exporte la texture en PNG dans SourceAssets/Textures (une fois par génération) ; renvoie le nom du fichier."""
    if not tex or tex.get_path_name().startswith("/Engine/"):
        return None
    key = tex.get_path_name()
    if key in _exported:
        return _exported[key]
    name = tex.get_name() + ".png"
    out = os.path.join(textures_dir(), name)
    os.makedirs(textures_dir(), exist_ok=True)
    task = unreal.AssetExportTask()
    task.set_editor_property("object", tex)
    task.set_editor_property("filename", out)
    task.set_editor_property("exporter", unreal.TextureExporterPNG())
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    ok = unreal.Exporter.run_asset_export_task(task) and os.path.exists(out)
    if not ok:
        unreal.log_warning("EON assets : export PNG impossible pour " + key)
    _exported[key] = name if ok else None
    return _exported[key]


def _names(fn, mat):
    try:
        return [str(n) for n in fn(mat)]
    except Exception:
        return []


def mat_info(mat):
    """Couleur de base (sRGB, multipliée par la texture), texture de base (PNG), émission et opacité, pour l'aperçu."""
    info = {"couleur": "#8a9096"}
    if not mat:
        return info
    found = False
    if isinstance(mat, unreal.MaterialInstance):
        vn = _names(MEL.get_vector_parameter_names, mat)
        tn = _names(MEL.get_texture_parameter_names, mat)
        sn = _names(MEL.get_scalar_parameter_names, mat)
        for p in COL_PARAMS:
            if p in vn:
                info["couleur"] = _hex(MEL.get_material_instance_vector_parameter_value(mat, p))
                found = True
                break
        for p in TEX_PARAMS:
            if p in tn:
                t = export_texture(MEL.get_material_instance_texture_parameter_value(mat, p))
                if t:
                    info["texture"] = t
                break
        em = None
        for p in ("EmissiveColor", "EmisAdd"):
            if p in vn:
                em = MEL.get_material_instance_vector_parameter_value(mat, p)
                break
        if em is not None and em.r + em.g + em.b > 0.01:
            m = max(em.r, em.g, em.b)
            info["emission"] = _hex(unreal.LinearColor(em.r / m, em.g / m, em.b / m, 1.0))
        if "EmisTex" in tn:
            et = export_texture(MEL.get_material_instance_texture_parameter_value(mat, "EmisTex"))
            if et:
                info["texture_emission"] = et
        for p in ("Opacity", "Opacite"):
            if p in sn:
                o = MEL.get_material_instance_scalar_parameter_value(mat, p)
                if 0.0 <= o < 0.99:
                    info["opacite"] = round(o, 2)
    else:
        try:
            for t in MEL.get_used_textures(mat):
                n = t.get_name().lower()
                if "normal" in n or "_nm" in n or "nml" in n:
                    continue
                e = export_texture(t)
                if e:
                    info["texture"] = e
                    info["couleur"] = "#ffffff"
                    found = True
                    break
        except Exception:
            pass
    if not found:
        n = mat.get_name().lower()
        for k, c in _NAMED:
            if k in n:
                info["couleur"] = c
                break
        if "texture" in info:
            info["couleur"] = "#ffffff"
    return info


# ---------------------------------------------------------------- géométrie (tous les StaticMesh du Blueprint)
def _world_of(comp, all_comps):
    """Transformation du composant dans le repère du Blueprint (parents compris)."""
    xf = comp.get_editor_property("relative_location"), comp.get_editor_property("relative_rotation"), comp.get_editor_property("relative_scale3d")
    t = unreal.Transform(xf[0], xf[1], xf[2])
    parent = comp.get_attach_parent() if hasattr(comp, "get_attach_parent") else None
    while parent is not None and parent in all_comps:
        pt = unreal.Transform(parent.get_editor_property("relative_location"), parent.get_editor_property("relative_rotation"), parent.get_editor_property("relative_scale3d"))
        t = t.multiply(pt)
        parent = parent.get_attach_parent() if hasattr(parent, "get_attach_parent") else None
    return t


def _components(bp):
    """Composants de scène du Blueprint, y compris ceux hérités d'une classe C++ (AEONLampe)."""
    return [o for _, o in _templates(bp) if isinstance(o, unreal.SceneComponent)]


def bp_geometry(bp):
    """Pièces {v (cm, repère du Blueprint), uv, t, mat} de tous les StaticMesh du Blueprint."""
    comps = _components(bp)
    parts = []
    for c in comps:
        if not isinstance(c, unreal.StaticMeshComponent):
            continue
        sm = c.get_editor_property("static_mesh")
        if not sm:
            continue
        t = _world_of(c, comps)
        neon = "neon" in [str(x) for x in c.get_editor_property("component_tags")]
        for s in range(sm.get_num_sections(0)):
            res = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(sm, 0, s)
            v = []
            for p in res[0]:
                q = t.transform_location(p)
                v.append((q.x, q.y, q.z))
            uvs = list(res[3]) if len(res) > 3 else []
            uv = [(u.x, u.y) for u in uvs] if len(uvs) == len(v) else [(0.0, 0.0)] * len(v)
            mat = c.get_material(s) if hasattr(c, "get_material") else None
            info = mat_info(mat or sm.get_material(s))
            if neon:
                info["neon"] = True
            parts.append({"v": v, "uv": uv, "t": list(res[1]), "mat": info})
    return parts


def bp_lights(bp):
    """Lumière d'un Blueprint AEONLampe : position (cm, repère du Blueprint) et réglages par défaut."""
    cdo = unreal.get_default_object(bp.generated_class())
    if not isinstance(cdo, unreal.EONLampe):
        return None
    comps = _components(bp)
    pos = (0.0, 0.0, 120.0)
    for c in comps:
        if isinstance(c, unreal.PointLightComponent):
            p = _world_of(c, comps).translation
            pos = (p.x, p.y, p.z)
            break
    mode = str(cdo.get_editor_property("mode")).split(".")[-1].lower()
    return {"pos_cm": [round(x, 1) for x in pos], "mode": mode if mode in ("fixe", "clignote", "vacille") else "fixe",
            "couleur": _hex(cdo.get_editor_property("couleur")), "rayon_px": round(cdo.get_editor_property("rayon_px"), 1),
            "intensite": round(cdo.get_editor_property("intensite"), 2)}


def _simplify(part, cell):
    idx, out_v, out_uv, remap = {}, [], [], []
    for v, uv in zip(part["v"], part["uv"]):
        k = (int(math.floor(v[0] / cell + 0.5)), int(math.floor(v[1] / cell + 0.5)), int(math.floor(v[2] / cell + 0.5)))
        if k not in idx:
            idx[k] = len(out_v)
            out_v.append((k[0] * cell, k[1] * cell, k[2] * cell))
            out_uv.append(uv)
        remap.append(idx[k])
    out_t, seen = [], set()
    tris = part["t"]
    for i in range(0, len(tris), 3):
        a, b, c = remap[tris[i]], remap[tris[i + 1]], remap[tris[i + 2]]
        if a == b or b == c or a == c:
            continue
        key = tuple(sorted((a, b, c)))
        if key in seen:
            continue
        seen.add(key)
        out_t += [a, b, c]
    return dict(part, v=out_v, uv=out_uv, t=out_t)


def preview(parts):
    """Pièces de l'aperçu : au plus MAX_TRIS triangles (simplification par grille au-delà)."""
    total = sum(len(p["t"]) // 3 for p in parts)
    allv = [q for p in parts for q in p["v"]]
    if not allv:
        return []
    ext = max(max(q[i] for q in allv) - min(q[i] for q in allv) for i in range(3)) or 1.0
    cell = 0.0
    while total > MAX_TRIS and cell < ext / 4:
        cell = cell * 1.5 if cell else ext / 400.0
        simp = [_simplify(p, cell) for p in parts]
        total = sum(len(p["t"]) // 3 for p in simp)
        if total <= MAX_TRIS:
            parts = simp
    out = []
    for p in parts:
        if not p["t"]:
            continue
        d = dict(p["mat"])
        d["v"] = [round(x, 1) for q in p["v"] for x in q]
        d["uv"] = [round(x, 4) for q in p["uv"] for x in q]
        d["t"] = p["t"]
        out.append(d)
    return out


# ---------------------------------------------------------------- .eonasset
def _footprint(cm):
    # Tolérance de 15 cm : un objet de 1,1 m tient sur 1 tuile.
    return max(1, int(math.ceil(cm / 100.0 - 0.15)))


def asset_dict(rel_no_ext, bp, parts, sm=None, fbx=None, lum=None, old=None):
    allv = [q for p in parts for q in p["v"]]
    if not allv:
        allv = [(-50.0, -50.0, 0.0), (50.0, 50.0, 0.0)]
    mn = [min(p[i] for p in allv) for i in range(3)]
    mx = [max(p[i] for p in allv) for i in range(3)]
    sx, sy, sz = mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]
    folder, name = os.path.dirname(rel_no_ext), os.path.basename(rel_no_ext)
    d = {
        "format": FORMAT,
        "version": VERSION,
        "id": rel_no_ext,
        "nom": name.replace("_", " "),
        "categorie": CATEGORIES.get(folder, os.path.basename(folder) or "Divers"),
        "tags": [],
        "collision": "aucune" if lum else ("solide" if sz >= 40.0 and max(sx, sy) >= 30.0 else "aucune"),
        "rotations": [0, 90, 180, 270],
        "pose": "sol",
    }
    if fbx is None:
        d["dynamique"] = True
    if lum:
        d["lumiere"] = {k: lum[k] for k in ("mode", "couleur", "rayon_px", "intensite")}
    if fbx:
        d["fbx"] = fbx
        d["mesh"] = sm.get_path_name() if sm else ""
    d.update({
        "unreal": bp.generated_class().get_path_name(),
        "empreinte": [_footprint(sx), _footprint(sy)] if not lum else [1, 1],
        "taille_cm": [round(sx, 1), round(sy, 1), round(sz, 1)],
        "centre_cm": [round((mn[0] + mx[0]) * 0.5, 1), round((mn[1] + mx[1]) * 0.5, 1)] if not lum else [0.0, 0.0],
        "sol_cm": round(mn[2], 1) if not lum else 0.0,
        "apercu": name + ".eonapercu",
    })
    if old:
        for k in KEEP:
            if k in old:
                d[k] = old[k]
    return d


def _read_json(path):
    if not os.path.exists(path):
        return None
    try:
        with open(path, encoding="utf-8-sig") as fh:
            return json.load(fh)
    except Exception:
        return None


def _write_pair(base, d, parts, lum):
    with open(base + ".eonasset", "w", encoding="utf-8", newline="\n") as fh:
        fh.write(json.dumps(d, ensure_ascii=False, indent=2) + "\n")
    ap = {"format": "eonapercu", "version": 2, "pieces": preview(parts)}
    if lum:
        ap["lumieres"] = [{"pos_cm": lum["pos_cm"]}]
    with open(base + ".eonapercu", "w", encoding="utf-8", newline="\n") as fh:
        fh.write(json.dumps(ap, ensure_ascii=False, separators=(",", ":")) + "\n")


def generate_theme():
    """SourceAssets/Salle.eontheme : matériaux du sol, des murs, des caisses et des cuves des salles du jeu."""
    th = {"format": "eontheme", "version": 1, "hauteur_mur_cm": 220}
    for k, p in THEME.items():
        th[k] = mat_info(unreal.load_asset(p))
    with open(os.path.join(source_dir(), "Salle.eontheme"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(json.dumps(th, ensure_ascii=False, indent=2) + "\n")


def generate_dynamics():
    """Blueprints de /Game/EON/RoomAssets/Dynamiques -> SourceAssets/Dynamiques/<chemin>/<nom>.eonasset (+ aperçu)."""
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    done = []
    for ad in ar.get_assets_by_path(DYN_ROOT, recursive=True):
        if str(ad.asset_class_path.asset_name) != "Blueprint":
            continue
        bp = ad.get_asset()
        pkg = str(ad.package_name)
        rel_dir = os.path.dirname(pkg[len(BP_ROOT) + 1:])
        name = str(ad.asset_name)
        name = name[4:] if name.startswith("BPA_") else name
        rel_no_ext = rel_dir + "/" + name
        parts = bp_geometry(bp)
        lum = bp_lights(bp)
        base = os.path.join(source_dir(), rel_no_ext.replace("/", os.sep))
        os.makedirs(os.path.dirname(base), exist_ok=True)
        old = _read_json(base + ".eonasset")
        d = asset_dict(rel_no_ext, bp, parts, lum=lum, old=old)
        if old is None and name.lower().startswith("neon_mural"):
            d["pose"] = "mur"
        _write_pair(base, d, parts, lum)
        done.append(d["id"])
    return done


# ---------------------------------------------------------------- FBX modifié (Blender) -> réimport
# Empreintes (date + taille) des FBX au dernier passage : SourceAssets/FBX/eon_fbx_empreintes.json (pas dans les meshes,
# pour ne pas réenregistrer tous les .uasset).
def _stamps_path():
    return os.path.join(assets_dir(), "eon_fbx_empreintes.json")


_stamps = {}


def _fbx_stamp(path):
    st = os.stat(path)
    return "%d:%d" % (int(st.st_mtime), st.st_size)


def _fbx_options():
    o = unreal.FbxImportUI()
    o.import_mesh = True
    o.import_as_skeletal = False
    o.import_materials = False
    o.import_textures = False
    o.import_animations = False
    o.static_mesh_import_data.combine_meshes = True
    o.static_mesh_import_data.generate_lightmap_u_vs = True
    o.static_mesh_import_data.auto_generate_collision = True
    return o


def reimport_if_changed(fbx, sm, bp):
    """Réimporte le StaticMesh si le FBX de SourceAssets a changé depuis le dernier passage (date + taille).
    Les matériaux des slots sont remis tels quels ; le composant « Mesh » du Blueprint d'asset est recentré
    (origine = centre de l'empreinte, au sol) ; les autres composants du Blueprint ne bougent pas.
    Premier passage : on note seulement l'empreinte du FBX."""
    eal = unreal.EditorAssetLibrary
    key = os.path.relpath(fbx, assets_dir()).replace("\\", "/")
    stamp = _fbx_stamp(fbx)
    old = _stamps.get(key)
    _stamps[key] = stamp
    if not old or old == stamp:
        return False
    path = sm.get_path_name().split(".")[0]
    mats = [(sl.get_editor_property("material_slot_name"), sl.get_editor_property("material_interface")) for sl in sm.get_editor_property("static_materials")]
    t = unreal.AssetImportTask()
    t.filename = fbx
    t.destination_path = path.rsplit("/", 1)[0]
    t.destination_name = path.rsplit("/", 1)[1]
    t.automated = True
    t.replace_existing = True
    t.save = False
    t.options = _fbx_options()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    sm = unreal.load_asset(path)
    if not isinstance(sm, unreal.StaticMesh):
        raise RuntimeError("réimport raté : " + fbx)
    # Matériaux : par nom de slot, sinon dans l'ordre.
    slots = sm.get_editor_property("static_materials")
    by_name = {str(n): m for n, m in mats}
    for i, sl in enumerate(slots):
        m = by_name.get(str(sl.get_editor_property("material_slot_name")))
        if m is None and i < len(mats):
            m = mats[i][1]
        if m is not None:
            sm.set_material(i, m)
    if not eal.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError("enregistrement de %s impossible (jeu EON lancé ?)" % path)
    # Blueprint d'asset : recentrer le composant Mesh sur le nouveau maillage.
    b = sm.get_bounding_box()
    for _, o in _templates(bp):
        if isinstance(o, unreal.StaticMeshComponent) and o.get_name().startswith("Mesh") and o.get_editor_property("static_mesh") == sm:
            o.set_editor_property("relative_location", unreal.Vector(-(b.min.x + b.max.x) * 0.5, -(b.min.y + b.max.y) * 0.5, -b.min.z))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    bpp = bp.get_path_name().split(".")[0]
    if not eal.save_asset(bpp, only_if_is_dirty=False):
        raise RuntimeError("enregistrement de %s impossible (jeu EON lancé ?)" % bpp)
    unreal.log("EON assets : %s réimporté (FBX modifié)" % fbx)
    return True


def generate_all(root=None):
    root = root or assets_dir()
    _exported.clear()
    _stamps.clear()
    _stamps.update(_read_json(_stamps_path()) or {})
    done, created, missing, reimported = [], [], [], []
    for dp, _, files in os.walk(root):
        for f in sorted(files):
            if not f.lower().endswith(".fbx"):
                continue
            full = os.path.join(dp, f)
            rel = os.path.relpath(full, root).replace("\\", "/")
            rel_no_ext = os.path.splitext(rel)[0]
            sm = unreal.load_asset(mesh_path(rel_no_ext))
            if not isinstance(sm, unreal.StaticMesh):
                missing.append(rel)
                unreal.log_warning("EON assets : pas de StaticMesh importé pour %s" % rel)
                continue
            bp, new = ensure_bp(rel_no_ext, sm)
            if new:
                created.append(bp_path(rel_no_ext))
            if reimport_if_changed(full, sm, bp):
                reimported.append(rel)
            parts = bp_geometry(bp)
            base = os.path.splitext(full)[0]
            d = asset_dict(rel_no_ext, bp, parts, sm=sm, fbx=os.path.basename(rel), old=_read_json(base + ".eonasset"))
            _write_pair(base, d, parts, None)
            done.append(d["id"])
    with open(_stamps_path(), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(json.dumps(_stamps, ensure_ascii=False, indent=1, sort_keys=True) + "\n")
    dyn = generate_dynamics()
    generate_theme()
    try:
        import eon_salles
        eon_salles.publish_drive()
    except Exception as e:
        unreal.log_warning("EON assets : copie sur le Drive impossible (%s)" % e)
    unreal.log("EON assets : %d .eonasset FBX, %d dynamiques, %d Blueprints créés, %d FBX réimportés %s, %d textures, %d FBX sans mesh %s"
               % (len(done), len(dyn), len(created), len(reimported), reimported, len([v for v in _exported.values() if v]), len(missing), missing))
    generate_all.reimported = reimported
    return done + dyn, created, missing


def load_library(root=None):
    """id -> dict de tous les .eonasset de SourceAssets (FBX et dynamiques), pour l'import des salles."""
    root = root or source_dir()
    lib = {}
    for dp, _, files in os.walk(root):
        for f in files:
            if f.lower().endswith(".eonasset"):
                d = _read_json(os.path.join(dp, f))
                if d and d.get("format") == FORMAT:
                    lib[d["id"]] = d
                elif d is None:
                    unreal.log_warning("EON assets : %s illisible" % f)
    return lib
