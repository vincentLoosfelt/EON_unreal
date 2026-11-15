# EON — le vrai Jira (cyborg aux cheveux longs) dans Unreal, à lancer éditeur fermé :
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_jira_cyborg.py
# Avant : supprimer Content/EON/Characters/Jira sur disque (le commandlet ne remplace pas proprement).
# Source : Saved/Jira/Jira_rigged.fbx, produit par blender_rig_jira.py (Cyborg_mode.fbx skinné sur l'armature
# des animations Jira_*, 15 actions recuites).
# 1. Import SK_Jira + squelette + animations Jira_* ; textures T_Cyborg_V2.
# 2. Matériaux : maître M_EON_ToonMasked (Substrate Toon, masqué, double face : mèches de cheveux en cartes),
#    MI_Jira_Corps / MI_Jira_Cheveux avec le profil toon TP_EON_Anime (style perso, comme l'AnimeShader d'Unity).
# 3. Locomotion : ABP_Unarmed du mannequin retargeté sur Jira, puis ses idle / marche / course remplacés
#    par les animations de Jira (consolidation : toutes les références pointent vers Jira_idle/_walk/_run).
#    Saut / chute / atterrissage restent ceux du mannequin retargetés (Jira n'en a pas).
import os, unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
P = unreal.MaterialProperty
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DIR = "/Game/EON/Characters/Jira"
TEXDIR = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/Textures/Textures_Chara/T_Cyborg_V2/"
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_JIRA " + str(m))


def task(src, dst, options=None):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dst
    t.automated = True; t.replace_existing = True; t.save = True
    if options:
        t.options = options
    return t


# --- 1. Import
o = unreal.FbxImportUI()
o.automated_import_should_detect_type = False
o.import_mesh = True
o.import_textures = False
o.import_materials = True
o.import_as_skeletal = True
o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
o.import_animations = True
o.create_physics_asset = False
o.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
o.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
o.anim_sequence_import_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
t = task(PROJ + "Saved/Jira/Jira_rigged.fbx", DIR, o)
AT.import_asset_tasks([t])
paths = list(t.imported_object_paths)
log("importés : %d" % len(paths))
mesh, anims = None, {}
for p in paths:
    a = unreal.load_asset(p)
    if isinstance(a, unreal.SkeletalMesh):
        mesh = a
    elif isinstance(a, unreal.AnimSequence):
        anims[a.get_name()] = a
log("mesh : %s ; anims : %s" % (mesh.get_path_name() if mesh else None, sorted(anims)))
if mesh.get_name() != "SK_Jira":
    EAL.rename_asset(mesh.get_path_name().split(".")[0], DIR + "/SK_Jira")
    mesh = unreal.load_asset(DIR + "/SK_Jira")
skel = mesh.get_editor_property("skeleton")
if skel.get_name() != "SKEL_Jira":
    EAL.rename_asset(skel.get_path_name().split(".")[0], DIR + "/SKEL_Jira")
    skel = unreal.load_asset(DIR + "/SKEL_Jira")
# Anims : nom final Jira_<prise>, rangées dans Anims/Jira
A = {}
for name, a in anims.items():
    take = "Jira_" + name.split("Jira_")[-1]
    dst = DIR + "/Anims/Jira/" + take
    if a.get_path_name().split(".")[0] != dst:
        EAL.rename_asset(a.get_path_name().split(".")[0], dst)
    A[take] = unreal.load_asset(dst)
log("anims rangées : %s" % sorted(A))
for k in ("Jira_idle", "Jira_walk", "Jira_run", "Jira_hacking", "Jira_death"):
    if k in A:
        log("  %s : %.2f s" % (k, A[k].get_play_length()))

tt = [task(TEXDIR + f, DIR + "/Textures") for f in ("T_Cyborg_BaseColor.png", "T_Cyborg_BaseColor_2.png")]
AT.import_asset_tasks(tt)
TEX = {}
for x in tt:
    for p in x.imported_object_paths:
        tex = unreal.load_asset(p); TEX[tex.get_name()] = tex
log("textures : %s" % sorted(TEX))

# --- 2. Matériaux
MDIR = "/Game/EON/Materials"
mp = MDIR + "/M_EON_ToonMasked"
if not EAL.does_asset_exist(mp):
    m = AT.create_asset("M_EON_ToonMasked", MDIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("opacity_mask_clip_value", 0.5)

    def node(cls, x, y, **kw):
        n = MEL.create_material_expression(m, cls, x, y)
        for k, v in kw.items():
            n.set_editor_property(k, v)
        return n

    def mul(a, ao, b, bo, x, y):
        n = node(unreal.MaterialExpressionMultiply, x, y)
        MEL.connect_material_expressions(a, ao, n, "A"); MEL.connect_material_expressions(b, bo, n, "B")
        return n

    def con(a, ao, b, names):
        for nm in names:
            if MEL.connect_material_expressions(a, ao, b, nm):
                return nm
        log("ÉCHEC connexion vers %s" % names)

    tp_bg = unreal.load_asset(MDIR + "/TP_EON_BG")
    toon = node(unreal.MaterialExpressionSubstrateToonBSDF, 0, 0, toon_profile=tp_bg)
    tex = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, -400, parameter_name="BaseTex",
               texture=unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    tint = node(unreal.MaterialExpressionVectorParameter, -900, -200, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    log("base -> %s" % con(mul(tex, "RGB", tint, "", -600, -300), "", toon, ["Base Color", "BaseColor"]))
    log("rough -> %s" % con(node(unreal.MaterialExpressionScalarParameter, -600, 300, parameter_name="Roughness", default_value=1.0), "", toon, ["Roughness"]))
    log("spec -> %s" % con(node(unreal.MaterialExpressionScalarParameter, -600, 400, parameter_name="Specular", default_value=0.0), "", toon, ["Specular"]))
    log("front -> %s" % MEL.connect_material_property(toon, "", P.MP_FRONT_MATERIAL))
    log("masque -> %s" % MEL.connect_material_property(tex, "A", P.MP_OPACITY_MASK))
    m.set_editor_property("used_with_skeletal_mesh", True)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
master = unreal.load_asset(mp)

tint = (1.0, 0.80146426, 0.50628924)     # Toon_Ramp_Tint de l'AnimeShader d'Unity (Captain.mat)
tp = unreal.load_asset(MDIR + "/TP_EON_Anime")


def make_mi(name, tex):
    mi = AT.create_asset(name, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", master)
    MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", tex)
    MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*[(1 + c) / 1.49 for c in tint], 1.0))
    log("%s : profil toon %s" % (name, unreal.EONStatics.set_instance_toon_profile(mi, tp)))
    EAL.save_loaded_asset(mi)
    return mi


mi_corps = make_mi("MI_Jira_Corps", TEX["T_Cyborg_BaseColor"])
mi_chev = make_mi("MI_Jira_Cheveux", TEX["T_Cyborg_BaseColor_2"])
mats = list(mesh.get_editor_property("materials"))
for s in mats:
    old = s.get_editor_property("material_interface")
    slot = str(s.get_editor_property("material_slot_name"))
    mi = mi_chev if "001" in slot else mi_corps
    s.set_editor_property("material_interface", mi)
    log("emplacement %s (%s) -> %s" % (slot, old.get_name() if old else None, mi.get_name()))
    if old and old.get_path_name().startswith(DIR):
        EAL.delete_asset(old.get_path_name().split(".")[0])
mesh.set_editor_property("materials", mats)
EAL.save_loaded_asset(mesh)

# --- 3. Retarget de la locomotion du mannequin
quinn = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")


def make_rig(name, m):
    rig = AT.create_asset(name, DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(m)
    c.apply_auto_generated_retarget_definition()
    return rig, c


rq, cq = make_rig("IK_Quinn", quinn)
EAL.save_loaded_asset(rq)
rj, cj = make_rig("IK_Jira", mesh)
for ch in list(cj.get_retarget_chains()):
    cj.remove_retarget_chain(ch.get_editor_property("chain_name"))
cj.set_retarget_root("CTRL_pelvis")      # « . » des os Blender -> « _ » à l'import
CH = [("Spine", "CTRL_spine", "CTRL_chest"), ("Neck", "CTRL_neck", "CTRL_neck"), ("Head", "CTRL_head", "CTRL_head"),
      ("LeftClavicle", "clavicle_L", "clavicle_L"), ("LeftArm", "upper_arm_L", "hand_L"),
      ("RightClavicle", "clavicle_R", "clavicle_R"), ("RightArm", "upper_arm_R", "canon_R"),
      ("LeftLeg", "leg_up_L", "foot_L"), ("LeftFoot", "toes_L", "toes_L"),
      ("RightLeg", "leg_up_R", "foot_R"), ("RightFoot", "toes_R", "toes_R"),
      ("LeftThumb", "thumb_01_L", "thumb_02_L")]
for f in ("Index", "Middle", "Ring", "Pinky"):
    CH.append(("Left" + f, "%s_01_L" % f.lower(), "%s_03_L" % f.lower()))
for name, a, b in CH:
    if not cj.add_retarget_chain(name, a, b, ""):
        log("chaîne refusée : %s %s->%s" % (name, a, b))
log("IK_Jira : racine=%s, %d chaînes" % (cj.get_retarget_root(), len(cj.get_retarget_chains())))
EAL.save_loaded_asset(rj)

rtg = AT.create_asset("RTG_Quinn_Jira", DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET
rc.set_ik_rig(S, rq); rc.set_ik_rig(T, rj)
rc.set_preview_mesh(S, quinn); rc.set_preview_mesh(T, mesh)
try:
    rc.add_default_ops(); rc.assign_ik_rig_to_all_ops(S, rq); rc.assign_ik_rig_to_all_ops(T, rj)
except Exception as e:
    log("ops : %s" % e)
try:
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
except Exception as e:
    log("auto_map : %s" % e)
try:
    rc.auto_align_all_bones(T); log("alignement auto : ok")
except Exception as e:
    log("auto_align : %s" % e)
EAL.save_loaded_asset(rtg)

abp = EAL.find_asset_data("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed")
res = unreal.IKRetargetBatchOperation.duplicate_and_retarget([abp], quinn, mesh, rtg, "", "", "", "_Jira", DIR + "/Anims", False, True, True)
log("retargetés : %d" % len(res))
EAL.save_directory(DIR, False, True)

# --- 4. Idle / marche / course : les animations de Jira remplacent celles du mannequin
reg = unreal.AssetRegistryHelpers.get_asset_registry()
repl = {"Jira_idle": [], "Jira_walk": [], "Jira_run": []}
for ad in reg.get_assets_by_path(DIR + "/Anims", recursive=False):
    n = str(ad.asset_name)
    if n.startswith("MM_Idle"):
        repl["Jira_idle"].append(ad.get_asset())
    elif n.startswith("MF_Unarmed_Walk"):
        repl["Jira_walk"].append(ad.get_asset())
    elif n.startswith("MF_Unarmed_Jog"):
        repl["Jira_run"].append(ad.get_asset())
for k, olds in repl.items():
    if k in A and olds:
        ok = EAL.consolidate_assets(A[k], olds)
        log("%s remplace %d anims du mannequin : %s" % (k, len(olds), ok))
EAL.save_directory(DIR, False, True)
for ad in reg.get_assets_by_path(DIR, recursive=True):
    log("  %s" % ad.package_name)
open(PROJ + "Saved/Logs/eon_jira_cyborg.txt", "w", encoding="utf-8").write("\n".join(LOG))
