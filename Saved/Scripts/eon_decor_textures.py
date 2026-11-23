# -*- coding: utf-8 -*-
# EON — textures du décor peint (Vincent, 23/11 : « des coups de pinceau, une brush texture qui change la façon dont les
# ombres se diffusent ; 3 teintes en triplanar avec des trames de papier ou de diffusion aquarelle »).
# Raccordables (bruits périodiques, coups de pinceau enroulés sur les bords). Python 3 hors Unreal :
#   py -3 Saved/Scripts/eon_decor_textures.py
# Sorties (Saved/Generated/DecorPeint, importées par eon_decor_peint.py) :
#   T_EON_Pinceau.png  R = valeur du coup de pinceau (aplat par coup, sert à déformer les seuils d'ombre)
#                      G = poils / brosse sèche (cassures des liserés)   B = couverture du coup (bord)
#   T_EON_Aquarelle.png R = taches de lavis   G = auréole (pigment accumulé au bord de la tache)   B = granulation
import os
import numpy as np
from PIL import Image

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Generated", "DecorPeint")
os.makedirs(OUT, exist_ok=True)
N = 1024
rng = np.random.default_rng(23112026)
yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)


def bruit(freq_lo, freq_hi, aniso=(1.0, 1.0), seed=None):
    """Bruit périodique filtré en fréquence (bande freq_lo..freq_hi en cycles par tuile), normalisé 0..1."""
    r = np.random.default_rng(seed)
    f = np.fft.fft2(r.standard_normal((N, N)))
    fy = np.fft.fftfreq(N)[:, None] * N / aniso[1]
    fx = np.fft.fftfreq(N)[None, :] * N / aniso[0]
    k = np.sqrt(fx * fx + fy * fy)
    filt = np.exp(-(k / freq_hi) ** 2) * (1 - np.exp(-(k / max(freq_lo, 1e-3)) ** 2))
    v = np.real(np.fft.ifft2(f * filt))
    v -= v.min(); v /= v.max()
    return v.astype(np.float32)


def smooth(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


# ---------- coups de pinceau ----------
val = bruit(1, 3, seed=1) * 0.5 + 0.25          # fond : sous-couche
poil = np.full((N, N), 0.5, np.float32)
couv = np.zeros((N, N), np.float32)
grain_bord = bruit(20, 60, seed=2)
for i in range(150):
    cx, cy = rng.uniform(0, N, 2)
    ang = rng.normal(0.0, 0.45) + (np.pi / 2 if rng.random() < 0.3 else 0.0)   # surtout couchés, quelques verticaux
    L = rng.uniform(160, 440); w = rng.uniform(40, 110); bend = rng.normal(0, 0.0006)
    v0 = np.clip(rng.normal(0.5, 0.2), 0.05, 0.95)
    dx = (xx - cx + N / 2) % N - N / 2; dy = (yy - cy + N / 2) % N - N / 2
    t = dx * np.cos(ang) + dy * np.sin(ang)
    s = -dx * np.sin(ang) + dy * np.cos(ang) - bend * t * t
    u = t / L + 0.5                                     # 0 = départ chargé, 1 = fin sèche
    taper = np.clip(np.minimum(u * 6, (1 - u) * 3), 0, 1) ** 0.5
    half = w * 0.5 * (0.75 + 0.25 * taper)
    sn = s / np.maximum(half, 1e-3)
    bristle_phase = rng.uniform(0, 1000)
    br = 0.5 + 0.5 * np.sin(sn * rng.uniform(9, 16) + np.sin(sn * 31 + bristle_phase) * 1.3)   # poils parallèles au coup
    m = (np.abs(sn) < 1.0) & (u > 0) & (u < 1)
    edge = smooth(1.0, 0.75 - 0.25 * grain_bord, np.abs(sn))
    dry = smooth(0.15, 0.0, (u - 0.55) * 1.2 - br * 0.6)             # brosse sèche vers la fin du coup
    a = np.where(m, edge * taper.clip(0.3, 1) * np.maximum(dry, 0.25), 0).astype(np.float32)
    val = val * (1 - a) + (v0 + (br - 0.5) * 0.06) * a
    poil = poil * (1 - a) + br * a
    couv = np.maximum(couv, a)
pinceau = np.stack([val, poil, couv], -1)
Image.fromarray((np.clip(pinceau, 0, 1) * 255).astype(np.uint8), "RGB").save(os.path.join(OUT, "T_EON_Pinceau.png"))

# ---------- aquarelle : taches de lavis + auréoles + granulation ----------
n1 = bruit(1.5, 5, seed=3) * 0.8 + bruit(6, 18, seed=4) * 0.2
lavis = np.zeros((N, N), np.float32); aur = np.zeros((N, N), np.float32)
for th in (0.42, 0.58, 0.7):
    tache = smooth(th, th + 0.015, n1)
    lavis += tache / 3
    aur = np.maximum(aur, np.exp(-((n1 - th - 0.012) / 0.012) ** 2) * (0.6 + 0.4 * bruit(30, 80, seed=int(th * 100))))
gran = bruit(60, 200, seed=7) * 0.6 + bruit(8, 30, seed=8) * 0.4
aqua = np.stack([lavis, aur, gran], -1)
Image.fromarray((np.clip(aqua, 0, 1) * 255).astype(np.uint8), "RGB").save(os.path.join(OUT, "T_EON_Aquarelle.png"))
print("ok", OUT)
