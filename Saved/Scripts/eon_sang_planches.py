# EON — planches de sang P2 (giclée, 8 images) et P3 retirée le 25/11 : elle vient de ChatGPT (eon_p3_chatgpt.py), dessinées par script (Vincent, 25/11).
# ChatGPT refuse le sang : on le dessine ici, d'après Akira (9:51–9:55) :
#   - aplats à 2 tons (vif + sombre), SANS trait ;
#   - taches déchiquetées en pointes, pas de gouttes rondes ;
#   - traînées effilées en flamme dans le sens du tir ;
#   - flaque sombre à bord irrégulier.
# Sorties en RGBA, fond transparent, niveaux de gris : blanc = ton vif, gris 0,42 = ton sombre. Le jeu multiplie par la
# couleur du monstre (Kama rouge, Nuke ocre, Ubume violette, Ikiryo orange). Aperçus teintés en rouge Akira à côté.
# Lancer : python eon_sang_planches.py  (PIL). Sortie : SourceAssets/VFX_Planches.
import math, random, os
from PIL import Image, ImageDraw

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceAssets", "VFX_Planches")
SS = 4                       # suréchantillonnage (bords nets mais lissés)
CELL = 512
VIF, SOMBRE = 255, 107       # niveaux de gris des 2 tons
ROUGE = (214, 18, 38)        # aperçu : rouge Akira


def blot(d, cx, cy, r, rng, val, pointes=7, longues=0.35):
    """Tache déchiquetée : contour irrégulier + quelques longues pointes."""
    n = rng.randint(14, 22)
    pts = []
    a0 = rng.random() * math.tau
    for i in range(n):
        a = a0 + i / n * math.tau + rng.uniform(-0.12, 0.12)
        k = rng.uniform(0.7, 1.05)
        if rng.random() < longues:
            k = rng.uniform(1.15, 1.55)   # pointes courtes : du liquide, pas une étoile
        pts.append((cx + math.cos(a) * r * k, cy + math.sin(a) * r * k * 0.85))
        # creux entre deux points : pointes plus franches
        a2 = a + math.tau / n * 0.5
        k2 = rng.uniform(0.45, 0.75)
        pts.append((cx + math.cos(a2) * r * k2, cy + math.sin(a2) * r * k2 * 0.85))
    d.polygon(pts, fill=val)
    return pts


def flamme(d, x, y, ang, long_, larg, rng, val, ondule=0.25):
    """Traînée effilée : tête arrondie vers l'avant (sens du mouvement), queue pointue et ondulée vers l'arrière."""
    ca, sa = math.cos(ang), math.sin(ang)
    def P(u, v):  # u le long (avant > 0), v en travers
        return (x + ca * u - sa * v, y + sa * u + ca * v)
    haut, bas = [], []
    N = 18
    ph = rng.random() * math.tau
    for i in range(N + 1):
        t = i / N                      # 0 = tête, 1 = bout de la queue
        u = larg * 0.5 - t * long_
        w = larg * 0.5 * (1 - t) ** 0.8 if t > 0.15 else larg * 0.5 * math.sqrt(max(0.0, 1 - ((0.15 - t) / 0.15) ** 2) * 0.6 + 0.4)
        o = math.sin(t * 5 + ph) * larg * ondule * t
        haut.append(P(u, o + w)); bas.append(P(u, o - w))
    d.polygon(haut + bas[::-1], fill=val)


def eclat(d, x, y, r, rng, val):
    """Petite projection : triangle ou losange irrégulier."""
    n = rng.choice([3, 4, 4, 5])
    a0 = rng.random() * math.tau
    d.polygon([(x + math.cos(a0 + i / n * math.tau) * r * rng.uniform(0.5, 1.4),
                y + math.sin(a0 + i / n * math.tau) * r * rng.uniform(0.5, 1.4)) for i in range(n)], fill=val)


def deux_tons(dessin, taille, rng_seed, decal=0.009):
    """Forme dessinée une fois ; le ton vif = la forme moins sa bande bas-droite (ombre franche d'Akira, pas un liseré).
    Les formes marquées « sombre » (flaque) restent entièrement sombres."""
    from PIL import ImageChops
    w, h = taille
    a = Image.new("L", (w, h), 0)
    sombre = Image.new("L", (w, h), 0)
    dessin(ImageDraw.Draw(a), ImageDraw.Draw(sombre), random.Random(rng_seed))
    d = int(w * decal)
    dec = ImageChops.offset(a, -d, -d)
    vif = ImageChops.subtract(ImageChops.multiply(a, dec), sombre)
    return ImageChops.lighter(a, sombre), vif


def cellule(alpha, vif):
    """Assemble alpha + 2 tons en RGBA niveaux de gris."""
    from PIL import ImageChops
    vif = ImageChops.multiply(vif, alpha)
    g = Image.new("L", alpha.size, SOMBRE)
    g.paste(VIF, mask=vif)
    return Image.merge("RGBA", (g, g, g, alpha))


# ---------- P2 : giclée, 8 images, part du bord gauche-centre vers la droite ----------
# Cône de ±35° : au début les paquets se chevauchent en une masse déchiquetée, puis ils s'écartent, s'étirent en flammes
# et retombent en se réduisant (la gravité tire vers le bas de l'image, vue de 3/4).
def p2_image(f):
    W = CELL * SS
    sx, sy = W * 0.20, W * 0.50
    t = f / 7.0
    rng0 = random.Random(77)
    parts = []
    for i in range(46):
        parts.append(dict(a=rng0.gauss(-0.08, 0.28), v=rng0.uniform(0.25, 1.0) ** 0.7, r=rng0.uniform(0.35, 1.0),
                          flamme=rng0.random() < 0.45, g=rng0.uniform(0.6, 1.4)))

    def dessin(d, ds, rng):
        if f == 0:
            blot(d, sx + W * 0.05, sy, W * 0.075, rng, 255, longues=0.6)
            for i in range(6):
                a = rng.uniform(-0.6, 0.5)
                eclat(d, sx + math.cos(a) * W * 0.17, sy + math.sin(a) * W * 0.13, W * 0.014, rng, 255)
            return
        T = 1 - (1 - t) ** 1.6                      # vitesse forte au départ puis freinée
        for p in parts:
            dist = W * 0.78 * p["v"] * T
            x = sx + W * 0.04 + math.cos(p["a"]) * dist
            y = sy + math.sin(p["a"]) * dist * 0.75 + (t ** 2) * p["g"] * W * 0.10 * p["v"]
            r = W * 0.05 * p["r"] * (1.25 - 0.85 * t) * (1.2 - 0.5 * p["v"])
            if r < W * 0.005 or x > W * 0.97 or y > W * 0.97:
                continue
            vx, vy = math.cos(p["a"]), math.sin(p["a"]) * 0.75 + t * p["g"] * 0.35
            ang = math.atan2(vy, vx)
            if p["flamme"] and t > 0.1:
                flamme(d, x, y, ang, r * (3.5 + 3.5 * (1 - t)), r * 1.4, rng, 255)
            else:
                blot(d, x, y, r, rng, 255, longues=0.25 if t < 0.5 else 0.12)
        # masse encore attachée à l'impact au début
        if t < 0.5:
            blot(d, sx + W * 0.07, sy, W * 0.07 * (1 - 2 * t) + W * 0.02, rng, 255, longues=0.5)
        # fines projections
        for i in range(int(14 + 16 * t)):
            a = rng.uniform(-0.8, 0.6); v = rng.uniform(0.4, 1.1)
            dist = W * 0.85 * v * T
            x = sx + math.cos(a) * dist; y = sy + math.sin(a) * dist * 0.75 + t * t * W * 0.2 * v
            if x < W * 0.97 and y < W * 0.97:
                eclat(d, x, y, W * rng.uniform(0.005, 0.012), rng, 255)

    a, v = deux_tons(dessin, (W, W), 1000 + f)
    return cellule(a, v).resize((CELL, CELL), Image.LANCZOS)


def planche(images, cols, nom):
    rows = (len(images) + cols - 1) // cols
    cw, ch = images[0].size
    p = Image.new("RGBA", (cols * cw, rows * ch), (0, 0, 0, 0))
    for i, im in enumerate(images):
        p.paste(im, ((i % cols) * cw, (i // cols) * ch))
    p.save(os.path.join(OUT, nom + ".png"))
    # aperçu teinté (rouge Akira) sur fond de sol gris-vert
    fond = Image.new("RGBA", p.size, (150, 160, 140, 255))
    r, g, b, al = p.split()
    teinte = Image.merge("RGBA", (r.point(lambda x: x * ROUGE[0] // 255), g.point(lambda x: x * ROUGE[1] // 255),
                                  b.point(lambda x: x * ROUGE[2] // 255), al))
    fond.alpha_composite(teinte)
    fond.convert("RGB").save(os.path.join(OUT, nom + "_apercu.png"))


planche([p2_image(f) for f in range(8)], 4, "EON_VFX_P2_Giclee")
print("ok")
