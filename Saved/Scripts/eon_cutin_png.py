# EON — JIR-V23 (24/11) : capture HDR brute (eon_cutin_jira.py) -> PNG détouré du buste de Jira pour le cut-in.
# Alpha = 1 - alpha de la capture (« inverse opacity »). Exposition calée sur Jira, courbe douce, sRGB.
import json, sys, numpy as np
from PIL import Image
D = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/CutIn/"
N = 1024


def load(name):
    a = np.array(json.load(open(D + name + ".json")), dtype=np.float32).reshape(N, N, 4)
    rgb, al = np.maximum(a[..., :3], 0), np.clip(1 - a[..., 3], 0, 1)
    return rgb, al


rgb, al = load("saine")
lum = rgb.mean(-1)[al > 0.5]
ex = 0.75 / max(np.percentile(lum, 92), 1e-6)


def png(rgb, al):
    x = rgb * ex
    x = x / (1 + 0.35 * x)
    x = np.where(x <= 0.0031308, 12.92 * x, 1.055 * np.power(np.clip(x, 0, 1), 1 / 2.4) - 0.055)
    out = np.dstack([np.clip(x, 0, 1) * 255, al * 255]).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


ys, xs = np.where(al > 0.1)
y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
yb = int(y0 + (y1 - y0) * 0.58)          # buste : tête -> hanches
box = (max(0, x0 - 8), max(0, y0 - 8), min(N, x1 + 8), min(N, yb))
im = png(rgb, al)
im.save(D + "jira_entiere.png")
b = im.crop(box)
s = 512 / b.size[1]
b = b.resize((max(1, int(b.size[0] * s)), 512), Image.LANCZOS)
b.save(D + "T_Jira_CutIn.png")
rv, av = load("veines")
png(rv, av).save(D + "jira_veines.png")
print("expo %.3f, boite %s, buste %s" % (ex, (x0, y0, x1, y1), b.size))
