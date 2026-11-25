# EON — planche P4 (flash des tirs, ChatGPT 25/11) : 4 étoiles (2 à 4 branches, 2 à 6 branches) en 2 × 2.
# Fond magenta -> alpha (bord décontaminé du magenta), chaque étoile recentrée dans sa case de 512 px (le centre
# de l'étoile = le bout du canon). Sortie EON_VFX_P4_Flash.png (1024 × 1024, gris, teinté dans le jeu).
# Lancer : python eon_p4_alpha.py  (PIL, numpy)
import os
import numpy as np
from PIL import Image
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceAssets", "VFX_Planches")
im = np.asarray(Image.open(os.path.join(D, "EON_VFX_P4_Flash_ChatGPT_source.png")).convert("RGB")).astype(np.float32) / 255.0
r, g, b = im[..., 0], im[..., 1], im[..., 2]
mag = np.clip((np.minimum(r, b) - g) / 0.6, 0, 1)
a = np.clip((1.0 - mag - 0.08) / 0.84, 0, 1)
M = np.array([1.0, 0.0, 1.0], np.float32)
c = np.clip((im - (1 - a[..., None]) * M) / np.maximum(a[..., None], 0.05), 0, 1)
# gris pur (teinté dans le jeu) : luminance
l = c @ np.array([0.299, 0.587, 0.114], np.float32)
rgba = np.dstack([l, l, l, a])
H, W = a.shape
out = np.zeros((1024, 1024, 4), np.float32)
for cy in range(2):
    for cx in range(2):
        y0, y1, x0, x1 = cy * H // 2, (cy + 1) * H // 2, cx * W // 2, (cx + 1) * W // 2
        cell = rgba[y0:y1, x0:x1]
        ys, xs = np.nonzero(cell[..., 3] > 0.1)
        by0, by1, bx0, bx1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
        star = cell[by0:by1, bx0:bx1]
        sh, sw = star.shape[:2]
        k = min(1.0, 480.0 / max(sh, sw))   # marge de 16 px dans la case de 512
        pil = Image.fromarray((star * 255).astype(np.uint8), "RGBA").resize((max(1, int(sw * k)), max(1, int(sh * k))), Image.LANCZOS)
        st = np.asarray(pil).astype(np.float32) / 255.0
        h2, w2 = st.shape[:2]
        oy, ox = cy * 512 + (512 - h2) // 2, cx * 512 + (512 - w2) // 2
        out[oy:oy + h2, ox:ox + w2] = st
        print("case", cy, cx, "boite", (bx1 - bx0, by1 - by0), "echelle %.2f" % k)
Image.fromarray((out * 255).astype(np.uint8), "RGBA").save(os.path.join(D, "EON_VFX_P4_Flash.png"))
print("ok")
