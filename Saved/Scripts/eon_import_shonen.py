# EON - importe le pack « Old-School Shonen SFX » (Helton Yan, CC BY 4.0)
# Source : Audio_Sources/OldSchoolShonen/Prises (hors Git)
# Destination : /Game/EON/Audio/OldSchoolShonen/<Famille>/SFX_Shonen_<Famille>_NN
import unreal, glob, os

# 26/11 : chaque WAV du pack contient 6 prises enchaînées ; on importe les prises séparées
# (Audio_Sources/OldSchoolShonen/Prises/<Famille>/SFX_Shonen_<Famille>_NN.wav, faites par Saved/Scripts/eon_split_shonen.py ;
# correspondance prise -> fichier d'origine dans Prises/_correspondance.json).
SRC = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/OldSchoolShonen/Prises"
DST = "/Game/EON/Audio/OldSchoolShonen"
LOG = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/import_shonen.txt"

tasks, lignes = [], []
for f in sorted(glob.glob(SRC + "/*/SFX_Shonen_*.wav")):
    fam = os.path.basename(os.path.dirname(f))
    t = unreal.AssetImportTask()
    t.filename = f
    t.destination_path = "%s/%s" % (DST, fam)
    t.destination_name = os.path.splitext(os.path.basename(f))[0]
    t.automated = True
    t.save = True
    t.replace_existing = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
ok = 0
for t in tasks:
    p = list(t.imported_object_paths)
    ok += 1 if p else 0
    lignes.append("%s -> %s" % (os.path.basename(t.filename), p))
lignes.insert(0, "IMPORTES %d / %d" % (ok, len(tasks)))
open(LOG, "w", encoding="utf-8").write("\n".join(lignes))
