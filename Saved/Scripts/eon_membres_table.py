# EON — table des membres coupables de tous les ennemis (Vincent, 25/11 : « produire cet effet pour TOUS les ennemis »).
# Lit Saved/os_<nom>.txt et Saved/verts_<nom>.txt (eon_membres_dump.py) et mesure chaque membre sur ses sommets :
#   axe = articulation -> centre des sommets du membre ; capsule large autour de l'axe (rayon R, longueur L) ;
#   plan de coupe incliné (normale N, jusqu'à 60° de l'axe) et décalé, choisi pour prendre le moins de sommets du corps
#   (Vincent, 25/11 : têtes et tentacules, que le plan droit mordait sur le tronc) ;
#   section testée par les trous à Ts de l'articulation, rayon Rs (rayon médian là) ;
#   rang de coupe (Vincent, 25/11 : seuls les critiques démembrent, d'abord la tête, puis les bras, et les jambes en
#   dernier ; jambe coupée = il tombe et meurt) : 0 tête, 1 bras, 2 jambe (les tentacules de l'Ubume, son bas du corps).
# Écrit Source/EON/Private/EONMembresTable.inl. Lancer : python eon_membres_table.py
import os, fnmatch, numpy as np
SAV = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(SAV, "..", "Source", "EON", "Private", "EONMembresTable.inl")


def ARP(cote):  # Auto-Rig Pro (Kama, Ikiryo)
    return [("forearm_stretch_" + cote, ["forearm_stretch_", "forearm_twist_", "forearm_", "hand_"], "forearm_twist_" + cote, cote),
            ("leg_stretch_" + cote, ["leg_stretch_", "leg_twist_", "leg_fk_", "foot_", "toes_01_"], "leg_twist_" + cote, cote)]


TETE_ARP = ("neck_x", ["head_x", "c_head_x", "head_scale_fix_x", "jaw_x"], "head_x", "")
DOIGTS = ["thumb", "index", "middlefinger", "ringfinger", "pinkie", "hand"]
SPEC = {
    "Kama": ARP("l") + ARP("r") + [TETE_ARP],
    "Ikiryo": ARP("l") + ARP("r") + [TETE_ARP],
    "Onibi": [("elbow_%s" % c, ["elbow_%s" % c] + ["*%s*_%s" % (d, c) for d in DOIGTS], "elbow_%s" % c, None) for c in "LR"]
             + [("leg_%s" % c, ["leg_%s" % c], "leg_%s" % c, None) for c in "LR"]
             + [("head", ["head", "brain_*"], "head", None)],
    "Ubume": [("Lower_arm_%s" % c, ["Lower_arm_%s" % c, "Hand_%s" % c, "*_finger_*_%s" % c], "Lower_arm_%s" % c, None) for c in "LR"]
             + [("Neck", ["Head1", "Mouth", "Mouth_End", "Head_End", "Mutation", "Mutation_End"], "Head1", None),
                ("Tentacles_2", ["Tentacles_2", "Tentacles_3", "Tentacles_4", "Tentacles_5"], "Tentacles_3", None)],
    "Nuke": [("%s_leg2_%s" % (f, c), ["%s_leg2_%s" % (f, c), "%s_leg3_%s" % (f, c), "%s_leg4_%s" % (f, c), "%s_foot_%s" % (f, c)], "%s_leg3_%s" % (f, c), None)
             for f in ("front", "back") for c in "LR"],   # « head1 » = tout le corps (les pattes s'y attachent) : pas coupable
}
lignes = ["// EON — généré par Saved/Scripts/eon_membres_table.py (25/11) : membres coupables de chaque ennemi, espace avant",
          "// skinning du maillage (cm). Ne pas modifier à la main : relancer le script.", ""]
fonc = []
for nom, spec in SPEC.items():
    os_ = [l.strip().split("|") for l in open(os.path.join(SAV, "os_%s.txt" % nom)) if l.strip()]
    names = [o[0] for o in os_]; pos = {o[0]: np.array([float(o[2]), float(o[3]), float(o[4])]) for o in os_}
    d = np.loadtxt(os.path.join(SAV, "verts_%s.txt" % nom)); P = d[:, :3]; B = d[:, 3].astype(int)
    lignes.append("static const FEONMembreDef GMembres_SK_%s[] = {" % nom)
    for joint, motifs, place, cote in spec:
        if cote:
            motifs = [mo + cote for mo in motifs]
        grp = [i for i, n in enumerate(names) if any(fnmatch.fnmatchcase(n, mo) for mo in motifs)]
        m = np.isin(B, grp); V = P[m]
        j0 = pos[joint]; a = V.mean(0) - j0; a /= np.linalg.norm(a)
        t_ = (V - j0) @ a; r_ = np.linalg.norm((V - j0) - np.outer(t_, a), axis=1)
        R = np.percentile(r_[t_ > -5], 99) + 1.5
        # Normales candidates : l'axe, puis l'axe incliné jusqu'à 60° (spirale de Fibonacci sur la calotte).
        u = np.cross(a, [0, 0, 1] if abs(a[2]) < 0.9 else [1, 0, 0]); u /= np.linalg.norm(u); v = np.cross(a, u)
        K = 240; ks = np.arange(K) + 0.5
        cz = 1 - ks / K * (1 - np.cos(np.radians(60))); sz = np.sqrt(1 - cz ** 2); ph = ks * np.pi * (3 - np.sqrt(5))
        Ns = np.vstack([a, np.outer(cz, a) + np.outer(sz * np.cos(ph), u) + np.outer(sz * np.sin(ph), v)])
        tt = (P - j0) @ a; rr = np.linalg.norm((P - j0) - np.outer(tt, a), axis=1)
        Lg0 = t_.max() + 2
        caps = (tt < Lg0) & (rr < R)                          # capsule (côté bout) ; le plan fait la séparation
        Pc = P[caps] - j0; mc = m[caps]; hors = int((m & ~caps).sum())
        best = None
        for t0 in np.arange(-6, 15.5, 1.0):
            sd = Pc @ Ns.T - t0                               # distance signée au plan, pour chaque normale
            ins = sd > 0
            faux = (ins & ~mc[:, None]).sum(0); manq = (~ins & mc[:, None]).sum(0) + hors
            sc = faux * 2 + manq                              # un sommet du corps pris coûte plus qu'un sommet oublié
            k = int(np.argmin(sc))
            if best is None or sc[k] < best[0]:
                best = (sc[k], t0, k, int(faux[k]), int(manq[k]))
        sc, t0, k, faux, manq = best
        n = Ns[k]; j = j0 + n * t0
        t = (V - j) @ a; r = np.linalg.norm((V - j) - np.outer(t, a), axis=1)
        Lg = t.max() + 2
        Ts = float(np.clip(0.12 * Lg, 3, 6))
        sec = np.abs(t - Ts) < 3
        Rs = float(np.median(r[sec]) * 0.85) if sec.sum() > 5 else R * 0.5
        incl = np.degrees(np.arccos(np.clip(n @ a, -1, 1)))
        jl = joint.lower()
        rang = 2 if jl.startswith(("leg", "front_leg", "back_leg", "tentacles")) else 0 if jl.startswith(("neck", "head")) else 1
        print("%-7s %-18s n %4d t0 %5.1f incl %4.1f L %5.1f R %5.1f Ts %.1f Rs %4.1f faux %3d manques %3d%s"
              % (nom, joint, m.sum(), t0, incl, Lg, R, Ts, Rs, faux, manq, " rang %d" % rang))
        lignes.append('\t{ TEXT("%s"), FVector(%.2f, %.2f, %.2f), FVector(%.4f, %.4f, %.4f), %.1ff, %.1ff, %.1ff, %.1ff, FVector(%.4f, %.4f, %.4f), %d },   // %s'
                      % (place, j[0], j[1], j[2], a[0], a[1], a[2], Lg, R, Ts, Rs, n[0], n[1], n[2], rang, joint))
    lignes.append("};")
    fonc.append('\tif (Nom == TEXT("SK_%s")) { return MakeArrayView(GMembres_SK_%s); }' % (nom, nom))
lignes += ["", "static TArrayView<const FEONMembreDef> MembresDuMaillage(FName Nom)", "{"] + fonc + ["\treturn {};", "}", ""]
open(OUT, "wb").write(("﻿" + "\r\n".join(lignes)).encode("utf-8"))
print("écrit", OUT)
