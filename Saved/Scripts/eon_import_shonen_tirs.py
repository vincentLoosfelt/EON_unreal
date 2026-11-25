# EON - importe les tirs taillés dans Old-School Shonen (Audio_Sources/OldSchoolShonen/Tirs/Retenus)
# SFX_Shonen_<Fam>_NN -> /Game/EON/Audio/OldSchoolShonen/<Fam>/
import unreal, glob, os
SRC = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/OldSchoolShonen/Tirs/Retenus"
LOG = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/import_shonen_tirs.txt"
tasks = []
for f in sorted(glob.glob(SRC + "/SFX_Shonen_*.wav")):
    n = os.path.splitext(os.path.basename(f))[0]
    t = unreal.AssetImportTask()
    t.filename = f
    t.destination_path = "/Game/EON/Audio/OldSchoolShonen/" + n.split("_")[2]
    t.destination_name = n
    t.automated = True; t.save = True; t.replace_existing = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
ok = sum(1 for t in tasks if list(t.imported_object_paths))
open(LOG, "w", encoding="utf-8").write("\n".join(["IMPORTES %d / %d" % (ok, len(tasks))] + ["%s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)) for t in tasks]))
