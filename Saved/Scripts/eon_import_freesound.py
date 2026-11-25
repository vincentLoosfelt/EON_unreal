# EON - importe les sons Freesound CC0 retenus par Vincent (Audio_Sources/Freesound_CC0/Retenus)
# dans /Game/EON/Audio/Freesound. Crédits : Audio_Sources/Freesound_CC0/CREDITS.txt
import unreal, glob, os

SRC = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/Freesound_CC0/Retenus"
DST = "/Game/EON/Audio/Freesound"
LOG = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/import_freesound.txt"

tasks = []
for f in sorted(glob.glob(SRC + "/*.wav")):
    t = unreal.AssetImportTask()
    t.filename = f
    t.destination_path = DST
    t.destination_name = os.path.splitext(os.path.basename(f))[0]
    t.automated = True
    t.save = True
    t.replace_existing = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
ok = sum(1 for t in tasks if list(t.imported_object_paths))
lignes = ["IMPORTES %d / %d" % (ok, len(tasks))] + ["%s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)) for t in tasks]
open(LOG, "w", encoding="utf-8").write("\n".join(lignes))
