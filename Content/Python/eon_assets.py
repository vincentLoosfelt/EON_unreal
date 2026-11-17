# EON — bibliothèque d'assets de l'éditeur de salles.
#
# Chaque asset posable est un Blueprint d'acteur /Game/EON/RoomAssets/<chemin>/BPA_<nom> : on peut y ajouter scripts,
# colliders, lumières... sans toucher aux salles. Pour chaque FBX de SourceAssets/FBX (copie des FBX d'Unity) :
#   - le Blueprint est créé s'il n'existe pas (un StaticMesh, pivot recentré : origine = centre de l'empreinte, au sol) ;
#     un Blueprint existant n'est jamais recréé ni modifié ;
#   - <nom>.eonasset (JSON) et <nom>.eonapercu (géométrie simplifiée) sont écrits à côté du FBX, mesurés dans le Blueprint
#     (tous ses composants StaticMesh, avec leurs transformations).
#
#   import eon_assets
#   eon_assets.generate_all()
#
# .eonasset :
#   id          chemin relatif sans extension, unique : "moniteur/Computer_hitech_V1"
#   nom, categorie, tags            réglés par l'artiste, gardés d'une génération à l'autre
#   collision   "solide" (les tuiles sous l'objet bloquent comme une caisse) ou "aucune" ; gardé aussi
#   rotations   lacets permis en degrés ; gardé aussi
#   fbx         FBX source (à côté du fichier) ; mesh : le StaticMesh importé
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
MAX_TRIS = 1500

CATEGORIES = {
    "": "Cuves", "Barriere": "Barrières", "boss": "Boss", "enviro": "Environnement", "enviro/wall": "Murs",
    "highttech/Assets_Hitech_big": "High-tech grands", "highttech/Assets_Hitech_medium": "High-tech moyens",
    "highttech/Assets_Hitech_small": "High-tech petits", "lootable": "Ramassables", "moniteur": "Moniteurs", "weapons": "Armes",
}


def assets_dir():
    return os.path.normpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceAssets", "FBX"))


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


# ---------------------------------------------------------------- géométrie (tous les StaticMesh du Blueprint)
_NAMED = [("glass", "#9fd8e6"), ("verre", "#9fd8e6"), ("screen", "#4fe0ff"), ("green", "#3fbf6a"), ("red", "#d84a3a"),
          ("yellow", "#e8c23a"), ("blue", "#3a78d8"), ("white", "#e6e8ea"), ("very_light", "#cfd3d6"), ("light_grey", "#aab0b5"),
          ("dark", "#3b4045"), ("black", "#1c1e21"), ("grey", "#7d848a"), ("light", "#f2e6a0"), ("metal", "#8a9096"), ("wall", "#5d7a74")]


def _hex(c):
    return "#%02x%02x%02x" % tuple(max(0, min(255, int(round(v * 255)))) for v in (c.r, c.g, c.b))


def _mat_color(mat):
    if not mat:
        return "#8a9096"
    for p in ("BaseColor", "Base Color", "Color", "Albedo", "Tint", "_Color"):
        try:
            if isinstance(mat, unreal.MaterialInstance):
                v = unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(mat, p)
                if v.r + v.g + v.b > 0.0:
                    return _hex(v)
        except Exception:
            pass
    n = mat.get_name().lower()
    for k, c in _NAMED:
        if k in n:
            return c
    return "#8a9096"


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


def bp_geometry(bp):
    """Pièces [sommets (cm, repère du Blueprint), triangles, couleur] de tous les StaticMesh du Blueprint."""
    comps = [o for _, o in _templates(bp) if isinstance(o, unreal.SceneComponent)]
    parts = []
    for c in comps:
        if not isinstance(c, unreal.StaticMeshComponent):
            continue
        sm = c.get_editor_property("static_mesh")
        if not sm:
            continue
        t = _world_of(c, comps)
        for s in range(sm.get_num_sections(0)):
            res = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(sm, 0, s)
            v = []
            for p in res[0]:
                q = t.transform_location(p)
                v.append((q.x, q.y, q.z))
            mat = c.get_material(s) if hasattr(c, "get_material") else None
            parts.append([v, list(res[1]), _mat_color(mat or sm.get_material(s))])
    return parts


def _simplify(verts, tris, cell):
    idx, out_v, remap = {}, [], []
    for v in verts:
        k = (int(math.floor(v[0] / cell + 0.5)), int(math.floor(v[1] / cell + 0.5)), int(math.floor(v[2] / cell + 0.5)))
        if k not in idx:
            idx[k] = len(out_v)
            out_v.append((k[0] * cell, k[1] * cell, k[2] * cell))
        remap.append(idx[k])
    out_t, seen = [], set()
    for i in range(0, len(tris), 3):
        a, b, c = remap[tris[i]], remap[tris[i + 1]], remap[tris[i + 2]]
        if a == b or b == c or a == c:
            continue
        key = tuple(sorted((a, b, c)))
        if key in seen:
            continue
        seen.add(key)
        out_t += [a, b, c]
    return out_v, out_t


def preview(parts):
    total = sum(len(t) // 3 for _, t, _ in parts)
    allv = [p for v, _, _ in parts for p in v]
    if not allv:
        return []
    ext = max(max(p[i] for p in allv) - min(p[i] for p in allv) for i in range(3)) or 1.0
    cell = 0.0
    while total > MAX_TRIS and cell < ext / 4:
        cell = cell * 1.5 if cell else ext / 160.0
        simp = [[*_simplify(v, t, cell), c] for v, t, c in parts]
        total = sum(len(t) // 3 for _, t, _ in simp)
        if total <= MAX_TRIS:
            parts = simp
    return [{"couleur": c, "v": [int(round(x)) for p in v for x in p], "t": t} for v, t, c in parts if t]


# ---------------------------------------------------------------- .eonasset
def _footprint(cm):
    # Tolérance de 15 cm : un objet de 1,1 m tient sur 1 tuile.
    return max(1, int(math.ceil(cm / 100.0 - 0.15)))


def asset_dict(rel, sm, bp, parts, old=None):
    allv = [p for v, _, _ in parts for p in v] or [(0.0, 0.0, 0.0)]
    mn = [min(p[i] for p in allv) for i in range(3)]
    mx = [max(p[i] for p in allv) for i in range(3)]
    sx, sy, sz = mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]
    rel_no_ext = os.path.splitext(rel)[0].replace("\\", "/")
    folder, name = os.path.dirname(rel_no_ext), os.path.basename(rel_no_ext)
    d = {
        "format": FORMAT,
        "version": VERSION,
        "id": rel_no_ext,
        "nom": name.replace("_", " "),
        "categorie": CATEGORIES.get(folder, folder or "Divers"),
        "tags": [],
        "collision": "solide" if sz >= 40.0 and max(sx, sy) >= 30.0 else "aucune",
        "rotations": [0, 90, 180, 270],
        "fbx": os.path.basename(rel),
        "mesh": sm.get_path_name() if sm else "",
        "unreal": bp.generated_class().get_path_name(),
        "empreinte": [_footprint(sx), _footprint(sy)],
        "taille_cm": [round(sx, 1), round(sy, 1), round(sz, 1)],
        "centre_cm": [round((mn[0] + mx[0]) * 0.5, 1), round((mn[1] + mx[1]) * 0.5, 1)],
        "sol_cm": round(mn[2], 1),
        "apercu": name + ".eonapercu",
    }
    if old:
        for k in ("nom", "categorie", "tags", "collision", "rotations"):
            if k in old:
                d[k] = old[k]
    return d


def generate_all(root=None):
    root = root or assets_dir()
    done, created, missing = [], [], []
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
            parts = bp_geometry(bp)
            base = os.path.splitext(full)[0]
            path = base + ".eonasset"
            old = None
            if os.path.exists(path):
                try:
                    with open(path, encoding="utf-8-sig") as fh:
                        old = json.load(fh)
                except Exception:
                    old = None
            d = asset_dict(rel, sm, bp, parts, old)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(json.dumps(d, ensure_ascii=False, indent=2) + "\n")
            with open(base + ".eonapercu", "w", encoding="utf-8", newline="\n") as fh:
                fh.write(json.dumps({"format": "eonapercu", "version": 1, "pieces": preview(parts)}, separators=(",", ":")) + "\n")
            done.append(d["id"])
    unreal.log("EON assets : %d .eonasset, %d Blueprints créés, %d FBX sans mesh %s" % (len(done), len(created), len(missing), missing))
    return done, created, missing


def load_library(root=None):
    """id -> dict de tous les .eonasset (pour l'import des salles)."""
    root = root or assets_dir()
    lib = {}
    for dp, _, files in os.walk(root):
        for f in files:
            if f.lower().endswith(".eonasset"):
                try:
                    with open(os.path.join(dp, f), encoding="utf-8-sig") as fh:
                        d = json.load(fh)
                    if d.get("format") == FORMAT:
                        lib[d["id"]] = d
                except Exception as e:
                    unreal.log_warning("EON assets : %s illisible (%s)" % (f, e))
    return lib
