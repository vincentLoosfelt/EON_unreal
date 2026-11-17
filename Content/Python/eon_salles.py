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
# Repère des portes dans le fichier : tuiles, origine en haut à gauche du plan (x vers la droite, y vers le bas).
#   extérieure : {"type": "exterieure", "cote": "N|S|E|W", "pos": première des 2 tuiles le long du bord}
#   intérieure : {"type": "interieure", "x": .., "y": .., "largeur": L, "sens": "horizontal|vertical"}
#                horizontal = L tuiles côte à côte sur une rangée (on passe du nord au sud), vertical = sur une colonne.
import json
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


def salles_dir():
    """Dossier des fichiers .eonsalle : <projet>/Salles."""
    return os.path.normpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Salles"))


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


def _norm_plan(rows):
    w = max((len(r) for r in rows), default=0)
    out = []
    for r in rows:
        r = r.replace(" ", "_").replace("D", "#").replace("x", "X").replace("k", "K")
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
        "poids": float(cdo.get_editor_property("weight")),
        "plan": plan,
        "portes": portes,
    }


def write_file(d, path):
    # Une rangée du plan par ligne : le fichier reste lisible et se compare bien dans Git.
    txt = json.dumps({k: v for k, v in d.items() if k != "plan"}, ensure_ascii=False, indent=2)
    plan = ",\n".join("    " + json.dumps(r) for r in d["plan"])
    txt = txt[:-2] + ',\n  "plan": [\n' + plan + "\n  ]\n}\n"
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
    bad = set("".join(plan)) - set("#._=XK")
    if bad:
        err.append("caractères inconnus dans le plan : %s" % "".join(sorted(bad)))
    if not d.get("nom") or any(c in d["nom"] for c in " /\\.:"):
        err.append("nom vide ou avec espace / point / barre")
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
    old = _door_components(bp)
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
    unreal.log("EON salles : %s %s (%dx%d, %d portes)" % ("créé" if created else "mis à jour", dst, w, h, len(d.get("portes", []))))
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
