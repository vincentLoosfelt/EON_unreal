# EON — base color des ennemis en aplats (Vincent, 26/11 : « quantifier les couleurs pour ne plus avoir de dégradé.
# Fais-en des doublons »). Les originaux ne sont pas touchés : écrit SourceAssets/Ennemis/<X>/<nom>_Quant.png.
#   1. lissage médian (retire le grain avant le découpage) ;
#   2. k-means en Lab sur un échantillon -> palette de N couleurs (moyennes réelles de la texture) ;
#   3. chaque pixel prend la couleur de palette la plus proche, sans tramage ;
#   4. filtre de mode 5x5 : les pixels isolés aux frontières rejoignent leur aplat. Alpha gardé tel quel.
# Planche avant / après : Saved/Shots/EON_Quant_planche.jpg. Lancer : python eon_quantif_couleurs.py [N]
import glob, os, sys
import numpy as np
from PIL import Image, ImageFilter
from scipy.cluster.vq import kmeans2
from scipy import ndimage

N = int(sys.argv[1]) if len(sys.argv) > 1 else 12
RAC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
SRC = os.path.join(RAC, "SourceAssets", "Ennemis")
SHOT = os.path.join(RAC, "Saved", "Shots")


def lab(rgb):  # sRGB 0..255 -> Lab (D65)
    c = rgb / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    M = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    x = c @ M.T / np.array([0.95047, 1.0, 1.08883])
    f = np.where(x > 0.008856, np.cbrt(x), 7.787 * x + 16 / 116)
    return np.stack([116 * f[:, 1] - 16, 500 * (f[:, 0] - f[:, 1]), 200 * (f[:, 1] - f[:, 2])], 1)


def mode5(idx, n):  # vote majoritaire 5x5
    best = np.zeros(idx.shape, np.float32); out = idx.copy()
    for k in range(n):
        s = ndimage.uniform_filter((idx == k).astype(np.float32), 5)
        m = s > best; out[m] = k; best[m] = s[m]
    return out


vign = []
for f in sorted(glob.glob(os.path.join(SRC, "*", "*BaseColor.png"))):
    im = Image.open(f); a = np.asarray(im.convert("RGBA"))
    rgb = np.asarray(im.convert("RGB").filter(ImageFilter.MedianFilter(5))).reshape(-1, 3).astype(np.float64)
    alpha = a[..., 3]
    vis = (alpha.reshape(-1) > 8)
    nb = len(np.unique(rgb[vis][:, 0] * 65536 + rgb[vis][:, 1] * 256 + rgb[vis][:, 2]))
    n = min(N, nb)
    L = lab(rgb)
    # Échantillon pondéré par couleur (poids = surface^0,35) : une petite zone de teinte à part (bleus de l'Ubume)
    # garde sa place dans la palette au lieu d'être noyée par les grands aplats.
    q = (rgb[vis] // 6).astype(np.int64); cle = q[:, 0] * 65536 + q[:, 1] * 256 + q[:, 2]
    _, inv, cnt = np.unique(cle, return_inverse=True, return_counts=True)
    w = cnt[inv] ** -0.65; w /= w.sum()
    rng = np.random.default_rng(7)
    ech = np.flatnonzero(vis)[rng.choice(int(vis.sum()), min(200000, int(vis.sum())), replace=False, p=w)]
    cent, _ = kmeans2(L[ech], n, minit="++", iter=30, seed=7)
    # affectation par blocs (4096² = 16 M pixels)
    idx = np.empty(len(L), np.int32)
    for i in range(0, len(L), 1 << 20):
        d = ((L[i:i + (1 << 20), None, :] - cent[None]) ** 2).sum(2); idx[i:i + (1 << 20)] = d.argmin(1)
    idx = mode5(idx.reshape(alpha.shape), n).reshape(-1)
    # palette = médiane des pixels visibles seulement (le fond transparent noir ne compte pas) ; fond laissé tel quel
    pal = np.array([np.median(rgb[(idx == k) & vis], 0) if ((idx == k) & vis).any() else cent[k] * 0 for k in range(n)])
    out = np.where(vis[:, None], pal[idx], a[..., :3].reshape(-1, 3)).reshape(alpha.shape + (3,)).clip(0, 255).astype(np.uint8)
    res = Image.fromarray(np.dstack([out, alpha]) if im.mode == "RGBA" else out, im.mode if im.mode in ("RGB", "RGBA") else "RGB")
    dst = f[:-4] + "_Quant.png"
    res.save(dst)
    print("%-40s %5d couleurs -> %d  %s" % (os.path.basename(f), nb, n, " ".join("%02x%02x%02x" % tuple(int(v) for v in p) for p in pal)))
    v0 = im.convert("RGB").resize((512, 512), Image.LANCZOS); v1 = res.convert("RGB").resize((512, 512), Image.NEAREST)
    vign.append((os.path.basename(f)[2:-14], v0, v1, pal))

W = 512; H = 512 + 40
pl = Image.new("RGB", (W * 2 + 30, H * len(vign)), (24, 24, 28))
from PIL import ImageDraw
dr = ImageDraw.Draw(pl)
for i, (nom, v0, v1, pal) in enumerate(vign):
    y = i * H
    pl.paste(v0, (0, y + 30)); pl.paste(v1, (W + 30, y + 30))
    dr.text((6, y + 8), "%s : original" % nom, fill=(230, 230, 230))
    dr.text((W + 36, y + 8), "%s : %d couleurs" % (nom, len(pal)), fill=(230, 230, 230))
    for k, p in enumerate(pal):
        dr.rectangle([W + 250 + k * 22, y + 6, W + 268 + k * 22, y + 24], fill=tuple(int(v) for v in p), outline=(0, 0, 0))
os.makedirs(SHOT, exist_ok=True)
pl.save(os.path.join(SHOT, "EON_Quant_planche.jpg"), quality=88)
print("planche", os.path.join(SHOT, "EON_Quant_planche.jpg"))
