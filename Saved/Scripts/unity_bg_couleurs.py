# EON — calcule, pour chaque .mat Unity en BG.shadergraph, la couleur rendue en jeu (formule exacte du graphe)
# et écrit Saved/Scripts/eon_bg_params.json, lu par eon_bg_master.py dans l'éditeur.
# Dans SampleScene la lumière principale URP (Directional Light (1), (1;0,806;0,676) × 0,6) n'a pas d'ombre :
# d + shadowAtten ≥ 1 > Offset + Smoothness, donc la rampe vaut 1 partout -> la couleur ne dépend que du matériau.
import re, json, os, glob
ROOT = "D:/_PROJETS/briscard/GIT/EON_Project/Assets"
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_bg_params.json"
BG = "2fa49bf02ac334e49bba042c8d3cff39"
DEF = dict(Toon_Ramp_Smoothness=0.0, Toon_Ramp_Tint=(0.4906,) * 3, Toon_Ramp_Offset=0.5, Rim_Power=5.0, Brightness_Rim=2.0,
           _Smoothness=0.0, _Strength=0.0, _Mid_Threshold=0.0, _dark_threshold=0.0, _BC_1=(1, 1, 1), _triplanar=0, _scale_1=0.0, _blend=0.0, _smoothness=0.0)
s2l = lambda c: c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
LIGHT = tuple(s2l(c) * 0.6 for c in (1.0, 0.8058, 0.6761))   # couleur de lumière URP = linéaire × intensité

guid2path = {}
for meta in glob.glob(ROOT + "/**/*.meta", recursive=True):
    try:
        g = re.search(r"guid: (\w+)", open(meta, encoding="utf-8", errors="ignore").read(200))
    except Exception:
        continue
    if g:
        guid2path[g.group(1)] = meta[:-5].replace("\\", "/")


def parse(path):
    t = open(path, encoding="utf-8").read()
    if BG not in t:
        return None
    p = dict(DEF)
    for k, v in re.findall(r"- (\w+): ([-\d.e]+)\n", t):
        p[k] = float(v)
    for k, r, g, b in re.findall(r"- (\w+): \{r: ([-\d.e]+), g: ([-\d.e]+), b: ([-\d.e]+)", t):
        p[k] = (float(r), float(g), float(b))   # couleurs de .mat = espace gamma (linéarisées à l'usage, URP en linéaire)
    tex = {}
    for k, g in re.findall(r"- (\w+):\n\s+m_Texture: \{fileID: \d+, guid: (\w+)", t):
        tex[k] = guid2path.get(g, g)
    p["tex"] = tex
    return p


def smooth(a, b, x):
    if b == a:
        return 1.0 if x >= a else 0.0
    t = min(1, max(0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def couleur(p):
    tint = [s2l(c) for c in p["Toon_Ramp_Tint"]]
    toon = [LIGHT[i] * (1 + tint[i]) for i in range(3)]
    # trames vides : dark = noir, mid/light = blanc (valeurs par défaut du graphe) ; texture affectée = sa moyenne ~0,5
    tx = p["tex"]
    dk = 0.5 if "_dark_trame" in tx else 0.0
    md = 0.5 if "_mid_trame" in tx else 1.0
    lt = 0.5 if "_light_trame" in tx else 1.0
    K = []
    for c in toon:
        x = min(1, max(0, c))
        S = p["_Smoothness"]
        sd = smooth(p["_dark_threshold"] - S, p["_dark_threshold"] + S, x)
        sm = smooth(p["_Mid_Threshold"] - S, p["_Mid_Threshold"] + S, x)
        trame = (1 - sd) * dk + (sd - sm) * md + sm * lt
        K.append(c + (c * trame - c) * p["_Strength"])   # Blend Multiply, opacité Strength
    return toon, K


res = {}
for f in glob.glob(ROOT + "/**/*.mat", recursive=True):
    p = parse(f)
    if not p:
        continue
    toon, K = couleur(p)
    bc1 = [s2l(c) for c in p["_BC_1"]]
    name = os.path.splitext(os.path.basename(f))[0]
    res[name] = dict(K=K, BC1=bc1, BC=p["tex"].get("_BC"), Normal=p["tex"].get("_Normal"), RimPower=p["Rim_Power"], RimBright=p["Brightness_Rim"],
                     Triplanar=int(p["_triplanar"]), Scale1=p["_scale_1"], Blend=p["_blend"], tex=p["tex"])
    flat = [min(1, bc1[i] ** 2 * K[i]) for i in range(3)]
    srgb = [round(255 * (12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055)) for c in flat]
    print("%-18s K=(%.3f %.3f %.3f) aplat sRGB(BC blanc×BC_1)=%s  BC=%s" % (name, *K, srgb, os.path.basename(str(p["tex"].get("_BC")))))
json.dump(res, open(OUT, "w"), indent=1)
