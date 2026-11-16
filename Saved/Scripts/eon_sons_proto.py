# EON — rend en WAV des sons du proto (EON_v0.60.1.html, fonction sfx) : le proto les synthétise en Web Audio,
# ce script refait la même synthèse (tone : oscillateur à glissement exponentiel et gain qui décroît en exponentielle ;
# noise : bruit blanc filtré biquad, Q 1, fréquence de coupure qui glisse) et écrit Audio_Sources/Placeholders/SFX_Proto_<clé>.wav.
#   py -3 eon_sons_proto.py
import numpy as np, wave, math, os
SR = 44100
OUT = "D:/_PROJETS/briscard/EON_UE/Audio_Sources/Placeholders"
rng = np.random.default_rng(7)

def env_exp(n, v0, v1=0.001):
    return v0 * (v1 / v0) ** (np.arange(n) / max(1, n - 1))

def osc(kind, f0, f1, dur, vol):
    n = int(dur * SR); f = f0 * (max(20, f1) / f0) ** (np.arange(n) / max(1, n - 1))
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = {"sine": np.sin(ph), "square": np.sign(np.sin(ph)), "sawtooth": 2 * ((ph / (2 * np.pi)) % 1) - 1,
         "triangle": 2 * np.abs(2 * ((ph / (2 * np.pi)) % 1) - 1) - 1}[kind]
    return x * env_exp(n, vol)

def biquad(x, ftype, f0, f1, q=1.0):
    n = len(x); y = np.zeros(n); x1 = x2 = y1 = y2 = 0.0
    fr = f0 * (max(30, f1) / f0) ** (np.arange(n) / max(1, n - 1))
    for i in range(n):
        w = 2 * math.pi * fr[i] / SR; a = math.sin(w) / (2 * q); c = math.cos(w)
        if ftype == "bandpass": b0, b1, b2 = a, 0, -a
        elif ftype == "highpass": b0, b1, b2 = (1 + c) / 2, -(1 + c), (1 + c) / 2
        else: b0, b1, b2 = (1 - c) / 2, 1 - c, (1 - c) / 2
        a0, a1, a2 = 1 + a, -2 * c, 1 - a
        v = (b0 * x[i] + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2) / a0
        x2, x1, y2, y1 = x1, x[i], y1, v; y[i] = v
    return y

def noise(dur, vol, ftype, f0, f1):
    n = int(dur * SR)
    return biquad(rng.uniform(-1, 1, n), ftype, f0, f1) * env_exp(n, vol)

def mix(parts):
    n = max(int(d * SR) + len(p) for d, p in parts); out = np.zeros(n)
    for d, p in parts:
        k = int(d * SR); out[k:k + len(p)] += p
    return out

SONS = {
    "hiss": lambda: mix([(0, noise(1.2, 0.22, "bandpass", 2400, 700)), (0, osc("sine", 90, 60, 0.8, 0.12))]),
    "door": lambda: mix([(0, osc("triangle", 220, 440, 0.2, 0.15))]),
    "lift": lambda: mix([(i * 0.07, osc("triangle", f, f, 0.12, 0.1)) for i, f in enumerate([262, 330, 392, 523, 659])]),
}
for k, fn in SONS.items():
    x = fn(); x = x / max(1e-6, np.abs(x).max()) * 0.9
    p = os.path.join(OUT, "SFX_Proto_%s.wav" % k)
    with wave.open(p, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((x * 32767).astype("<i2").tobytes())
    print(p, round(len(x) / SR, 2), "s")
