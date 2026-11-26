# Old-School Shonen : chaque WAV du pack contient 6 prises enchaînées. On les sépare (coupe dans les silences, aucun traitement du son).
import glob, json, os, re, shutil
import numpy as np, soundfile as sf
S = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/OldSchoolShonen/"
DST = S + "Prises/"
FAM = {"ELECSprk_Anime Spark": "Spark", "EXPLDsgn_Anime Explosion": "Explosion", "FGHTBf_Anime Land": "Land", "FGHTClth_Anime Swing": "Swing",
 "FGHTImpt_Anime Melee Gore": "MeleeGore", "FGHTImpt_Anime Melee": "Melee", "FGHTMisc_Anime Dodge": "Dodge", "FGHTMisc_Anime Jump": "Jump",
 "FGHTMisc_Anime Throw": "Throw", "FRWKMisc_Anime Falling": "FallRocket", "TOONWhis_Anime Falling": "FallWhistle",
 "MAGSpel_Anime Ability Charge": "AbilityCharge", "MAGSpel_Anime Ability Ready": "AbilityReady", "MAGSpel_Anime Ability Release": "AbilityRelease", "SWSH_Anime Fly": "Fly"}
shutil.rmtree(DST, ignore_errors=True)
groups = {}
for f in glob.glob(S + "*.wav"):
    m = re.match(r"^(.*) (\d+)$", os.path.splitext(os.path.basename(f))[0])
    groups.setdefault(FAM[m.group(1)], []).append((int(m.group(2)), f))
MAP, bad = {}, []
for fam, lst in sorted(groups.items()):
    os.makedirs(DST + fam, exist_ok=True); k = 0
    for num, f in sorted(lst):
        x, sr = sf.read(f, always_2d=True); info = sf.info(f)
        mono = x.mean(1); n = int(sr * 0.01)
        e = np.sqrt(np.convolve(mono * mono, np.ones(n) / n, "same")); on = np.flatnonzero(e > e.max() * 10 ** (-45 / 20))
        g = np.diff(on); cuts = sorted(np.argsort(g)[-5:])            # les 5 plus grands silences
        starts = [on[0]] + [on[c + 1] for c in cuts]; ends = [on[c] for c in cuts] + [on[-1]]
        if len(starts) != 6: bad.append(f)
        for a, b in zip(starts, ends):
            a = max(0, a - int(0.005 * sr)); b = min(len(x), b + int(0.05 * sr))
            k += 1; name = "SFX_Shonen_%s_%02d" % (fam, k)
            sf.write(DST + fam + "/" + name + ".wav", x[a:b], sr, subtype=info.subtype)
            MAP[name] = "%s %d, prise %d (%.2f s)" % (os.path.basename(f)[:-4], num, (k - 1) % 6 + 1, (b - a) / sr)
    print(fam, k)
json.dump(MAP, open(DST + "_correspondance.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
d = [float(re.search(r"\(([\d.]+) s\)", v).group(1)) for v in MAP.values()]
print("prises", len(MAP), "durée min %.2f max %.2f" % (min(d), max(d)), "anomalies", bad)
