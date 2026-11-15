# EON — reprise de tous les meshes et textures du projet Unity (à lancer éditeur fermé) :
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_import_unity.py
# Source : D:/_PROJETS/briscard/GIT/EON_Project/Assets (lecture seule).
# Destination : /Game/EON/Unity/<même arborescence que Assets/>.
# Relançable : les assets existants sont remplacés.
import os, re, unreal

UNITY = "D:/_PROJETS/briscard/GIT/EON_Project/Assets"
DEST = "/Game/EON/Unity"
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
REPORT = []


def log(msg):
    REPORT.append(msg)
    unreal.log("EON_UNITY " + msg)


def dest_dir(rel_dir):
    parts = [re.sub(r"[^A-Za-z0-9_]", "_", p) for p in rel_dir.replace("\\", "/").split("/") if p]
    return "/".join([DEST] + parts)


def has_skin(path):
    with open(path, "rb") as f:
        data = f.read()
    return b"Deformer::" in data or b"SubDeformer" in data


def fbx_options(skeletal):
    o = unreal.FbxImportUI()
    o.automated_import_should_detect_type = False
    o.import_mesh = True
    o.import_textures = False   # les textures sont importées à part, toutes
    o.import_materials = True   # emplacements de matériaux ; remplis ensuite depuis les .mat Unity
    o.import_as_skeletal = skeletal
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
    o.import_animations = skeletal
    o.create_physics_asset = skeletal
    if not skeletal:
        o.static_mesh_import_data.combine_meshes = True
        o.static_mesh_import_data.generate_lightmap_u_vs = True
        o.static_mesh_import_data.auto_generate_collision = True
    return o


def task(src, dst, options=None):
    t = unreal.AssetImportTask()
    t.filename = src
    t.destination_path = dst
    t.automated = True
    t.replace_existing = True
    t.save = True
    if options:
        t.options = options
    return t


meshes, textures = [], []
for root, dirs, files in os.walk(UNITY):
    for f in files:
        full = os.path.join(root, f).replace("\\", "/")
        rel = os.path.relpath(root, UNITY).replace("\\", "/")
        ext = f.lower().rsplit(".", 1)[-1]
        if ext in ("fbx", "obj"):
            meshes.append((full, rel))
        elif ext in ("png", "tga", "jpg", "jpeg", "psd", "exr", "tif") and rel.startswith("import"):
            textures.append((full, rel))

log("trouvés : %d meshes, %d textures" % (len(meshes), len(textures)))

# --- Textures
tasks = [task(src, dest_dir(rel)) for src, rel in textures]
AT.import_asset_tasks(tasks)
ok_tex = 0
for t in tasks:
    for p in t.imported_object_paths:
        tex = unreal.load_asset(p)
        if isinstance(tex, unreal.Texture2D):
            ok_tex += 1
            n = tex.get_name().lower()
            if re.search(r"(normal|_nm$|_nml$|_n$|_nrm)", n):
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
                tex.set_editor_property("srgb", False)
            elif re.search(r"(metal|_m$|rough|_ao$|occlusion|mask|height)", n):
                tex.set_editor_property("srgb", False)
            EAL.save_loaded_asset(tex)
log("textures importées : %d / %d" % (ok_tex, len(textures)))

# --- Meshes (un par un : un FBX raté n'arrête pas les autres)
ok, failed = 0, []
for src, rel in meshes:
    skel = src.lower().endswith(".fbx") and has_skin(src)
    t = task(src, dest_dir(rel), fbx_options(skel) if src.lower().endswith(".fbx") else None)
    AT.import_asset_tasks([t])
    paths = list(t.imported_object_paths)
    if paths:
        ok += 1
        log("%s %s -> %d assets" % ("SK" if skel else "SM", os.path.relpath(src, UNITY), len(paths)))
    else:
        failed.append(os.path.relpath(src, UNITY))
        log("ECHEC " + os.path.relpath(src, UNITY))
log("meshes importés : %d / %d" % (ok, len(meshes)))

out = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Logs", "eon_import_unity.txt")
with open(out, "w", encoding="utf-8") as f:
    f.write("\n".join(REPORT))
