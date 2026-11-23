# -*- coding: utf-8 -*-
# EON — étude colorimétrique fond / celluloïds : Orbitals (captures de l'article Unreal) contre notre HUB.
# Zones relevées à la main (coordonnées 1920×1080). Sortie : JSON + texte dans Docs/Rendu/Etude_Orbitals_Couleur.
import json, sys
import numpy as np
from PIL import Image
ORB = sys.argv[1]   # dossier des captures Orbitals
EON = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/DecorPeint/CARTE_pres.png"
OUT = "D:/_PROJETS/briscard/EON_UE/Docs/Rendu/Etude_Orbitals_Couleur/"


def lab(rgb):
    c = rgb / 255.0
    c = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    M = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    xyz = c @ M.T / np.array([0.9505, 1.0, 1.089])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    L = 116 * f[..., 1] - 16; a = 500 * (f[..., 0] - f[..., 1]); b = 200 * (f[..., 1] - f[..., 2])
    return L, np.hypot(a, b), np.degrees(np.arctan2(b, a)) % 360


IM = {
    "Orbitals_couloir": dict(f=ORB + "05.png",
        cel=[(585, 380, 650, 500), (630, 160, 740, 225), (1080, 450, 1230, 540), (1060, 620, 1190, 700)],
        boites=[(570, 120, 855, 975), (1035, 300, 1335, 885)]),
    "Orbitals_chat": dict(f=ORB + "07.png",
        cel=[(940, 640, 1080, 770), (800, 425, 890, 470), (900, 495, 1040, 535)],
        boites=[(765, 158, 1380, 900)]),
    "Orbitals_salle": dict(f=ORB + "13.png",
        cel=[(322, 705, 450, 840), (930, 720, 1050, 840), (1140, 675, 1260, 765), (930, 870, 1050, 945)],
        boites=[(285, 645, 510, 960), (915, 630, 1072, 960), (1118, 638, 1282, 900)]),
    "EON_HUB": dict(f=EON,
        cel=[(420, 330, 480, 435), (945, 225, 1020, 345), (1425, 225, 1485, 375), (615, 570, 675, 675), (1320, 555, 1440, 660)],
        boites=[(225, 180, 630, 780), (495, 480, 795, 788), (765, 165, 1200, 570), (1035, 585, 1230, 750), (1260, 142, 1612, 795)]),
}
for k in ("P0", "P1", "P2", "P3"):
    IM["EON_" + k] = dict(IM["EON_HUB"], f="D:/_PROJETS/briscard/EON_UE/Saved/Shots/DecorPeint/Etalonnage/%s_pres.png" % k)
del IM["EON_HUB"]
R = {}
for nom, z in IM.items():
    im = np.asarray(Image.open(z["f"]).convert("RGB").resize((1920, 1080), Image.LANCZOS)).astype(np.float32)
    L, C, H = lab(im)
    fond = np.ones(L.shape, bool)
    for x0, y0, x1, y1 in z["boites"]:
        fond[y0:y1, x0:x1] = False
    cel = np.zeros(L.shape, bool)
    for x0, y0, x1, y1 in z["cel"]:
        cel[y0:y1, x0:x1] = True
    boite = ~fond
    r = dict(fond_L=float(L[fond].mean()), fond_L_p10=float(np.percentile(L[fond], 10)), fond_L_p90=float(np.percentile(L[fond], 90)),
             fond_C=float(C[fond].mean()), cel_L=float(L[cel].mean()), cel_C=float(C[cel].mean()),
             trait_L=float(np.percentile(L[boite], 2)),
             fond_teinte=float(np.degrees(np.arctan2(np.sin(np.radians(H[fond])) @ C[fond], np.cos(np.radians(H[fond])) @ C[fond])) % 360))
    r["ecart_L"] = r["cel_L"] - r["fond_L"]; r["ratio_C"] = r["cel_C"] / max(r["fond_C"], 1e-3)
    R[nom] = r
json.dump(R, open(OUT + "mesures.json", "w"), indent=1)
for n, r in R.items():
    print("%-18s fond L %5.1f (p10 %4.1f p90 %4.1f) C %5.1f teinte %3.0f | cel L %5.1f C %5.1f | ecart L %+5.1f ratio C %.2f | trait L %4.1f"
          % (n, r["fond_L"], r["fond_L_p10"], r["fond_L_p90"], r["fond_C"], r["fond_teinte"], r["cel_L"], r["cel_C"], r["ecart_L"], r["ratio_C"], r["trait_L"]))
