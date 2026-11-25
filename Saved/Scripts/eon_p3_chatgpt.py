# EON — planche P3 (traînées de sang au sol) depuis l'image générée par ChatGPT (25/11, conversation « Create VFX
# Sprite Sheet ») : fond magenta -> alpha, blanc -> ton vif 255, gris -> ton sombre 107 (comme eon_sang_planches.py),
# grille 2 × 4 recoupée et recadrée en cases 1024 × 512 (impact à gauche, gerbe vers la droite), planche 2048².
# Lancer : python eon_p3_chatgpt.py <image ChatGPT>   (PIL, numpy). Sortie : SourceAssets/VFX_Planches.
import sys, os
import numpy as np
from PIL import Image

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceAssets", "VFX_Planches")
src = Image.open(sys.argv[1]).convert("RGB")
a = np.asarray(src).astype(np.float32) / 255.0
r, g, b = a[..., 0], a[..., 1], a[..., 2]
# distance au magenta pur : le vert monte et le rouge/bleu restent hauts pour le blanc et le gris
mag = np.clip((np.minimum(r, b) - g) / 0.6, 0, 1)          # 1 = magenta pur
alpha = 1.0 - mag
lum = np.clip(g / np.maximum(alpha, 1e-3), 0, 1)            # luminance sans le magenta (canal vert)
ton = np.where(lum > 0.75, 255, 107).astype(np.uint8)
al8 = (np.clip((alpha - 0.15) / 0.7, 0, 1) * 255).astype(np.uint8)
H, W = al8.shape
CW, CH = 1024, 512
planche = Image.new("RGBA", (2 * CW, 4 * CH), (0, 0, 0, 0))
for row in range(4):
    for col in range(2):
        x0, x1 = W * col // 2, W * (col + 1) // 2
        y0, y1 = H * row // 4, H * (row + 1) // 4
        ca = al8[y0:y1, x0:x1]
        ys, xs = np.nonzero(ca > 40)
        if len(xs) == 0:
            continue
        bx0, bx1, by0, by1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
        g8 = ton[y0:y1, x0:x1][by0:by1, bx0:bx1]
        a8 = ca[by0:by1, bx0:bx1]
        cell = Image.merge("RGBA", [Image.fromarray(g8)] * 3 + [Image.fromarray(a8)])
        # impact à 4 % du bord gauche, bout de gerbe à 96 % ; hauteur au plus 90 % de la case, centrée
        k = min(CW * 0.92 / cell.width, CH * 0.9 / cell.height)
        cell = cell.resize((max(1, int(cell.width * k)), max(1, int(cell.height * k))), Image.LANCZOS)
        planche.paste(cell, (col * CW + int(CW * 0.04), row * CH + (CH - cell.height) // 2), cell)
planche.save(os.path.join(OUT, "EON_VFX_P3_TachesSol.png"))
src.save(os.path.join(OUT, "EON_VFX_P3_ChatGPT_source.png"))
# aperçu teinté rouge sur fond de sol
fond = Image.new("RGBA", planche.size, (150, 160, 140, 255))
p = np.asarray(planche).astype(np.float32)
t = p.copy(); t[..., 0] *= 214 / 255; t[..., 1] *= 18 / 255; t[..., 2] *= 38 / 255
fond.alpha_composite(Image.fromarray(t.astype(np.uint8)))
fond.convert("RGB").save(os.path.join(OUT, "EON_VFX_P3_TachesSol_apercu.png"))
print("ok", src.size)
