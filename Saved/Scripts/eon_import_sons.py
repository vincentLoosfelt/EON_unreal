# EON — importe les WAV de Audio_Sources/Placeholders dans /Game/EON/Audio/Placeholders.
import unreal, glob, os
src = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/Placeholders"
tasks = []
for f in sorted(glob.glob(src + "/*.wav")):
    t = unreal.AssetImportTask()
    t.filename = f; t.destination_path = "/Game/EON/Audio/Placeholders"; t.automated = True; t.save = True; t.replace_existing = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
L = []
for t in tasks:
    L.append("%s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/import_sons.txt", "w", encoding="utf-8").write("\n".join(L))
