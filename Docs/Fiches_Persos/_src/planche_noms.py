# -*- coding: utf-8 -*-
# EON — noms posés sur la planche brute (Saved/Shots/Persos/PLANCHE_BRUTE.png), boîtes détectées automatiquement.
# 24/11, 2e planche : 6 personnages (Petite Nuke, Brainless, Méduse retirées), cadrage serré.
import json, numpy as np
from PIL import Image, ImageDraw, ImageFont
SRC = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Persos/PLANCHE_BRUTE.png"
DOC = "D:/_PROJETS/briscard/EON_UE/Docs/Fiches_Persos/"
im = Image.open(SRC).convert("RGB"); W, H = im.size
a = np.asarray(im).astype(float); lum = a.mean(axis=2)
# (nom, code, taille, x centre 4K, demi-largeur de recherche) : x lus sur la planche brute
P = json.load(open(DOC + "_src/planche_cols.json", encoding="utf-8"))
Y0, Y1 = P["y0"], P["y1"]
boxes = {}
for nom, code, t, cx, hw in P["persos"]:
    sub = lum[Y0:Y1, cx - hw:cx + hw] > P.get("seuil", 70)
    ys, xs = np.nonzero(sub)
    x0, x1 = cx - hw + xs.min(), cx - hw + xs.max(); y0, y1 = Y0 + ys.min(), Y0 + ys.max()
    boxes[code] = [int(x0), int(y0), int(x1), int(y1)]
    s = max(x1 - x0, y1 - y0, 240) * 1.15; mx, my = (x0 + x1) / 2, (y0 + y1) / 2
    im.crop((int(mx - s / 2), int(my - s / 2), int(mx + s / 2), int(my + s / 2))).resize((900, 900), Image.LANCZOS) \
        .save(DOC + "img/portrait_%s.png" % code)
json.dump(boxes, open(DOC + "img/boxes.json", "w"), indent=1)
F = "C:/Windows/Fonts/"
fT = ImageFont.truetype(F + "seguisb.ttf", 104); fN = ImageFont.truetype(F + "seguisb.ttf", 76); fS = ImageFont.truetype(F + "segoeui.ttf", 44)
d = ImageDraw.Draw(im)
d.text((W / 2, P["titre_y"]), "E.O.N  ·  Les personnages", font=fT, fill=(236, 226, 214), anchor="mm")
basey = max(b[3] for b in boxes.values()) + 90
for nom, code, t, cx, hw in P["persos"]:
    x0, y0, x1, y1 = boxes[code]; mx = (x0 + x1) / 2
    d.text((mx, basey), nom, font=fN, fill=(244, 236, 224), anchor="mm")
    d.text((mx, basey + 72), "%s  ·  %s" % (code, t), font=fS, fill=(176, 164, 150), anchor="mm")
im.save(DOC + "EON_Planche_Personnages_4K.png")
im.resize((1920, 1080), Image.LANCZOS).save(DOC + "img/planche_HD.png")
print(boxes)
