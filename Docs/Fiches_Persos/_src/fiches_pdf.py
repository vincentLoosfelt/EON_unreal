# -*- coding: utf-8 -*-
# Génère les fiches PDF des personnages EON (HTML -> PDF par Edge headless).
import os, sys, subprocess, html
sys.path.insert(0, os.path.dirname(__file__))
from fiches_data import COMMUN, GLOBAUX, PERSOS
DOC = "D:/_PROJETS/briscard/EON_UE/Docs/Fiches_Persos"
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
DATE = "24/11/2026"
E = html.escape

CSS = """
@page { size: A4; margin: 14mm 13mm 14mm 13mm; }
* { box-sizing: border-box; }
body { font-family: 'Segoe UI', sans-serif; color: #2a2522; font-size: 9.6pt; line-height: 1.38; margin: 0; }
h1 { font-size: 22pt; margin: 0 0 2mm; color: #1d1a18; letter-spacing: .5px; }
h2 { font-size: 12.5pt; margin: 6mm 0 2mm; color: #7a2e3a; border-bottom: 1.5px solid #e3d6cc; padding-bottom: 1mm; }
.sous { color: #8a7d72; font-size: 9pt; margin-bottom: 4mm; }
.tete { display: flex; gap: 6mm; align-items: flex-start; }
.tete img { width: 62mm; height: 62mm; border-radius: 3mm; background: #1b1918; object-fit: cover; }
.tete .id { flex: 1; }
.code { font-family: Consolas, monospace; font-weight: 700; color: #7a2e3a; white-space: nowrap; }
.role { font-size: 10.5pt; margin: 2mm 0 3mm; }
table { width: 100%; border-collapse: collapse; margin: 1mm 0 2mm; page-break-inside: auto; }
tr { page-break-inside: avoid; }
th { background: #efe6de; text-align: left; font-size: 8.6pt; padding: 1.4mm 2mm; color: #5a4e45; }
td { border-bottom: 1px solid #eee5dd; padding: 1.4mm 2mm; vertical-align: top; font-size: 8.8pt; }
.stats td:first-child { color: #8a7d72; width: 30%; }
.b { display: inline-block; padding: .3mm 2mm; border-radius: 2mm; font-size: 7.8pt; font-weight: 600; white-space: nowrap; }
.OK { background: #dcefd8; color: #2f6a2a; } .PROV { background: #fbe9c8; color: #8a5a0a; } .ABS { background: #f6d6d6; color: #8a2a2a; }
.lev { font-size: 8.2pt; color: #5a4e45; }
ul { margin: 1mm 0 0 5mm; padding: 0; } li { margin-bottom: 1mm; }
.cadre { background: #f7f1ec; border-left: 3px solid #7a2e3a; padding: 2.5mm 4mm; margin: 2mm 0; border-radius: 1mm; }
.ex { font-family: Consolas, monospace; background: #fff; border: 1px solid #e3d6cc; padding: 1.5mm 3mm; border-radius: 1mm; margin: 1.2mm 0; display: block; }
.pied { color: #a89a8e; font-size: 7.6pt; margin-top: 5mm; border-top: 1px solid #eee5dd; padding-top: 1.5mm; }
.planche { width: 100%; border-radius: 2mm; margin: 2mm 0; }
.pb { page-break-before: always; }
"""

STAT = {"OK": "Fini", "PROV": "Provisoire", "ABS": "Absent"}


def badge(s):
    return '<span class="b %s">%s</span>' % (s, STAT[s])


def table_elements(rows):
    h = ['<table><tr><th style="width:15mm">Code</th><th style="width:34mm">Nom</th><th>Ce qu\'on voit / ce qu\'il fait</th>'
         '<th style="width:30mm">Valeurs actuelles</th><th style="width:28mm">Se règle par</th><th style="width:17mm">État</th></tr>']
    for c, n, d, v, l, s in rows:
        h.append('<tr><td class="code">%s</td><td><b>%s</b></td><td>%s</td><td>%s</td><td class="lev">%s</td><td>%s</td></tr>'
                 % (E(c), E(n), E(d), E(v), E(l) or "—", badge(s)))
    h.append("</table>")
    return "".join(h)


def page(title, body):
    return ('<!doctype html><html lang="fr"><head><meta charset="utf-8"><title>%s</title><style>%s</style></head><body>%s'
            '<div class="pied">E.O.N — portage Unreal 5.8 · fiches personnages · version 2 du %s · valeurs relevées dans le code du jeu</div>'
            '</body></html>') % (E(title), CSS, body, DATE)


def fiche(p):
    b = ['<div class="tete"><img src="img/portrait_%s.png"><div class="id">' % p["id"],
         '<h1>%s</h1><div class="sous">Code de fiche <span class="code">%s</span> · nom dans le code : %s · hauteur en jeu : %s</div>'
         % (E(p["nom"]), p["id"], E(p["interne"]), E(p["taille"])),
         '<div class="role">%s</div><table class="stats">' % E(p["role"])]
    b += ['<tr><td>%s</td><td>%s</td></tr>' % (E(k), E(v)) for k, v in p["stats"]]
    b.append('</table></div></div>')
    b.append('<div class="cadre">Pour me parler d\'un élément, donne son <b>code</b> et ce que tu veux : '
             '<span class="code">%s</span> + « plus… / moins… / autre chose ». Les éléments communs à tous les ennemis '
             '(flash de coup, gerbe de mort, barre de vie…) sont dans le guide, codes <span class="code">COM-…</span>.</div>'
             % E(p["elements"][0][0].split("-")[0] + "-…"))
    b.append('<h2>Éléments nommés</h2>' + table_elements(p["elements"]))
    for t, cols, rows in p["tableaux"]:
        b.append('<h2>%s</h2><table><tr>%s</tr>' % (E(t), "".join("<th>%s</th>" % E(c) for c in cols)))
        for r in rows:
            b.append("<tr>%s</tr>" % "".join('<td%s>%s</td>' % (' class="code"' if str(x).startswith(p["id"] + "-") else "", E(str(x))) for x in r))
        b.append("</table>")
    b.append('<h2>Ce qui manque ou reste provisoire</h2><ul>%s</ul>' % "".join("<li>%s</li>" % E(m) for m in p["manque"]))
    return page("EON — " + p["nom"], "".join(b))


def guide():
    b = ['<h1>Comment me parler des personnages</h1><div class="sous">Guide des fiches E.O.N · à lire une fois, puis garder sous la main</div>',
         '<img class="planche" src="img/planche_HD.png">',
         '<h2>1. Chaque chose a un code</h2>',
         '<p>Chaque personnage a un code de 3 lettres, et chaque élément visible, audible ou réglable un numéro dans sa fiche. '
         'La lettre du milieu dit de quoi il s\'agit :</p><table><tr><th>Lettre</th><th>Famille</th><th>Exemples</th></tr>',
         '<tr><td class="code">M</td><td>Modèle, matériau, taille</td><td>JIR-M1 le modèle de Jira</td></tr>',
         '<tr><td class="code">A</td><td>Animation</td><td>NUK-A2 l\'attaque de la Nuke</td></tr>',
         '<tr><td class="code">C</td><td>Comportement, attaque, déplacement</td><td>KAM-C3 la charge-lame du Kama</td></tr>',
         '<tr><td class="code">T</td><td>Tir, projectile, objet lancé</td><td>JIR-T1 la balle de Jira, UBU-T1 l\'œuf d\'Ubume</td></tr>',
         '<tr><td class="code">G</td><td>Greffe (arme du bras de Jira)</td><td>JIR-G3 le Laser</td></tr>',
         '<tr><td class="code">V</td><td>Effet visuel : éclats, flash, anneau, lumière, traînée</td><td>ONI-V2 l\'anneau d\'explosion</td></tr>',
         '<tr><td class="code">S</td><td>Son</td><td>JIR-S1 le son de tir</td></tr>',
         '<tr><td class="code">U</td><td>Interface</td><td>JIR-U1 les cœurs et barres de Jira</td></tr>',
         '<tr><td class="code">X</td><td>Variante</td><td>KAM-X1 « La chose »</td></tr></table>',
         '<p>Codes des personnages : <span class="code">JIR</span> Jira · <span class="code">NUK</span> Nuke · '
         '<span class="code">ONI</span> Onibi · <span class="code">KAM</span> Kama · <span class="code">UBU</span> Ubume · <span class="code">IKI</span> Ikiryo · '
         '<span class="code">COM</span> commun à tous.</p>',
         '<h2>2. Comment formuler une demande</h2><div class="cadre">Code + ce que tu veux changer, en mots simples. Tu peux en enchaîner plusieurs.'
         '<span class="ex">JIR-T1 : plus fin et plus lumineux, avec une petite traînée</span>'
         '<span class="ex">ONI-C2 : armement plus long, et un bip qui accélère</span>'
         '<span class="ex">KAM-C3 trop rapide ; KAM-V1 plus visible</span>'
         '<span class="ex">COM-V4 : moins d\'éclats sur les petits ennemis</span>'
         'À l\'oral, dis les lettres : « J-I-R T-1 » ou « Jira, tir 1 ». Si tu ne connais pas le code, décris ce que tu vois : je retrouve '
         'l\'élément et je te donne son code.</div>',
         '<h2>3. Ce que veulent dire « Se règle par » et « État »</h2><table><tr><th>Mot</th><th>Sens</th></tr>',
         '<tr><td class="code">F6</td><td>Menu Tweak en jeu (touche F6) : tu peux essayer toi-même tout de suite, sans moi.</td></tr>',
         '<tr><td class="code">CONSOLE</td><td>Commande <b>eon.feel &lt;Clé&gt; &lt;valeur&gt;</b> dans la console du jeu.</td></tr>',
         '<tr><td class="code">DONNEES</td><td>Tables du jeu (DT_Ennemis, DT_Greffes) : je change la valeur, pas besoin de recompiler.</td></tr>',
         '<tr><td class="code">CODE</td><td>Écrit dans le code C++ : je le change et je recompile (quelques minutes).</td></tr>',
         '<tr><td class="code">EDITEUR</td><td>Asset Unreal (matériau, anim, effet Niagara) : je le modifie ou en crée un nouveau.</td></tr>',
         '<tr><td>%s</td><td>En place et voulu.</td></tr>' % badge("OK"),
         '<tr><td>%s</td><td>Fonctionne, mais à remplacer : ligne de debug (invisible dans le jeu final), modèle de remplacement, sphère grise…</td></tr>' % badge("PROV"),
         '<tr><td>%s</td><td>N\'existe pas encore : on peut le créer.</td></tr></table>' % badge("ABS"),
         '<p>Unités : le jeu compte en <b>px du proto</b>. 1 px = 6,25 cm ; 16 px = 1 tuile = 1 m. Une vitesse de 82 px/s fait 5,1 m/s.</p>',
         '<h2 class="pb">4. Éléments communs à tous les ennemis</h2>', table_elements(COMMUN),
         '<h2>5. Réglages globaux (menu F6, rubrique « Ennemis »)</h2><table><tr><th>Clé</th><th>Réglage</th><th>Où</th></tr>']
    b += ['<tr><td class="code">%s</td><td>%s</td><td class="lev">%s</td></tr>' % (E(k), E(n), E(w)) for k, n, w in GLOBAUX]
    b.append('</table><h2>6. Les fiches</h2><ul>')
    b += ['<li><span class="code">%s</span> — %s : %s.pdf</li>' % (p["id"], E(p["nom"]), p["fichier"]) for p in PERSOS]
    b.append('</ul>')
    return page("EON — Guide des fiches", "".join(b))


def to_pdf(name, htm):
    hp = os.path.join(DOC, name + ".html").replace("/", "\\")
    open(hp, "w", encoding="utf-8").write(htm)
    pdf = hp[:-5] + ".pdf"
    subprocess.run([EDGE, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--print-to-pdf=" + pdf,
                    "file:///" + hp.replace("\\", "/")], check=True, capture_output=True, timeout=120)
    os.remove(hp)
    print(pdf, os.path.getsize(pdf))


to_pdf("EON_Fiche_00_Guide", guide())
for p in PERSOS:
    to_pdf(p["fichier"], fiche(p))
