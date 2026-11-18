# EON — salles au format .eonsalle (fichier texte JSON, éditable sur Mac et PC) <-> Blueprints de salle d'Unreal.
#
# Le fichier .eonsalle fait foi : l'éditeur web (Tools/EditeurSalles/EditeurSalles.html) l'ouvre et l'enregistre,
# l'import crée ou met à jour le Blueprint /Game/EON/Salles/BP_<nom>, l'export réécrit le fichier depuis le Blueprint.
#
#   import eon_salles
#   eon_salles.import_all()          # tous les .eonsalle du dossier Salles/ -> Blueprints
#   eon_salles.import_file(chemin)   # un seul fichier
#   eon_salles.export_all()          # tous les Blueprints de salle -> .eonsalle
#   eon_salles.watch(True)           # réimport automatique quand un fichier change (éditeur ouvert)
#
# Objets posés : {"asset": id .eonasset, "x", "y" (coin haut-gauche de l'empreinte tournée), "rot" (degrés)}. Chacun devient
# un ChildActorComponent du Blueprint de salle (classe = Blueprint d'asset BPA_*), marqué par ses tags eonasset= / eonpos=.
# Sous un objet solide, le plan du Blueprint reçoit des tuiles O (bloquent comme une caisse) ; le fichier garde le sol.
#
# Repère des portes dans le fichier : tuiles, origine en haut à gauche du plan (x vers la droite, y vers le bas).
#   extérieure : {"type": "exterieure", "cote": "N|S|E|W", "pos": première des 2 tuiles le long du bord}
#   intérieure : {"type": "interieure", "x": .., "y": .., "largeur": L, "sens": "horizontal|vertical"}
#                horizontal = L tuiles côte à côte sur une rangée (on passe du nord au sud), vertical = sur une colonne.
import json
import shutil
import subprocess
import math
import os
import time

import unreal

FORMAT = "eonsalle"
VERSION = 1
BP_DIR = "/Game/EON/Salles"
TAG_SOURCE = "EONSalleSource"
TAG_STAMP = "EONSalleStamp"
TILE_CM = 100.0
DOOR_Z = 60.0
ROLES = ["COMBAT", "TENSION", "CALME", "CORRIDOR", "DEPART", "OPPORTUNITE", "ASCENSEUR", "SECURITE", "MARCHAND",
         "FAILLE", "EVENEMENT", "GARDIEN", "TRAINING", "COM"]
FORMES = {"std": (32, 20), "carree": (32, 32), "large": (40, 26), "small": (24, 16), "corH": (50, 9), "corV": (12, 32)}
YAW = {"N": -90.0, "S": 90.0, "E": 0.0, "W": 180.0}


# Dossier maître sur le Google Drive (Vincent, 17/11) : on dessine les salles depuis n'importe quel PC ou Mac.
#   <DRIVE>/Salles            les .eonsalle (lus et écrits par l'import / l'export)
#   <DRIVE>/Bibliotheque      copie de SourceAssets sans les FBX (.eonasset, .eonapercu, Salle.eontheme, Textures)
#   <DRIVE>/EditeurSalles.html
# Le dossier Salles du projet en reste le miroir, pour l'historique Git. Sans Drive, tout se fait dans le projet.
DRIVE = r"G:\My Drive\EON_EditeurSalles"


def project_dir():
    return os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


def project_salles_dir():
    return os.path.join(project_dir(), "Salles")


def drive_on():
    return os.path.isdir(os.path.dirname(DRIVE))


def salles_dir():
    """Dossier des fichiers .eonsalle : <DRIVE>/Salles si le Drive est monté, sinon <projet>/Salles."""
    return os.path.join(DRIVE, "Salles") if drive_on() else project_salles_dir()


def _mirror(path):
    """Copie un .eonsalle du Drive dans le dossier Salles du projet (miroir pour Git)."""
    dst = os.path.join(project_salles_dir(), os.path.basename(path))
    if os.path.normcase(os.path.abspath(path)) == os.path.normcase(os.path.abspath(dst)):
        return
    os.makedirs(project_salles_dir(), exist_ok=True)
    shutil.copy2(path, dst)


def _copy_if_newer(src, dst):
    if os.path.exists(dst):
        a, b = os.stat(src), os.stat(dst)
        if int(a.st_mtime) <= int(b.st_mtime) and a.st_size == b.st_size:
            return False
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)
    return True


def _texture_web(src, dst):
    """Texture allégée pour l'éditeur en ligne (WebP 1024 px, ~30 Ko au lieu de 4 Mo).
    Renvoie 1 si écrite, 0 si déjà à jour, None si impossible (Pillow absent : la PNG est copiée)."""
    if os.path.exists(dst) and os.stat(dst).st_mtime >= os.stat(src).st_mtime:
        return 0
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    tool = os.path.join(project_dir(), "Tools", "EditeurSalles", "texture_web.py")
    try:
        r = subprocess.run(["py", "-3", tool, src, dst], capture_output=True, text=True, timeout=120,
                           creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        if r.returncode == 0 and os.path.exists(dst):
            return 1
        unreal.log_warning("EON salles : texture WebP impossible (%s) : %s" % (os.path.basename(src), (r.stderr or "").strip()[-300:]))
    except Exception as e:
        unreal.log_warning("EON salles : texture WebP impossible (%s) : %s" % (os.path.basename(src), e))
    return None


def publish_drive():
    """Met la bibliothèque d'assets et l'éditeur sur le Drive ; la première fois, y recopie les salles du projet."""
    if not drive_on():
        unreal.log_warning("EON salles : Drive absent (%s)" % os.path.dirname(DRIVE))
        return 0
    src = os.path.join(project_dir(), "SourceAssets")
    lib = os.path.join(DRIVE, "Bibliotheque")
    n = 0
    keep = set()
    for dp, _, files in os.walk(src):
        for f in files:
            if not f.lower().endswith((".eonasset", ".eonapercu", ".eontheme", ".png")):
                continue
            if f.lower().endswith(".png") and os.path.basename(dp) != "Textures":
                continue
            rel = os.path.relpath(os.path.join(dp, f), src)
            if f.lower().endswith(".png"):
                web = _texture_web(os.path.join(dp, f), os.path.join(lib, rel[:-4] + ".webp"))
                if web is not None:
                    keep.add(os.path.normcase(rel[:-4] + ".webp"))
                    n += web
                    continue
            keep.add(os.path.normcase(rel))
            n += _copy_if_newer(os.path.join(dp, f), os.path.join(lib, rel))
    # Ce qui n'est plus dans la bibliothèque du projet est retiré du Drive.
    for dp, _, files in os.walk(lib):
        for f in files:
            rel = os.path.relpath(os.path.join(dp, f), lib)
            if os.path.normcase(rel) not in keep:
                os.remove(os.path.join(dp, f))
    n += _copy_if_newer(os.path.join(project_dir(), "Tools", "EditeurSalles", "EditeurSalles.html"), os.path.join(DRIVE, "EditeurSalles.html"))
    # Code de l'appli Google (à recopier dans le projet Apps Script s'il change).
    for f in ("Code.gs", "appsscript.json"):
        n += _copy_if_newer(os.path.join(project_dir(), "Tools", "EditeurSalles", "AppsScript", f), os.path.join(DRIVE, "AppsScript", f))
    ds = os.path.join(DRIVE, "Salles")
    if not os.path.isdir(ds) or not any(f.lower().endswith(".eonsalle") for f in os.listdir(ds)):
        os.makedirs(ds, exist_ok=True)
        for f in os.listdir(project_salles_dir()):
            if f.lower().endswith(".eonsalle"):
                shutil.copy2(os.path.join(project_salles_dir(), f), os.path.join(ds, f))
                n += 1
    unreal.log("EON salles : Drive à jour (%d fichier(s) copiés) -> %s" % (n, DRIVE))
    return n


def _num(v):
    # Nombre entier écrit sans ".0", comme l'éditeur de salles (JavaScript) : mêmes fichiers des deux côtés.
    v = float(v)
    return int(v) if v == int(v) else v


def _rnd(x):
    # FMath::RoundToInt : arrondi au plus proche, .5 vers le haut.
    return int(math.floor(x + 0.5))


def _clamp(v, a, b):
    return max(a, min(b, v))


def _dir_from_yaw(yaw):
    r = math.radians(yaw)
    c, s = math.cos(r), math.sin(r)
    if abs(c) >= abs(s):
        return "E" if c > 0 else "W"
    return "S" if s > 0 else "N"


def _sub():
    return unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def _door_components(bp):
    sub = _sub()
    out = []
    for h in sub.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if isinstance(o, unreal.EONDoorComponent):
            out.append((h, o))
    return out


def _tag(o, key):
    for t in o.get_editor_property("component_tags"):
        t = str(t)
        if t.startswith(key + "="):
            return t[len(key) + 1:]
    return None


def _object_components(bp):
    sub = _sub()
    out = []
    for h in sub.k2_gather_subobject_data_for_blueprint(bp):
        o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        if isinstance(o, unreal.ChildActorComponent) and _tag(o, "eonasset") is not None:
            out.append((h, o))
    return out


def _rot_dims(a, rot):
    w, h = a["empreinte"]
    return (h, w) if int(rot) % 180 else (w, h)


def _norm_plan(rows):
    w = max((len(r) for r in rows), default=0)
    out = []
    for r in rows:
        r = r.replace(" ", "_").replace("D", "#").replace("x", "X").replace("k", "K").replace("O", ".")
        out.append(r + "#" * (w - len(r)))
    return out, w, len(out)


# ---------------------------------------------------------------- Blueprint -> fichier
def bp_to_dict(bp):
    cdo = unreal.get_default_object(bp.generated_class())
    plan, w, h = _norm_plan(list(cdo.get_editor_property("layout")))
    portes = []
    for _, d in _door_components(bp):
        loc = d.get_editor_property("relative_location")
        yaw = d.get_editor_property("relative_rotation").yaw
        tx, ty = (loc.x + w * TILE_CM * 0.5) / TILE_CM, (loc.y + h * TILE_CM * 0.5) / TILE_CM
        cote = _dir_from_yaw(yaw)
        if d.get_editor_property("exterieure"):
            if cote in ("N", "S"):
                pos = _clamp(_rnd(tx) - 1, 1, w - 3)
            else:
                pos = _clamp(_rnd(ty) - 1, 1, h - 3)
            portes.append({"type": "exterieure", "cote": cote, "pos": pos})
        else:
            L = _clamp(int(d.get_editor_property("largeur")), 1, 8)
            if cote in ("E", "W"):
                x0, y0, sens = _clamp(math.floor(tx), 1, w - 2), _clamp(_rnd(ty - L * 0.5), 1, h - 1 - L), "vertical"
            else:
                x0, y0, sens = _clamp(_rnd(tx - L * 0.5), 1, w - 1 - L), _clamp(math.floor(ty), 1, h - 2), "horizontal"
            portes.append({"type": "interieure", "x": int(x0), "y": int(y0), "largeur": L, "sens": sens})
    # Ordre stable (l'ordre des composants change d'une session à l'autre) : extérieures N, S, E, W puis intérieures.
    portes.sort(key=lambda p: (0, "NSEW".index(p["cote"]), 0) if p["type"] == "exterieure" else (1, p["y"], p["x"]))
    objets = []
    for _, o in sorted(_object_components(bp), key=lambda ho: ho[1].get_name()):
        pos = (_tag(o, "eonpos") or "0,0,0").split(",")
        ob = {"asset": _tag(o, "eonasset"), "x": int(pos[0]), "y": int(pos[1]), "rot": int(pos[2])}
        lum = _tag(o, "eonlum")
        if lum:
            q = lum.split(";")
            ob["lumiere"] = {"mode": q[0], "couleur": q[1], "rayon_px": float(q[2]), "intensite": float(q[3])}
            for k in ("rayon_px", "intensite"):
                if ob["lumiere"][k] == int(ob["lumiere"][k]):
                    ob["lumiere"][k] = int(ob["lumiere"][k])
        objets.append(ob)
    nom = bp.get_name()
    if nom.startswith("BP_"):
        nom = nom[3:]
    shape = str(cdo.get_editor_property("shape"))
    return {
        "format": FORMAT,
        "version": VERSION,
        "nom": nom,
        "role": str(cdo.get_editor_property("room_role")),
        "rolesEnPlus": [str(r) for r in cdo.get_editor_property("extra_roles")],
        "forme": "" if shape in ("None", "") else shape,
        "poids": _num(cdo.get_editor_property("weight")),
        "portes": portes,
        "objets": objets,
        "plan": plan,
    }


def _lignes(a):
    return "[\n" + ",\n".join("    " + json.dumps(o, ensure_ascii=False) for o in a) + "\n  ]" if a else "[]"


def file_text(d):
    """Texte d'un .eonsalle : une porte, un objet, une rangée du plan par ligne (même texte que toFileText de l'éditeur)."""
    head = [(k, d[k]) for k in ("format", "version", "nom", "role", "rolesEnPlus", "forme")]
    txt = "{\n" + ",\n".join("  %s: %s" % (json.dumps(k), json.dumps(v, ensure_ascii=False)) for k, v in head)
    txt += ',\n  "poids": %r' % float(d["poids"])
    txt += ',\n  "portes": ' + _lignes(d.get("portes", [])) + ',\n  "objets": ' + _lignes(d.get("objets", []))
    txt += ',\n  "plan": ' + _lignes(d["plan"]) + "\n}\n"
    return txt


def write_file(d, path):
    txt = file_text(d)
    json.loads(txt)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(txt)


def export_bp(asset_path, out_dir=None):
    bp = unreal.load_asset(asset_path)
    if not bp:
        raise RuntimeError("Blueprint introuvable : " + asset_path)
    d = bp_to_dict(bp)
    out_dir = out_dir or salles_dir()
    os.makedirs(out_dir, exist_ok=True)
    path = os.path.join(out_dir, d["nom"] + ".eonsalle")
    write_file(d, path)
    _mirror(path)
    eal = unreal.EditorAssetLibrary
    eal.set_metadata_tag(bp, TAG_SOURCE, path.replace("\\", "/"))
    eal.set_metadata_tag(bp, TAG_STAMP, _stamp(path))
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    unreal.log("EON salles : exporté %s -> %s" % (asset_path, path))
    return path


def list_room_bps():
    out = []
    for p in unreal.EditorAssetLibrary.list_assets(BP_DIR, recursive=True, include_folder=False):
        a = unreal.EditorAssetLibrary.find_asset_data(p)
        if str(a.asset_class_path.asset_name) != "Blueprint":
            continue
        bp = unreal.load_asset(p)
        gc = bp.generated_class() if bp else None
        if gc and isinstance(unreal.get_default_object(gc), unreal.EONTileRoom):
            out.append(p.split(".")[0])
    return out


def export_all(out_dir=None):
    paths = [export_bp(p, out_dir) for p in list_room_bps()]
    unreal.log("EON salles : %d salles exportées" % len(paths))
    return paths


# ---------------------------------------------------------------- fichier -> Blueprint
def _stamp(path):
    st = os.stat(path)
    return "%d:%d" % (int(st.st_mtime), st.st_size)


def read_file(path):
    with open(path, encoding="utf-8-sig") as f:
        d = json.load(f)
    if d.get("format") != FORMAT:
        raise ValueError("%s : ce n'est pas un fichier .eonsalle" % path)
    if int(d.get("version", 0)) > VERSION:
        raise ValueError("%s : version %s plus récente que l'import (%d)" % (path, d.get("version"), VERSION))
    return d


def validate(d):
    """Mêmes règles que AEONTileRoom::LayoutToGrid. Renvoie la liste des erreurs (vide = bon)."""
    err = []
    plan, w, h = _norm_plan(d.get("plan", []))
    if w < 8 or h < 6:
        err.append("plan de %dx%d, minimum 8x6" % (w, h))
    if sum(r.count(".") for r in plan) < 12:
        err.append("moins de 12 tuiles de sol")
    bad = set("".join(plan)) - set("#._=XKO")
    if bad:
        err.append("caractères inconnus dans le plan : %s" % "".join(sorted(bad)))
    if not d.get("nom") or any(c in d["nom"] for c in " /\\.:"):
        err.append("nom vide ou avec espace / point / barre")
    for o in d.get("objets", []):
        if not isinstance(o, dict) or "asset" not in o or int(o.get("x", -1)) < 0 or int(o.get("y", -1)) < 0 or int(o.get("x", 0)) >= w or int(o.get("y", 0)) >= h:
            err.append("objet mal placé : %s" % o)
    cotes = [p["cote"] for p in d.get("portes", []) if p.get("type") == "exterieure"]
    for c in set(cotes):
        if cotes.count(c) > 1:
            err.append("deux portes extérieures côté %s" % c)
    return err


def _door_list(d, w, h):
    """Portes du fichier -> (nom, x cm, y cm, lacet, extérieure, largeur), repère du Blueprint (pivot au centre)."""
    out, n = [], {}
    def cm(tx, ty):
        return tx * TILE_CM - w * TILE_CM * 0.5, ty * TILE_CM - h * TILE_CM * 0.5
    for p in d.get("portes", []):
        if p.get("type") == "exterieure":
            c = p["cote"]
            pos = int(p["pos"])
            if c == "N":
                x, y = cm(pos + 1, 0)
            elif c == "S":
                x, y = cm(pos + 1, h)
            elif c == "E":
                x, y = cm(w, pos + 1)
            else:
                x, y = cm(0, pos + 1)
            out.append(("Porte_" + c, x, y, YAW[c], True, 2))
        else:
            L = _clamp(int(p.get("largeur", 2)), 1, 8)
            x0, y0 = int(p["x"]), int(p["y"])
            k = n.get("int", 0) + 1
            n["int"] = k
            if p.get("sens") == "vertical":
                x, y = cm(x0 + 0.5, y0 + L * 0.5)
                yaw = 0.0
            else:
                x, y = cm(x0 + L * 0.5, y0 + 0.5)
                yaw = 90.0
            out.append(("Porte_Interieure_%d" % k, x, y, yaw, False, L))
    return out


def import_file(path, save=True):
    path = os.path.normpath(path)
    d = read_file(path)
    err = validate(d)
    if err:
        raise ValueError("%s : %s" % (os.path.basename(path), " ; ".join(err)))
    plan, w, h = _norm_plan(d["plan"])
    objets = list(d.get("objets", []))
    lib = None
    if objets:
        import eon_assets
        lib = eon_assets.load_library()
        absent = sorted(set(o["asset"] for o in objets if o["asset"] not in lib))
        if absent:
            raise ValueError("%s : assets absents de la bibliothèque : %s" % (os.path.basename(path), ", ".join(absent)))
    eal = unreal.EditorAssetLibrary
    dst = BP_DIR + "/BP_" + d["nom"]
    if eal.does_asset_exist(dst):
        bp = unreal.load_asset(dst)
        created = False
    else:
        fac = unreal.BlueprintFactory()
        fac.set_editor_property("parent_class", unreal.EONTileRoom)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_" + d["nom"], BP_DIR, unreal.Blueprint, fac)
        created = True
    if not bp:
        raise RuntimeError("impossible de créer " + dst)
    sub = _sub()
    # Portes : on remplace toutes celles du Blueprint par celles du fichier.
    old = _door_components(bp) + _object_components(bp)
    if old:
        bp.modify()
        sub.delete_subobjects(sub.k2_gather_subobject_data_for_blueprint(bp)[0], [h for h, _ in old], bp)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    root = sub.k2_gather_subobject_data_for_blueprint(bp)[0]
    door_cls = unreal.load_class(None, "/Script/EON.EONDoorComponent")
    for (name, x, y, yaw, ext, larg) in _door_list(d, w, h):
        hd, fail = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=door_cls, blueprint_context=bp))
        if not fail.is_empty():
            raise RuntimeError("porte %s : %s" % (name, fail))
        sub.rename_subobject(hd, unreal.Text(name))
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(hd))
        comp.set_editor_property("relative_location", unreal.Vector(x, y, DOOR_Z))
        comp.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, yaw))
        comp.set_editor_property("exterieure", ext)
        comp.set_editor_property("largeur", larg)
    # Objets : un ChildActorComponent par objet, pivot du Blueprint d'asset au centre de son empreinte, au sol.
    grid = [list(r) for r in plan]
    for i, o in enumerate(objets):
        a = lib[o["asset"]]
        rot = int(o.get("rot", 0)) % 360
        ow, oh = _rot_dims(a, rot)
        x0, y0 = int(o["x"]), int(o["y"])
        cx, cy = (x0 + ow * 0.5) * TILE_CM - w * TILE_CM * 0.5, (y0 + oh * 0.5) * TILE_CM - h * TILE_CM * 0.5
        r = math.radians(rot)
        mx, my = a["centre_cm"]
        px, py = cx - (mx * math.cos(r) - my * math.sin(r)), cy - (mx * math.sin(r) + my * math.cos(r))
        cls = unreal.load_object(None, a["unreal"])
        if not cls:
            raise RuntimeError("Blueprint d'asset introuvable : " + a["unreal"])
        hd, fail = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.ChildActorComponent, blueprint_context=bp))
        if not fail.is_empty():
            raise RuntimeError("objet %s : %s" % (o["asset"], fail))
        sub.rename_subobject(hd, unreal.Text("Objet_%02d_%s" % (i + 1, os.path.basename(o["asset"]))))
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(hd))
        comp.set_editor_property("relative_location", unreal.Vector(px, py, -float(a.get("sol_cm", 0.0))))
        comp.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, float(rot)))
        comp.set_child_actor_class(cls)
        tags = ["eonasset=" + o["asset"], "eonpos=%d,%d,%d" % (x0, y0, rot)]
        # Réglages de la lumière posée (lus par AEONLampe au lancement).
        if isinstance(o.get("lumiere"), dict):
            l = o["lumiere"]
            tags.append("eonlum=%s;%s;%g;%g" % (l.get("mode", "fixe"), l.get("couleur", "#ff3d8b"), float(l.get("rayon_px", 52)), float(l.get("intensite", 0.8))))
        comp.set_editor_property("component_tags", tags)
        if a.get("collision", "solide") != "aucune":
            for yy in range(y0, y0 + oh):
                for xx in range(x0, x0 + ow):
                    if 0 <= yy < h and 0 <= xx < w and grid[yy][xx] == ".":
                        grid[yy][xx] = "O"
    plan = ["".join(r) for r in grid]
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("layout", plan)
    cdo.set_editor_property("room_role", d.get("role") or "COMBAT")
    cdo.set_editor_property("extra_roles", list(d.get("rolesEnPlus", [])))
    cdo.set_editor_property("shape", d.get("forme") or "None")
    cdo.set_editor_property("weight", float(d.get("poids", 1.0)))
    eal.set_metadata_tag(bp, TAG_SOURCE, path.replace("\\", "/"))
    eal.set_metadata_tag(bp, TAG_STAMP, _stamp(path))
    if save and not eal.save_asset(dst, only_if_is_dirty=False):
        raise RuntimeError("enregistrement de %s impossible : fichier verrouillé (jeu EON lancé ?)" % dst)
    unreal.log("EON salles : %s %s (%dx%d, %d portes, %d objets)" % ("créé" if created else "mis à jour", dst, w, h, len(d.get("portes", [])), len(objets)))
    _mirror(path)
    return dst


def import_all(src_dir=None, only_changed=False):
    src_dir = src_dir or salles_dir()
    done, errors = [], []
    if not os.path.isdir(src_dir):
        unreal.log_warning("EON salles : dossier absent " + src_dir)
        return done, errors
    for f in sorted(os.listdir(src_dir)):
        if not f.lower().endswith(".eonsalle"):
            continue
        p = os.path.join(src_dir, f)
        if only_changed and not _changed(p):
            continue
        try:
            done.append(import_file(p))
        except Exception as e:  # une salle fausse n'arrête pas les autres
            errors.append("%s : %s" % (f, e))
            unreal.log_error("EON salles : " + errors[-1])
    unreal.log("EON salles : %d importées, %d erreurs" % (len(done), len(errors)))
    return done, errors


def _changed(path):
    try:
        d = read_file(path)
    except Exception:
        return True
    dst = BP_DIR + "/BP_" + str(d.get("nom"))
    eal = unreal.EditorAssetLibrary
    if not eal.does_asset_exist(dst):
        return True
    return eal.get_metadata_tag(unreal.load_asset(dst), TAG_STAMP) != _stamp(path)


# ---------------------------------------------------------------- surveillance du dossier
_watch = {"handle": None, "t": 0.0}


def _tick(dt):
    _watch["t"] += dt
    if _watch["t"] < 2.0:
        return
    _watch["t"] = 0.0
    try:
        done, errors = import_all(only_changed=True)
        if done or errors:
            msg = "%d salle(s) mise(s) à jour" % len(done) + ("" if not errors else ", %d erreur(s) (voir Output Log)" % len(errors))
            unreal.log("EON salles (auto) : " + msg)
    except Exception as e:
        unreal.log_error("EON salles (auto) : %s" % e)


def watch(on=True):
    if on and _watch["handle"] is None:
        _watch["handle"] = unreal.register_slate_post_tick_callback(_tick)
        unreal.log("EON salles : réimport automatique activé (%s)" % salles_dir())
    elif not on and _watch["handle"] is not None:
        unreal.unregister_slate_post_tick_callback(_watch["handle"])
        _watch["handle"] = None
        unreal.log("EON salles : réimport automatique coupé")
    return _watch["handle"] is not None


def toggle_watch():
    return watch(_watch["handle"] is None)


def open_editor():
    import webbrowser
    p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Tools", "EditeurSalles", "EditeurSalles.html")
    webbrowser.open("file:///" + os.path.normpath(p).replace("\\", "/"))
