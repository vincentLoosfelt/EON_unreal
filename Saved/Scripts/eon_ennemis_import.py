# EON — import des ennemis du studio (exports blender_ennemis_export.py, SourceAssets/Ennemis/<Nom>) :
#   SK_<Nom> (maillage + squelette SKEL_<Nom>), A_<Nom>_<Anim>, T_<Nom>_*, MI_<Nom>[_Partie] (M_EON_ToonEnnemi + TP_EON_Anime,
#   comme Jira). Drone et écran du C.O.M en StaticMesh.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_ennemis_import.py
import unreal, os, glob
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, "SourceAssets", "Ennemis")
LOG = []
def log(s): LOG.append(s); unreal.log("EON_ENNEMIS " + s)

def task(fn, dest, name, opts):
    t = unreal.AssetImportTask()
    t.filename = fn; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True; t.options = opts
    AT.import_asset_tasks([t])
    return list(t.imported_object_paths)

def import_texture(fn, dest):
    name = os.path.splitext(os.path.basename(fn))[0]
    t = unreal.AssetImportTask()
    t.filename = fn; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    AT.import_asset_tasks([t])
    tex = unreal.load_asset(dest + "/" + name)
    if tex and ("Height" in name or "Displacement" in name):
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
        EAL.save_loaded_asset(tex)
    return tex

def sk_options(skel=None, anim=False):
    o = unreal.FbxImportUI()
    o.automated_import_should_detect_type = False
    o.import_as_skeletal = True
    o.import_materials = False
    o.import_textures = False
    if anim:
        o.import_mesh = False
        o.import_animations = True
        o.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        o.skeleton = skel
        o.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
        o.anim_sequence_import_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    else:
        o.import_mesh = True
        o.import_animations = False
        o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        o.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
    return o

def sm_options():
    o = unreal.FbxImportUI()
    o.automated_import_should_detect_type = False
    o.import_mesh = True; o.import_as_skeletal = False; o.import_animations = False
    o.import_materials = True; o.import_textures = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.set_editor_property("combine_meshes", True)
    o.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", False)
    return o

TP = unreal.load_asset("/Game/EON/Materials/TP_EON_Anime")
LIT = unreal.load_asset("/Game/EON/Materials/M_EON_ToonEnnemi")   # toon sans normal map (eon_ennemis_toon.py)

def make_mi(dest, name, tex):
    p = dest + "/" + name
    mi = unreal.load_asset(p) if EAL.does_asset_exist(p) else AT.create_asset(name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", LIT)
    if tex: MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", tex)
    if TP: unreal.EONStatics.set_instance_toon_profile(mi, TP)
    EAL.save_loaded_asset(mi)
    return mi

# Parties qui ont leur propre texture (nom de l'emplacement de matériau -> suffixe de texture)
PARTS = {"Ubume": ["Body", "Tentacles", "Glasses"]}

SEUL = os.environ.get("EON_SEUL")   # ex. EON_SEUL=Kama : réimporte ce seul ennemi (sans l'écran du C.O.M)
for nom in sorted(os.listdir(SRC)):
    if SEUL and nom != SEUL:
        continue
    d = os.path.join(SRC, nom)
    dest = "/Game/EON/Ennemis/" + nom
    texs = {}
    for fn in sorted(glob.glob(os.path.join(d, "T_*.png"))):
        t = import_texture(fn, dest + "/Textures")
        texs[os.path.splitext(os.path.basename(fn))[0]] = t
    sk_fbx = os.path.join(d, "SK_%s.fbx" % nom)
    sm_fbx = os.path.join(d, "SM_%s.fbx" % nom)
    if os.path.exists(sm_fbx):
        r = task(sm_fbx, dest, "SM_" + nom, sm_options())
        sm = unreal.load_asset(dest + "/SM_" + nom)
        log("%s : StaticMesh %s" % (nom, "ok, %d triangles" % sm.get_num_triangles(0) if sm else "ÉCHEC %s" % r))
        continue
    if not os.path.exists(sk_fbx):
        continue
    r = task(sk_fbx, dest, "SK_" + nom, sk_options())
    sk = unreal.load_asset(dest + "/SK_" + nom)
    if not isinstance(sk, unreal.SkeletalMesh):
        log("%s : SK ÉCHEC %s" % (nom, r)); continue
    skel = sk.get_editor_property("skeleton")
    # Squelette renommé SKEL_<Nom> (l'import le nomme SK_<Nom>_Skeleton)
    want = dest + "/SKEL_" + nom
    if skel.get_path_name().split(".")[0] != want:
        if EAL.does_asset_exist(want):
            EAL.delete_asset(want)
        EAL.rename_asset(skel.get_path_name().split(".")[0], want)
        skel = unreal.load_asset(want)
    # Matériaux
    mats = list(sk.get_editor_property("materials"))
    parts = PARTS.get(nom)
    main_tex = texs.get("T_%s_BaseColor" % nom) or (texs.get("T_%s_Body_BaseColor" % nom) if parts else None)
    mis = {}
    if parts:
        for p in parts:
            mis[p] = make_mi(dest, "MI_%s_%s" % (nom, p), texs.get("T_%s_%s_BaseColor" % (nom, p)))
    else:
        mis[""] = make_mi(dest, "MI_" + nom, main_tex)
    slots = []
    for s in mats:
        sn = str(s.get_editor_property("material_slot_name"))
        key = next((p for p in (parts or []) if p.lower() in sn.lower()), (parts[0] if parts else ""))
        s.set_editor_property("material_interface", mis[key])
        slots.append("%s->%s" % (sn, mis[key].get_name()))
    sk.set_editor_property("materials", mats)
    EAL.save_loaded_asset(sk)
    b = sk.get_bounds()
    log("%s : SK ok, %d os, hauteur %.0f cm, emplacements %s" % (nom, len(skel.get_editor_property("bone_tree")) if hasattr(skel, "bone_tree") else -1, b.box_extent.z * 2, slots))
    for fn in sorted(glob.glob(os.path.join(d, "A_*.fbx"))):
        an = os.path.splitext(os.path.basename(fn))[0]
        r = task(fn, dest + "/Anims", an, sk_options(skel, True))
        a = unreal.load_asset(dest + "/Anims/" + an)
        log("   %s : %s" % (an, "%.2f s" % a.get_play_length() if isinstance(a, unreal.AnimSequence) else "ÉCHEC %s" % r))

# Écran mural du C.O.M (module de salle)
if not SEUL:
  fn = os.path.join(ROOT, "SourceAssets", "Modules", "Com", "SM_Ecran_Com.fbx")
  r = task(fn, "/Game/EON/Meshes/COM", "SM_Ecran_Com", sm_options())
  sm = unreal.load_asset("/Game/EON/Meshes/COM/SM_Ecran_Com")
  log("Ecran_Com : %s" % ("ok, %d triangles, %d matériaux, taille %s" % (sm.get_num_triangles(0), len(sm.get_editor_property("static_materials")), sm.get_bounds().box_extent * 2) if sm else "ÉCHEC %s" % r))

EAL.save_directory("/Game/EON/Ennemis", False, True)
EAL.save_directory("/Game/EON/Meshes/COM", False, True)
open(os.path.join(ROOT, "Saved", "Logs", "eon_ennemis_import.txt"), "w", encoding="utf-8").write("\n".join(LOG))
