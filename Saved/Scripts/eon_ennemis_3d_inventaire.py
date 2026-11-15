# EON — inventaire des modèles 3D candidats pour les ennemis + import de Shooter_mode.fbx (Ikiryo).
import unreal, os

OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/ennemis_3d_inventaire.txt"
lines = []
def log(s):
    lines.append(s); unreal.log("INV " + s)

# 1. import du Shooter (Ikiryo), s'il n'est pas déjà là
DST = "/Game/EON/Ennemis/Ikiryo"
src = "D:/_PROJETS/briscard/EON_UE/Reference/3D_Studio/FBX/Chara/Shooter/Shooter_mode.fbx"
if not unreal.EditorAssetLibrary.does_directory_exist(DST) or not unreal.EditorAssetLibrary.list_assets(DST):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = DST; t.automated = True; t.save = True; t.replace_existing = True
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_as_skeletal = True; o.import_animations = False; o.import_materials = True; o.import_textures = True
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    t.options = o
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    log("import Shooter : %s" % list(t.imported_object_paths))

ar = unreal.AssetRegistryHelpers.get_asset_registry()
paths = ["/Game/EON/Unity/import/fbx/chara", "/Game/EON/Unity/import/fbx/xeno", "/Game/EON/Ennemis"]
anims = [a for a in ar.get_assets_by_path("/Game", recursive=True) if str(a.asset_class_path.asset_name) in ("AnimSequence",)]
for p in paths:
    for a in ar.get_assets_by_path(p, recursive=True):
        cls = str(a.asset_class_path.asset_name)
        if cls not in ("SkeletalMesh", "StaticMesh"):
            continue
        obj = a.get_asset()
        b = obj.get_bounds()
        ext = b.box_extent
        info = "%s [%s] taille %.0f x %.0f x %.0f cm" % (a.package_name, cls, ext.x * 2, ext.y * 2, ext.z * 2)
        if cls == "SkeletalMesh":
            sk = obj.skeleton
            mine = [str(x.package_name) for x in anims if x.get_asset().get_editor_property("skeleton") == sk] if sk else []
            info += " | squelette %s | anims %s" % (sk.get_name() if sk else None, mine)
            mats = [m.material_interface.get_name() if m.material_interface else None for m in obj.materials]
            info += " | mat %s" % mats
        log(info)
open(OUT, "w", encoding="utf-8").write("\n".join(lines))
