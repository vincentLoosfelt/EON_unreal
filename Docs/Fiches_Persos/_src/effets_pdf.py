# -*- coding: utf-8 -*-
# EON — PDF « tous les effets spéciaux » (Vincent, 24/11) depuis effets_inventaire.md.
# 1) donne un code à chaque effet qui n'en a pas (préfixe de la section + V + numéro suivant), réécrit le .md ;
# 2) .md -> HTML (titres, paragraphes, listes, tableaux, gras, code) -> PDF paysage par Edge headless.
import re, os, subprocess, html
SRC = os.path.dirname(os.path.abspath(__file__))
DOC = os.path.dirname(SRC)
MD = os.path.join(SRC, "effets_inventaire.md")
OUT = os.path.join(os.path.dirname(DOC), "Etude_VFX", "EON_Effets_Speciaux.pdf")   # dans Etude_VFX (Docs et EON_Docs sur le Drive), Vincent 25/11
VERSION, DATE = 6, "24/11/2026"
PREF = {"1": "JIR", "2": "COM", "3": "NUK", "4": "ONI", "5": "KAM", "6": "UBU", "7": "IKI", "8": "GAR", "9": "DEC", "10": "INT"}

lines = open(MD, encoding="utf-8").read().split("\n")
data = open(os.path.join(SRC, "fiches_data.py"), encoding="utf-8").read() + "\n".join(lines)
pref = None
for i, l in enumerate(lines):
    m = re.match(r"## (\d+)\. ", l)
    if m: pref = PREF.get(m.group(1)); continue
    if l.startswith("## "): pref = None; continue
    if pref and l.startswith("| — |"):
        n = max([int(x) for x in re.findall(pref + r"-V(\d+)", data)] or [0]) + 1
        code = "%s-V%d" % (pref, n)
        lines[i] = "| " + code + l[3:]
        data += "\n" + code
open(MD, "w", encoding="utf-8").write("\n".join(lines))

def inline(t):
    t = html.escape(t, quote=False)
    t = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", t)
    t = re.sub(r"`(.+?)`", r"<code>\1</code>", t)
    return t

# --- Tri en 3 classes (Vincent, 24/11) + annexe, voir effets_classes.py ---
from effets_classes import C as CLASSE
CLS = [("S", "1. Effets visuels : sprites et particules",
        "Éclats, gerbes, anneaux, traînées, rayons, poussière, vapeur, marques au sol : ce qui se fait en Niagara avec des sprites ou des rubans."),
       ("M", "2. Modélisation",
        "Objets en 3D à modéliser ou à refaire : projectiles, œuf, douille, tentacule, cuves, socles, portes, consoles, butin."),
       ("U", "3. Interface (UI)",
        "Ce qui se dessine à l'écran par-dessus le jeu : barres, chiffres de dégâts, noms, bannières, cut-in, fondus, voiles, réticule."),
       ("X", "Annexe : hors des trois classes",
        "Aucun asset à produire : secousses et poussées de caméra, arrêts sur image, lumières seules, flash de matériau, tremblements et comportements.")]
intro, sec, rows, iso, cur = [], None, [], [], "intro"
HEAD = "| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |"
for l in lines:
    if re.match(r"## \d+\. ", l): cur, sec = "sec", l[3:].split(". ", 1)[1]; continue
    if l.startswith("## Isolés"): cur = "iso"; iso.append(l); continue
    if l.startswith("## Totaux"): cur = "fin"; continue
    if cur == "intro": intro.append(l)
    elif cur == "sec":
        m = re.match(r"\| ([A-Z]{3}-[A-Z0-9.]+) \|", l)
        if m: rows.append((CLASSE[m.group(1)], sec, l))
    elif cur == "iso": iso.append(l)
new = intro[:]
tot = ["| classe | OK | PRÊT | DEBUG | ABS | total |", "|---|---|---|---|---|---|"]
for k, titre, _ in CLS:
    r = [x[2] for x in rows if x[0] == k]
    n = {e: sum(1 for x in r if x.rstrip().endswith("| %s |" % e)) for e in ("OK", "PRÊT", "DEBUG", "ABS")}
    tot.append("| %s | %d | %d | %d | %d | **%d** |" % (titre, n["OK"], n["PRÊT"], n["DEBUG"], n["ABS"], len(r)))
tot.append("| **Total** | | | | | **%d** |" % len(rows))
new += ["## Le tri en un coup d'œil", ""] + tot + [""]
for k, titre, desc in CLS:
    new += ["## " + titre, "", "**" + desc + "**", ""]
    for s in dict.fromkeys(x[1] for x in rows if x[0] == k):
        new += ["### " + s, "", HEAD, "|---|---|---|---|---|---|"] + [x[2] for x in rows if x[0] == k and x[1] == s] + [""]
new += iso
lines = new

ETAT = {"OK": "ok", "PRÊT": "pret", "DEBUG": "dbg", "ABS": "abs"}
b, i = [], 0
while i < len(lines):
    l = lines[i]
    if l.startswith("|"):
        rows = []
        while i < len(lines) and lines[i].startswith("|"):
            rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")]); i += 1
        head, body = rows[0], [r for r in rows[2:]]
        t = ["<table><tr>" + "".join("<th>%s</th>" % inline(h) for h in head) + "</tr>"]
        for r in body:
            cells = []
            for k, c in enumerate(r):
                h = head[k].lower() if k < len(head) else ""
                if h == "état" and c in ETAT: cells.append('<td><span class="b %s">%s</span></td>' % (ETAT[c], c))
                elif h == "code": cells.append('<td class="code">%s</td>' % inline(c))
                elif "fichier" in h: cells.append('<td class="loc">%s</td>' % inline(c))
                else: cells.append("<td>%s</td>" % inline(c))
            t.append("<tr>" + "".join(cells) + "</tr>")
        b.append("".join(t) + "</table>"); continue
    if l.startswith("# "): b.append("<h1>%s</h1>" % inline(l[2:]))
    elif l.startswith("## "): b.append(("<h2 class=\"nb\">%s</h2>" if "coup d" in l else "<h2>%s</h2>") % inline(l[3:]))
    elif l.startswith("### "): b.append("<h3>%s</h3>" % inline(l[4:]))
    elif l.startswith("- "):
        it = []
        while i < len(lines) and lines[i].startswith("- "): it.append("<li>%s</li>" % inline(lines[i][2:])); i += 1
        b.append("<ul>%s</ul>" % "".join(it)); continue
    elif l.strip() == "---": pass
    elif l.strip(): b.append("<p>%s</p>" % inline(l))
    i += 1

guide = ('<div class="cadre"><b>Comment me parler d\'un effet :</b> code + ce que tu veux changer. Exemples : '
         '« <span class="c">JIR-V1</span> plus court et plus blanc », « <span class="c">ONI-V1</span> deux fois plus d\'éclats », '
         '« <span class="c">KAM-C2</span> passe en vrai effet, pas en debug ». Les codes absents des fiches (ex. <span class="c">JIR-V13</span>, <span class="c">COM-V8</span>, et les préfixes '
         '<span class="c">GAR</span> gardien, <span class="c">DEC</span> décor, <span class="c">INT</span> interface) sont nouveaux : ils '
         'n\'existent que dans ce document pour l\'instant.</div>')
b.insert(1, '<div class="ver">version %d du %s · relevé dans le code du jeu</div>' % (VERSION, DATE) + guide)
CSS = """@page{size:A4 landscape;margin:12mm 11mm 14mm}
body{font-family:'Segoe UI',sans-serif;font-size:9pt;color:#2a2622;line-height:1.35}
h1{font-size:20pt;color:#7a2e3a;margin:0 0 6px} h2{font-size:15pt;color:#7a2e3a;border-bottom:2px solid #e3d6cc;padding-bottom:3px;margin:20px 0 6px;page-break-after:avoid;page-break-before:always} h3{font-size:11pt;color:#4a3a34;margin:12px 0 4px;page-break-after:avoid}
p{margin:4px 0} ul{margin:4px 0 4px 18px;padding:0}
table{border-collapse:collapse;width:100%;margin:6px 0 10px;font-size:8pt} tr{page-break-inside:avoid}
th{background:#f1e8e1;text-align:left;padding:4px 5px;font-weight:600} td{border-bottom:1px solid #eee3da;padding:3px 5px;vertical-align:top}
td.code{font-family:Consolas,monospace;font-weight:700;color:#7a2e3a;white-space:nowrap} td.loc{font-family:Consolas,monospace;font-size:7pt;color:#7a716a;width:16%}
code{font-family:Consolas,monospace;font-size:8pt;background:#f4eee9;padding:0 2px}
.b{display:inline-block;padding:1px 6px;border-radius:8px;font-weight:700;font-size:7.5pt}
.ok{background:#d9efd6;color:#2d6a27}.dbg{background:#fde7c4;color:#8a5a00}.abs{background:#f6d3d3;color:#8a2323}.pret{background:#d6e4f5;color:#23508a}
.cadre{background:#f7f1ec;border-left:4px solid #7a2e3a;padding:7px 10px;margin:8px 0}
.c{font-family:Consolas,monospace;font-weight:700;color:#7a2e3a}
.ver{font-size:8.5pt;color:#9a8f86;margin:-2px 0 6px} h2.nb{page-break-before:auto}"""
doc = ('<!doctype html><html lang="fr"><head><meta charset="utf-8"><style>%s</style></head><body>%s'
       '</body></html>'
       % (CSS, "\n".join(b)))
H = os.path.join(SRC, "_effets.html")
open(H, "w", encoding="utf-8").write(doc)
edge = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
subprocess.run([edge, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--print-to-pdf=" + OUT, "file:///" + H.replace("\\", "/")], check=True)
os.remove(H)
print(OUT, os.path.getsize(OUT))
