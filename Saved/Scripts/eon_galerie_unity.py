# EON — carte de contrôle L_Galerie_Unity : copie de L_TestRoom + tous les meshes repris d'Unity,
# posés en rangées par famille autour du départ de Jira (mannequin = repère d'échelle 1,81 m).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_galerie_unity.py
import unreal

EAL = unreal.EditorAssetLibrary
SRC, DST = "/Game/EON/Maps/L_TestRoom", "/Game/EON/Maps/L_Galerie_Unity"
REPORT = []


def log(m):
    REPORT.append(m)
    unreal.log("EON_GAL " + m)


if EAL.does_asset_exist(DST):
    EAL.delete_asset(DST)
EAL.duplicate_asset(SRC, DST)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level(DST)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("Decor_X1", "Decor_X2"):
        eas.destroy_actor(a)

reg = unreal.AssetRegistryHelpers.get_asset_registry()
families = {}
for ad in reg.get_assets_by_path("/Game/EON/Unity", recursive=True):
    a = ad.get_asset()
    if not isinstance(a, (unreal.StaticMesh, unreal.SkeletalMesh)):
        continue
    path = str(ad.package_path)
    fam = path.split("/")[-1]
    families.setdefault(fam, []).append(a)

# Sol 32x20 tuiles = 3200 x 2000 cm, centré. Rangées le long de X, de y = -850 vers +850.
y = -850.0
for fam in sorted(families):
    items = sorted(families[fam], key=lambda a: a.get_name())
    sizes = []
    for a in items:
        b = a.get_bounds() if isinstance(a, unreal.StaticMesh) else a.get_bounds()
        ext = b.box_extent
        sizes.append((a, ext, b.origin))
        log("%-14s %-40s %6.0f x %6.0f x %6.0f cm" % (fam, a.get_name(), ext.x * 2, ext.y * 2, ext.z * 2))
    depth = max(min(e.y * 2, 600.0) for _, e, _ in sizes) + 80.0
    x = -1500.0
    for a, ext, org in sizes:
        w = min(ext.x * 2, 800.0) + 60.0
        if x + w > 1500.0:
            x = -1500.0
            y += depth
        loc = unreal.Vector(x + w * 0.5 - org.x, y + depth * 0.5 - org.y, -org.z + ext.z)
        if isinstance(a, unreal.StaticMesh):
            act = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc)
            if act:
                act.static_mesh_component.set_static_mesh(a)
        else:
            act = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, loc)
            act.skeletal_mesh_component.set_skinned_asset_and_update(a)
        if not act:
            log("POSE IMPOSSIBLE " + a.get_name())
            continue
        act.set_actor_label("U_" + a.get_name())
        act.set_folder_path("Unity/" + fam)
        x += w
    y += depth

log("rangées jusqu'à y = %.0f cm (sol : ±1000)" % y)
les.save_current_level()
out = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_galerie_unity.txt"
open(out, "w", encoding="utf-8").write("\n".join(REPORT))
