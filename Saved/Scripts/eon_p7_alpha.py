# EON — planche P7 (onomatopées BAM ZAP BOOM / KRAK DOOM VRRM, ChatGPT 25/11) : fond magenta -> alpha, couleurs gardées
# (lettres blanches, ombre orange, contour noir), bord décontaminé du magenta. Sortie EON_VFX_P7_Onomatopees_alpha.png.
# Lancer : python eon_p7_alpha.py  (PIL, numpy)
import os
import numpy as np
from PIL import Image
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceAssets", "VFX_Planches")
im = np.asarray(Image.open(os.path.join(D, "EON_VFX_P7_Onomatopees.png")).convert("RGB")).astype(np.float32) / 255.0
r, g, b = im[..., 0], im[..., 1], im[..., 2]
mag = np.clip((np.minimum(r, b) - g) / 0.6, 0, 1)
a = np.clip((1.0 - mag - 0.08) / 0.84, 0, 1)
M = np.array([1.0, 0.0, 1.0], np.float32)
c = (im - (1 - a[..., None]) * M) / np.maximum(a[..., None], 0.05)
c = np.clip(c, 0, 1)
out = np.dstack([c, a]) * 255
Image.fromarray(out.astype(np.uint8), "RGBA").save(os.path.join(D, "EON_VFX_P7_Onomatopees_alpha.png"))
print("ok", im.shape)
