# EON - importe le pack « Old-School Shonen SFX » (Helton Yan, CC BY 4.0)
# Source : Audio_Sources/OldSchoolShonen/*.wav (hors Git)
# Destination : /Game/EON/Audio/OldSchoolShonen/<Famille>/SFX_Shonen_<Famille>_NN
import unreal, glob, os, re

SRC = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/OldSchoolShonen"
DST = "/Game/EON/Audio/OldSchoolShonen"
LOG = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/import_shonen.txt"

# préfixe du fichier (sans le numéro) -> famille
FAMILLES = {
    "ELECSprk_Anime Spark": "Spark",
    "EXPLDsgn_Anime Explosion": "Explosion",
    "FGHTBf_Anime Land": "Land",
    "FGHTClth_Anime Swing": "Swing",
    "FGHTImpt_Anime Melee Gore": "MeleeGore",
    "FGHTImpt_Anime Melee": "Melee",
    "FGHTMisc_Anime Dodge": "Dodge",
    "FGHTMisc_Anime Jump": "Jump",
    "FGHTMisc_Anime Throw": "Throw",
    "FRWKMisc_Anime Falling": "FallRocket",
    "TOONWhis_Anime Falling": "FallWhistle",
    "MAGSpel_Anime Ability Charge": "AbilityCharge",
    "MAGSpel_Anime Ability Ready": "AbilityReady",
    "MAGSpel_Anime Ability Release": "AbilityRelease",
    "SWSH_Anime Fly": "Fly",
}

tasks, lignes = [], []
for f in sorted(glob.glob(SRC + "/*.wav")):
    base = os.path.splitext(os.path.basename(f))[0]
    m = re.match(r"^(.*) (\d+)$", base)
    if not m or m.group(1) not in FAMILLES:
        lignes.append("IGNORE %s" % base)
        continue
    fam, num = FAMILLES[m.group(1)], int(m.group(2))
    t = unreal.AssetImportTask()
    t.filename = f
    t.destination_path = "%s/%s" % (DST, fam)
    t.destination_name = "SFX_Shonen_%s_%02d" % (fam, num)
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
