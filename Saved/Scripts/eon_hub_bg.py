# EON — L_HUB aux couleurs d'Unity : chaque pièce reçoit, emplacement par emplacement, le MI_BG_* du .mat que
# lui donne le prefab Room_elevator (hub_elevator.json) ; COM : surcharges par pièce de COMv1.prefab.
# Lumières et post-process repris de SampleScene + Global.asset.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_hub_bg.py   (après eon_bg_master.py)
import json, math, re, unreal
HERE = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
DATA = json.load(open(HERE + "hub_elevator.json", encoding="utf-8"))
MAP = "/Game/EON/Maps/L_HUB"
MI_DIR = "/Game/EON/Materials/BG/"
GUID_BASEWHITE = "f5c03b7d48332454cbf96bd82bc98f12"
# COMv1.prefab : surcharges par pièce (voir eon_com_materials.py) ; light = émissif du FBX ; dôme = M_EON_ChampForce.
COM_SLOTS = {"metal": "New_Material", "ground": "New_Material_2", "metal_001": "New_Material_3", "Material_006": "New_Material_1"}
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_HUBBG " + str(m))


def mi_for(unity_mat):
    if unity_mat == GUID_BASEWHITE:
        n = "BaseWhite"
    else:
        n = re.sub(r"[^A-Za-z0-9]", "_", unity_mat.rsplit("/", 1)[-1].rsplit(".", 1)[0])
    a = unreal.load_asset(MI_DIR + "MI_BG_" + n)
    if a is None:
        log("MI absente : MI_BG_" + n)
    return a


def qrot(q, v):
    x, y, z, w = q

    def m(a, b):
        ax, ay, az, aw = a; bx, by, bz, bw = b
        return [aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
                aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz]
    return m(m(q, [v[0], v[1], v[2], 0]), [-x, -y, -z, w])[:3]


def to_ue(v):
    return unreal.Vector(v[2], v[0], v[1])


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level(MAP)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()
smas = [a for a in actors if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh]

done = miss = 0
for o in DATA:
    if not o.get("fbx") or not o.get("mats"):
        continue
    mesh_name = re.sub(r"[^A-Za-z0-9_]", "_", o["fbx"].rsplit("/", 1)[-1].rsplit(".", 1)[0])
    loc = to_ue(o["pos"]) * 100.0
    best = None
    for a in smas:
        if not a.static_mesh_component.static_mesh.get_name().startswith(mesh_name):
            continue
        d = (a.get_actor_location() - loc).length()
        if d < 3.0 and (best is None or d < best[0]):
            best = (d, a)
    if best is None:
        miss += 1
        continue
    comp = best[1].static_mesh_component
    if mesh_name == "COMv1":
        for slot in comp.get_material_slot_names():
            key = str(slot).replace(".", "_")
            if key in COM_SLOTS:
                comp.set_material_by_name(slot, unreal.load_asset(MI_DIR + "MI_BG_" + COM_SLOTS[key]))
                log("COM %s -> MI_BG_%s" % (slot, COM_SLOTS[key]))
        done += 1
        continue
    n = comp.get_num_materials()
    for i, um in enumerate(o["mats"][:n]):
        mi = mi_for(um)
        if mi:
            comp.set_material(i, mi)
    done += 1
log("pièces habillées : %d ; sans acteur correspondant : %d" % (done, miss))

# Murs du HUB (Vincent, 15/11) : « un shader unique, vert de gris assez foncé, voire kaki, avec l'effet painting ».
# Les emplacements d'Unreal ne suivent pas l'ordre des sous-meshes d'Unity (WallA320 : « light » = 4 triangles des
# fines lignes lumineuses, « metal01 » = le mur). Poser par index mettait Room light sur tout le mur (murs blancs) :
# on pose donc par nom d'emplacement. light -> Room light ; le reste -> walll painted (vert-de-gris de MasterPainted).
MUR = unreal.load_asset(MI_DIR + "MI_BG_walll_painted")
NEON = unreal.load_asset(MI_DIR + "MI_BG_Room_light")
MURS = ("WallA320", "WallA160CornerIn", "walldooreno")
murs = neons = 0
for a in smas:
    comp = a.static_mesh_component
    if not comp.static_mesh.get_name().startswith(MURS):
        continue
    for slot in comp.get_material_slot_names():
        if str(slot).lower() == "light":
            comp.set_material_by_name(slot, NEON); neons += 1
        else:
            comp.set_material_by_name(slot, MUR); murs += 1
log("murs : %d emplacements en MI_BG_walll_painted, %d lignes lumineuses en MI_BG_Room_light" % (murs, neons))

# Lumières de SampleScene. URP : radiance = couleur linéaire × intensité, sans 1/π -> Unreal : lux = intensité × π.
UNITY_LIGHTS = [("Soleil_principal", (0.2622, 0.7108, -0.3314, 0.5623), (1.0, 0.8058, 0.6761), 0.6, False),
                ("Soleil_2", (0.1094, -0.8754, 0.4082, 0.2346), (1.0, 1.0, 1.0), 0.56, True)]
dirs = [a for a in actors if isinstance(a, unreal.DirectionalLight)]
for i, (label, q, col, inten, shadows) in enumerate(UNITY_LIGHTS):
    fwd = to_ue(qrot(q, [0, 0, 1]))
    rot = unreal.MathLibrary.make_rot_from_x(fwd)
    L = dirs[i] if i < len(dirs) else eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1200), rot)
    L.set_actor_rotation(rot, False)
    lc = L.light_component
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property("intensity", inten * math.pi)
    r, g, b = [int(round(255 * c)) for c in col]      # couleur Unity en espace gamma = FColor sRGB
    lc.set_editor_property("light_color", unreal.Color(r=r, g=g, b=b, a=255))
    lc.set_editor_property("cast_shadows", shadows)
    lc.set_editor_property("atmosphere_sun_light", i == 0)
    L.set_actor_label(label)
    log("%s : rot %s, %.2f lux" % (label, rot, inten * math.pi))
for a in dirs[len(UNITY_LIGHTS):]:
    eas.destroy_actor(a)

# Post-process = Global.asset : bloom seuil 1 / intensité 1, pas de tonemapping (Unity sans Tonemapping) ;
# exposition fixe à 1 (manuelle, sans caméra physique : 2^biais / 1,2 = 1).
for ppv in [a for a in actors if isinstance(a, unreal.PostProcessVolume)]:
    s = ppv.get_editor_property("settings")
    for k, v in (("auto_exposure_bias", math.log2(1.2)), ("bloom_threshold", 1.0), ("bloom_intensity", 1.0),
                 ("scene_fringe_intensity", 0.0), ("tone_curve_amount", 0.0), ("expand_gamut", 0.0), ("blue_correction", 0.0)):
        try:
            s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
        except Exception as e:
            log("PPV %s : %s" % (k, e))
    ppv.set_editor_property("settings", s)
    log("PPV %s réglé" % ppv.get_actor_label())

ok = les.save_current_level()
log("carte enregistrée : %s" % ok)
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_hub_bg.txt", "w", encoding="utf-8").write("\n".join(LOG))
