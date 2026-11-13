# EON — génération des assets du jalon 0 (à lancer éditeur fermé) :
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_setup_j0.py
# Relançable : chaque asset est recréé à l'identique depuis les JSON de la passation.
import json, os, unreal

PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
PASS = os.path.join(PROJ, "Docs", "PASSATION_UNREAL")
DONNEES = os.path.join(PASS, "DONNEES")
PIX = os.path.join(PASS, "PLACEHOLDERS_PIXEL")

AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
LOG = []


def log(msg):
    LOG.append(msg)
    unreal.log("EON_SETUP " + msg)


def load_json(name):
    with open(os.path.join(DONNEES, name), encoding="utf-8") as f:
        return json.load(f)


def fresh(path):
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)


def hex_lin(h, a=1.0):
    h = h.lstrip("#")
    c = [int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    lin = [x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4 for x in c]
    return {"R": lin[0], "G": lin[1], "B": lin[2], "A": a}


def lin_color(h):
    d = hex_lin(h)
    return unreal.LinearColor(d["R"], d["G"], d["B"], 1.0)


# ---------------------------------------------------------------- DataTables
def make_table(name, struct_path, rows):
    path = "/Game/EON/Data/" + name
    if EAL.does_asset_exist(path):
        dt = unreal.load_asset(path)  # déjà référencée : on la remplit à nouveau
    else:
        fac = unreal.DataTableFactory()
        fac.struct = unreal.load_object(None, struct_path)
        dt = AT.create_asset(name, "/Game/EON/Data", unreal.DataTable, fac)
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json.dumps(rows))
    EAL.save_loaded_asset(dt)
    log("%s : %d lignes (import %s)" % (name, len(rows), ok))


def cap(k):
    return k[0].upper() + k[1:]


def tables():
    feel = {k: v for k, v in load_json("reglages_gamefeel.json").items() if not k.startswith("_")}
    sec = {"shake": 0, "hitstop": 0, "knock": 0.25, "hitFlash": False, "dmgNums": False, "recoil": 0,
           "flash": False, "shells": False, "camKick": 0, "lookahead": 0, "crt": False}
    mx = {"shake": 2, "hitstop": 120, "knock": 2.2, "hitFlash": True, "dmgNums": True, "recoil": 2.5,
          "flash": True, "shells": True, "camKick": 2.6, "lookahead": 40, "crt": True}
    rows = []
    for name, over in (("studio", {}), ("eon", {}), ("sec", sec), ("max", mx)):
        r = {cap(k): v for k, v in dict(feel, **over).items()}
        r["Name"] = name
        rows.append(r)
    make_table("DT_GameFeel", "/Script/EON.EONGameFeelRow", rows)

    g = load_json("greffes.json")
    order = g["ordre_jira"]
    rows = []
    for key, v in g["greffes"].items():
        r = {"Name": key, "DisplayName": v["name"], "Type": v["type"],
             "JiraOrder": order.index(key) + 1 if key in order else 0}
        for k, val in v.items():
            if k in ("name", "type"):
                continue
            if k == "col":
                r["Col"] = hex_lin(val)
            elif k == "min":
                r["MinShots"] = val
            elif k == "max":
                r["MaxShots"] = val
            else:
                r[cap(k)] = val
        rows.append(r)
    make_table("DT_Greffes", "/Script/EON.EONGreffeRow", rows)

    e = load_json("ennemis.json")
    rows = []
    for key, v in e["ennemis"].items():
        r = {"Name": key, "OfficialName": e["noms_officiels"].get(key, key), "Adn": e["adn"].get(key, 0)}
        for k, val in v.items():
            if k in ("contact", "fly"):
                r[cap(k)] = bool(val)
            elif k in ("hh", "hr"):
                r[k.upper()] = val
            else:
                r[cap(k)] = val
        rows.append(r)
    make_table("DT_Ennemis", "/Script/EON.EONEnemyRow", rows)

    with open(os.path.join(PIX, "_index.json"), encoding="utf-8") as f:
        idx = json.load(f)["sprites"]
    rows = [{"Name": k, "FW": v["fw"], "FH": v["fh"], "N": v["n"]} for k, v in idx.items()]
    make_table("DT_Sprites", "/Script/EON.EONSpriteRow", rows)


# ---------------------------------------------------------------- Sprites
def sprites():
    tasks = []
    for fn in sorted(os.listdir(PIX)):
        if not fn.lower().endswith(".png"):
            continue
        t = unreal.AssetImportTask()
        t.filename = os.path.join(PIX, fn)
        t.destination_path = "/Game/EON/Placeholders"
        t.destination_name = "T_" + os.path.splitext(fn)[0]
        t.replace_existing = True
        t.automated = True
        t.save = False
        tasks.append(t)
    AT.import_asset_tasks(tasks)
    n = 0
    for t in tasks:
        for p in t.imported_object_paths:
            tex = unreal.load_asset(p)
            if not isinstance(tex, unreal.Texture2D):
                continue
            tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
            tex.set_editor_property("srgb", True)
            tex.set_editor_property("never_stream", True)
            EAL.save_loaded_asset(tex)
            n += 1
    log("sprites importés : %d" % n)


# ---------------------------------------------------------------- Matériaux
def new_material(name):
    path = "/Game/EON/Materials/" + name
    fresh(path)
    return AT.create_asset(name, "/Game/EON/Materials", unreal.Material, unreal.MaterialFactoryNew())


def expr(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def scalar(mat, name, default, x, y):
    e = expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def vector(mat, name, color, x, y):
    e = expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", color)
    return e


def pixel_sprite_material():
    m = new_material("M_PixelSprite")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("opacity_mask_clip_value", 0.5)

    uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    mr = expr(m, unreal.MaterialExpressionComponentMask, -1200, -60)
    mr.set_editor_property("r", True); mr.set_editor_property("g", False)
    mg = expr(m, unreal.MaterialExpressionComponentMask, -1200, 120)
    mg.set_editor_property("r", False); mg.set_editor_property("g", True)
    MEL.connect_material_expressions(uv, "", mr, "")
    MEL.connect_material_expressions(uv, "", mg, "")

    inv = expr(m, unreal.MaterialExpressionOneMinus, -1000, -20)
    MEL.connect_material_expressions(mr, "", inv, "")
    flip = scalar(m, "FlipX", 0.0, -1000, 60)
    lerp = expr(m, unreal.MaterialExpressionLinearInterpolate, -820, -60)
    MEL.connect_material_expressions(mr, "", lerp, "A")
    MEL.connect_material_expressions(inv, "", lerp, "B")
    MEL.connect_material_expressions(flip, "", lerp, "Alpha")

    frame = scalar(m, "Frame", 0.0, -820, 80)
    add = expr(m, unreal.MaterialExpressionAdd, -640, -20)
    MEL.connect_material_expressions(lerp, "", add, "A")
    MEL.connect_material_expressions(frame, "", add, "B")
    frames = scalar(m, "Frames", 1.0, -640, 100)
    div = expr(m, unreal.MaterialExpressionDivide, -480, -20)
    MEL.connect_material_expressions(add, "", div, "A")
    MEL.connect_material_expressions(frames, "", div, "B")
    app = expr(m, unreal.MaterialExpressionAppendVector, -320, 40)
    MEL.connect_material_expressions(div, "", app, "A")
    MEL.connect_material_expressions(mg, "", app, "B")

    tex = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -140, 40)
    tex.set_editor_property("parameter_name", "Tex")
    default_tex = unreal.load_asset("/Game/EON/Placeholders/T_jira_a0")
    if default_tex:
        tex.set_editor_property("texture", default_tex)
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    MEL.connect_material_expressions(app, "", tex, "UVs")

    tint = vector(m, "Tint", unreal.LinearColor(1, 1, 1, 1), -140, 260)
    bright = scalar(m, "Brightness", 1.0, -140, 380)
    mul = expr(m, unreal.MaterialExpressionMultiply, 120, 40)
    MEL.connect_material_expressions(tex, "RGB", mul, "A")
    MEL.connect_material_expressions(tint, "", mul, "B")
    mul2 = expr(m, unreal.MaterialExpressionMultiply, 280, 40)
    MEL.connect_material_expressions(mul, "", mul2, "A")
    MEL.connect_material_expressions(bright, "", mul2, "B")
    MEL.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    log("M_PixelSprite créé")


def flat_material():
    m = new_material("M_Flat")
    col = vector(m, "Color", unreal.LinearColor(0.2, 0.2, 0.2, 1), -400, 0)
    rough = scalar(m, "Roughness", 0.8, -400, 200)
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)

    t = new_material("M_TileFloor")
    wp = expr(t, unreal.MaterialExpressionWorldPosition, -900, 0)
    cus = expr(t, unreal.MaterialExpressionCustom, -500, 0)
    cus.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    cus.set_editor_property("code", (
        "float2 p = WP.xy / 100.0;\n"
        "float2 t = floor(p);\n"
        "float h = frac(sin(dot(t, float2(12.9898, 78.233))) * 43758.5453);\n"
        "float3 c = h < 0.6 ? C0 : (h < 0.85 ? C1 : C2);\n"
        "float2 f = frac(p);\n"
        "float edge = step(f.x, 0.03) + step(f.y, 0.03);\n"
        "return lerp(c, G, saturate(edge));"))
    inputs = []
    for n in ("WP", "C0", "C1", "C2", "G"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    cus.set_editor_property("inputs", inputs)
    MEL.connect_material_expressions(wp, "", cus, "WP")
    for i, n in enumerate(("C0", "C1", "C2", "G")):
        v = vector(t, n, unreal.LinearColor(0.1, 0.1, 0.1, 1), -800, 200 + i * 160)
        MEL.connect_material_expressions(v, "", cus, n)
    rough = scalar(t, "Roughness", 0.85, -500, 300)
    MEL.connect_material_property(cus, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(t)
    EAL.save_loaded_asset(t)
    log("M_Flat et M_TileFloor créés")


def material_instance(name, parent_path, vectors):
    path = "/Game/EON/Materials/" + name
    fresh(path)
    fac = unreal.MaterialInstanceConstantFactoryNew()
    mi = AT.create_asset(name, "/Game/EON/Materials", unreal.MaterialInstanceConstant, fac)
    mi.set_editor_property("parent", unreal.load_asset(parent_path))
    for k, v in vectors.items():
        MEL.set_material_instance_vector_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


# ---------------------------------------------------------------- Entrées
def inputs():
    folder = "/Game/EON/Input"
    def ia(name, vt):
        path = folder + "/" + name
        fresh(path)
        a = AT.create_asset(name, folder, unreal.InputAction, unreal.InputAction_Factory())
        a.set_editor_property("value_type", vt)
        EAL.save_loaded_asset(a)
        return a

    B = unreal.InputActionValueType.BOOLEAN
    A2 = unreal.InputActionValueType.AXIS2D
    acts = {
        "IA_Move": (A2, []), "IA_Aim": (A2, []),
        "IA_Fire": (B, ["LeftMouseButton", "Gamepad_RightTriggerAxis"]),
        "IA_Dash": (B, ["SpaceBar", "LeftShift", "Gamepad_FaceButton_Bottom", "Gamepad_LeftTriggerAxis"]),
        "IA_Spell": (B, ["E", "RightMouseButton", "Gamepad_RightShoulder", "Gamepad_LeftShoulder"]),
        "IA_Holo": (B, ["F", "Gamepad_FaceButton_Left"]),
        "IA_Ultimate": (B, ["R", "Gamepad_FaceButton_Top"]),
        "IA_Interact": (B, ["C", "Enter", "Gamepad_FaceButton_Right"]),
        "IA_Map": (B, ["Tab", "M", "Gamepad_Special_Left"]),
        "IA_Pause": (B, ["P", "Escape", "Gamepad_Special_Right"]),
        "IA_GreffeNext": (B, ["Gamepad_DPad_Right"]),
        "IA_GreffePrev": (B, ["Gamepad_DPad_Left"]),
    }
    for i, k in enumerate(["One", "Two", "Three", "Four", "Five"]):
        acts["IA_Greffe%d" % (i + 1)] = (B, [k])
    objs = {n: ia(n, vt) for n, (vt, _) in acts.items()}

    path = folder + "/IMC_EON"
    fresh(path)
    imc = AT.create_asset("IMC_EON", folder, unreal.InputMappingContext, unreal.InputMappingContext_Factory())

    def key(name):
        k = unreal.Key()
        k.import_text(name)
        return k

    def swz():
        s = unreal.InputModifierSwizzleAxis()
        s.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
        return s

    neg = unreal.InputModifierNegate
    # Déplacement : X = droite, Y = haut de l'écran. ZQSD / WASD / flèches / stick gauche.
    move = objs["IA_Move"]
    plan = []
    for k in ("W", "Z", "Up"):
        plan.append((move, k, [swz()]))
    for k in ("S", "Down"):
        plan.append((move, k, [swz(), neg()]))
    for k in ("D", "Right"):
        plan.append((move, k, []))
    for k in ("A", "Q", "Left"):
        plan.append((move, k, [neg()]))
    dz = unreal.InputModifierDeadZone()
    dz.set_editor_property("lower_threshold", 0.18)
    plan.append((move, "Gamepad_Left2D", [dz]))
    plan.append((objs["IA_Aim"], "Gamepad_Right2D", []))
    for n, (vt, keys) in acts.items():
        for k in keys:
            plan.append((objs[n], k, []))

    for a, k, mods in plan:
        imc.map_key(a, key(k))
    # Les modificateurs se posent sur la liste des mappings puis on la réécrit.
    prop, maps = None, None
    for cand in ("default_key_mappings", "mappings"):
        try:
            v = imc.get_editor_property(cand)
            if cand == "default_key_mappings":
                v = v.get_editor_property("mappings")
            if len(v) > 0:
                prop, maps = cand, v
                break
        except Exception as ex:
            log("mappings %s : %s" % (cand, ex))
    if maps is None or len(maps) != len(plan):
        log("ATTENTION : %s mappings lus pour %d posés" % (None if maps is None else len(maps), len(plan)))
    for i, (a, k, mods) in enumerate(plan):
        if mods and maps is not None:
            maps[i].set_editor_property("modifiers", mods)
    if prop == "mappings":
        imc.set_editor_property("mappings", maps)
    elif prop == "default_key_mappings":
        dkm = imc.get_editor_property("default_key_mappings")
        dkm.set_editor_property("mappings", maps)
        imc.set_editor_property("default_key_mappings", dkm)
    EAL.save_loaded_asset(imc)
    log("entrées : %d actions, %d touches (propriété %s)" % (len(objs), len(plan), prop))


# ---------------------------------------------------------------- Blueprints
def blueprint(name, parent_cls):
    folder = "/Game/EON/Blueprints"
    fresh(folder + "/" + name)
    fac = unreal.BlueprintFactory()
    fac.set_editor_property("parent_class", parent_cls)
    bp = AT.create_asset(name, folder, unreal.Blueprint, fac)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp)
    return bp


def blueprints():
    jira = blueprint("BP_Jira", unreal.EONCharacter)
    pc = blueprint("BP_EONPlayerController", unreal.EONPlayerController)
    gm = blueprint("BP_EONGameMode", unreal.EONGameMode)
    cdo = unreal.get_default_object(gm.generated_class())
    cdo.set_editor_property("default_pawn_class", jira.generated_class())
    cdo.set_editor_property("player_controller_class", pc.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(gm)
    EAL.save_loaded_asset(gm)
    log("Blueprints : BP_Jira, BP_EONPlayerController, BP_EONGameMode")


# ---------------------------------------------------------------- Salle de test
def test_room():
    sal = load_json("salles.json")
    pal = sal["palettes"][0]  # Coursive (étage 1)
    floor_mi = material_instance("MI_Sol_Coursive", "/Game/EON/Materials/M_TileFloor", {
        "C0": lin_color(pal["f"][0]), "C1": lin_color(pal["f"][1]),
        "C2": lin_color(pal["f"][2]), "G": lin_color(pal["g"])})
    wall_mi = material_instance("MI_Mur_Coursive", "/Game/EON/Materials/M_Flat", {"Color": lin_color(pal["w"][1])})
    neon_mi = material_instance("MI_Neon_Coursive", "/Game/EON/Materials/M_Flat", {"Color": lin_color(pal["neon"])})

    path = "/Game/EON/Maps/L_TestRoom"
    fresh(path)
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(path)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")

    def box(label, cx, cy, sx, sy, sz, mi, z0=0.0):
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, z0 + sz * 50.0))
        a.static_mesh_component.set_static_mesh(cube)
        a.set_actor_scale3d(unreal.Vector(sx, sy, sz))
        a.static_mesh_component.set_material(0, mi)
        a.set_actor_label(label)
        a.set_folder_path("Salle")
        return a

    W, H = 32, 20  # tuiles, murs compris (salle standard)
    box("Sol", 0, 0, W, H, 0.1, floor_mi, z0=-10.0)
    hw, hh = W * 50.0, H * 50.0
    wall_h = 2.5
    box("Mur_Nord", 0, -hh + 50, W, 1, wall_h, wall_mi)
    box("Mur_Sud", 0, hh - 50, W, 1, wall_h, wall_mi)
    box("Mur_Ouest", -hw + 50, 0, 1, H - 2, wall_h, wall_mi)
    box("Mur_Est", hw - 50, 0, 1, H - 2, wall_h, wall_mi)
    # Repères : bandes néon le long du mur nord, deux blocs de décor (tuiles X).
    box("Neon_Nord", 0, -hh + 101, W - 2, 0.04, 0.06, neon_mi, z0=180.0)
    box("Decor_X1", -600, 200, 2, 2, 1.2, wall_mi)
    box("Decor_X2", 700, -300, 1, 3, 1.2, wall_mi)

    room = eas.spawn_actor_from_class(unreal.EONRoomBounds, unreal.Vector(0, 0, 0))
    room.set_editor_property("size_tiles", unreal.IntPoint(W, H))
    room.set_actor_label("Salle_32x20")
    eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100)).set_actor_label("Depart_Jira")

    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 800), unreal.Rotator(-20, -60, -50))
    sun.light_component.set_editor_property("intensity", 3.0)
    sun.light_component.set_editor_property("light_color", unreal.Color(255, 236, 220, 255))
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 600))
    sc = sky.light_component
    # Ombre jamais noire : ambiance teintée à la couleur "amb" de la palette.
    sc.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_CAPTURED_SCENE)
    sc.set_editor_property("lower_hemisphere_is_black", False)
    amb = lin_color(pal["amb"])
    k = 1.0 / max(amb.r, amb.g, amb.b)
    sc.set_editor_property("lower_hemisphere_color", unreal.LinearColor(amb.r * k, amb.g * k, amb.b * k, 1))
    sc.set_editor_property("intensity", 0.8)
    sky.set_actor_label("Ambiance_Teintee")

    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    s.set_editor_property("override_auto_exposure_method", True)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    s.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    s.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("auto_exposure_bias", 0.0)
    ppv.set_editor_property("settings", s)
    ppv.set_actor_label("PP_Exposition_Fixe")

    ws = unreal.EditorLevelLibrary.get_editor_world().get_world_settings()
    ws.set_editor_property("default_game_mode", unreal.load_asset("/Game/EON/Blueprints/BP_EONGameMode").generated_class())
    les.save_current_level()
    log("L_TestRoom : salle 32x20, palette %s" % pal["n"])


import sys
STEPS = [a for a in sys.argv[1:] if not a.startswith("-")]
for step in (tables, sprites, pixel_sprite_material, flat_material, inputs, blueprints, test_room):
    if STEPS and step.__name__ not in STEPS:
        continue
    try:
        step()
    except Exception as ex:
        log("ÉCHEC %s : %s" % (step.__name__, ex))

with open(os.path.join(PROJ, "Saved", "Scripts", "eon_setup_j0.log"), "w", encoding="utf-8") as f:
    f.write("\n".join(LOG))
