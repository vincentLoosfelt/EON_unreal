# EON — détoure une planche ChatGPT (fond magenta uni) en RGBA niveaux de gris, recadrée en grille exacte de cellules 512.
# Usage : python eon_detoure_planche.py <entrée.png> <sortie.png> <colonnes> <lignes>
# Alpha = 1 - « magenta-ité » (min(R, B) - V) ; la valeur grise vient du canal vert, divisé par l'alpha (bords sans rose).
import sys
import numpy as np
from PIL import Image
src, dst, cols, rows = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
im = np.asarray(Image.open(src).convert('RGB')).astype(np.float32) / 255
r, g, b = im[..., 0], im[..., 1], im[..., 2]
a = 1 - np.clip((np.minimum(r, b) - g - 0.25) / 0.35, 0, 1)
gris = np.where(a > 0.01, np.clip(g / np.maximum(a, 0.01), 0, 1), 0)
o = Image.fromarray((np.dstack([gris, gris, gris, a]) * 255).astype(np.uint8), 'RGBA')
H, W = a.shape
cw, ch = W / cols, H / rows
res = Image.new('RGBA', (cols * 512, rows * 512), (0, 0, 0, 0))
for i in range(cols * rows):
    c, l = i % cols, i // cols
    cell = o.crop((int(c * cw), int(l * ch), int((c + 1) * cw), int((l + 1) * ch)))
    s = min(512 / cell.width, 512 / cell.height)
    cell = cell.resize((int(cell.width * s), int(cell.height * s)), Image.LANCZOS)
    res.paste(cell, (c * 512 + (512 - cell.width) // 2, l * 512 + (512 - cell.height) // 2))
res.save(dst)
