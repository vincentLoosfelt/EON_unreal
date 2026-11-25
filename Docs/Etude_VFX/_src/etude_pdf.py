# -*- coding: utf-8 -*-
# EON — étude du style des effets « animé japonais années 90 » (Vincent, 24/11) -> PDF par Edge headless.
import os, subprocess
SRC = os.path.dirname(os.path.abspath(__file__)); DOC = os.path.dirname(SRC)
OUT = os.path.join(DOC, "EON_Etude_VFX_Anime90.pdf")
VERSION, DATE = 1, "24/11/2026"


def t(rows, head):
    return ("<table><tr>" + "".join("<th>%s</th>" % h for h in head) + "</tr>"
            + "".join("<tr>" + "".join("<td>%s</td>" % c for c in r) + "</tr>" for r in rows) + "</table>")


V = "<span class='v'>source</span>"
D = "<span class='d'>lecture</span>"
S = []
S.append("<h1>E.O.N — Étude des effets façon animé des années 90</h1><div class='ver'>version %d du %s · références : Akira (1988), Ghost in the Shell (1995)</div>" % (VERSION, DATE))
S.append("<div class='cadre'><b>But.</b> Faire passer les effets du jeu du carré pixel du proto à des effets dessinés comme dans un animé des années 90. "
         "Ce document réunit ce que montrent les références, les choix que tu as faits le 24/11 (6 séries de questions) et la façon de les fabriquer dans Unreal. "
         "Légende : %s = vérifié dans un article cité ; %s = mon analyse, sans source.</div>" % (V, D))
S.append("<h2>1. Ce que montrent les références</h2>")
S.append("<h3>Fumée (ta référence : le plan de fumée qui se dissipe dans Akira)</h3><ul>"
         "<li>%s Volutes en grappes (« chou-fleur »), en 2 tons nets (clair + ombre), sans dégradé.</li>"
         "<li>%s La fumée prend la couleur de la lumière ambiante (rose dans ce plan) : elle n'est pas grise.</li>"
         "<li>%s Elle se dissipe en s'ouvrant en anneau vers l'extérieur ; les bords restent nets, sans fondu en transparence.</li>"
         "<li>%s Shinya Ohira a dessiné la fumée « réaliste » d'Akira : chaque masse a sa forme et cède la place à une nouvelle, dans une évolution organique continue.</li></ul>" % (D, D, D, V))
S.append("<h3>Lumière, flashs et explosions</h3><ul>"
         "<li>%s Toutes les lueurs d'Akira (feux des motos, contre-jours) sont des cels rétroéclairés : de la vraie lumière passe à travers le cel, avec un bord adouci par un flou.</li>"
         "<li>%s Le halo tire vers le cyan en s'éloignant du centre blanc ; les explosions les plus vives ont souvent un halo rouge-orange.</li>"
         "<li>%s Explosions « blanc craie » dans Akira ; chez Anno, explosions « en bulles » qui gonflent en plusieurs sphères puis deviennent fumée.</li>"
         "<li>%s Images d'impact : images monochromes très contrastées, tenues une fraction de seconde.</li></ul>" % (V, V, V, V))
S.append("<h3>Débris et cadence</h3><ul>"
         "<li>%s Débris en aplat clair (éclats de verre bleu pâle dans le plan de fumée) qui retombent.</li>"
         "<li>%s Akira : environ 2 200 plans, 160 000 dessins, 327 teintes dont 50 créées pour le film.</li>"
         "<li>%s Les effets sont animés « en 2 », soit 12 i/s : c'est la cadence choisie, comme pour les persos.</li>"
         "<li>Aucune source fiable sur le sang et les étincelles de ces deux films : je les traite sur le même modèle (aplats + trait).</li></ul>" % (D, V, D))
S.append("<h2>2. Tes choix du 24/11</h2>")
S.append(t([
    ["Sprites d'effets", "Animé 90, 12 i/s, trait sombre fin ; générés par moi dans ChatGPT"],
    ["Fumée", "Sphères animées par Niagara (évolution, turbulence) + shader toon 2 tons + trait ; pour la poussière de course et de dash, les explosions et la vapeur des cuves"],
    ["Explosions", "Dôme blanc aveuglant bordé d'orange, puis fumée en aplats qui monte"],
    ["Gros coups", "Flash bichrome plein écran pendant 2 images (noir + couleur de l'effet) ; onomatopées seulement sur ces coups"],
    ["Sang", "Couleur du monstre ; taches au sol 10 s, 100 au plus"],
    ["Flash des tirs", "Étoile dessinée à 4 ou 6 branches, 3 dessins qui alternent"],
    ["Ondes au sol", "Anneau épais irrégulier qui déforme l'image (onde de choc)"],
    ["Dash", "Silhouettes de Jira en aplat cyan avec trait, à 12 i/s, transparence dégressive"],
    ["Lumière", "Seuls les gros effets éclairent la scène"],
    ["Annonces ennemies", "Ikiryo et Kama : laser fin qui s'épaissit avant le coup ; Onibi, Ubume et apparition : formes au sol"],
    ["Chiffres", "BD / manga : blanc, jaune dès 3, rouge pour les critiques et les morts ; contour noir épais"],
], ["sujet", "choix"]))
S.append("<h2>3. Comment les fabriquer dans Unreal</h2>")
S.append(t([
    ["Fumée toon", "Niagara, émetteur à maillage (sphères). Taille qui gonfle puis s'érode, montée lente, bruit de turbulence. Matériau : seuil sur la lumière qui donne 2 tons, couleur teintée par la lumière ambiante, dissipation par érosion d'un bruit (le seuil monte avec l'âge : bords nets, pas de fondu). Trait : le post-process celluloïd déjà en place. Position et taille figées par paliers de 1/12 s.", D + " recette ; tutoriels Niagara « toon explosion »"],
    ["Sprites dessinés", "Planches de 4×4 ou 8×8 images générées dans ChatGPT, détourées, jouées à 12 i/s dans Niagara (SubUV, sans interpolation entre images).", V + " doc Epic flipbook"],
    ["Onde de choc", "Anneau translucide en réfraction (distorsion de l'image) + trait dessiné.", V + " 80.lv"],
    ["Flash bichrome", "Matériau post-process après le tonemapping : seuil de luminance sur 2 couleurs, piloté pendant 2 images.", V + " doc Epic post-process"],
    ["Taches de sang", "Decals au sol à l'arrivée des gouttes, couleur du monstre, 10 s, 100 au plus.", ""],
    ["Silhouettes de dash", "Fantômes actuels (copie de pose) en matériau aplat + trait, poses figées par 1/12 s.", ""],
], ["effet", "méthode", "source"]))
S.append("<h2>4. Les planches à générer dans ChatGPT</h2>")
S.append(t([
    ["P1", "Éclats d'impact", "8 dessins : éclats anguleux qui jaillissent, aplat + ombre + trait sombre fin"],
    ["P2", "Giclée de sang", "8 dessins en blanc (teinté dans le jeu à la couleur du monstre)"],
    ["P3", "Taches de sang au sol", "6 formes vues de dessus"],
    ["P4", "Flash de bouche", "3 étoiles à 4 ou 6 branches inégales, blanc + bord"],
    ["P5", "Dôme d'explosion", "8 dessins : dôme blanc qui grossit, bord orange"],
    ["P6", "Étincelles", "8 dessins : traits courts qui filent et se brisent"],
    ["P7", "Onomatopées", "BAM, ZAP… en lettres manga épaisses, contour noir"],
], ["planche", "contenu", "détail"]))
S.append("<p>Fond uni à détourer, vue de 3/4 de dessus comme la caméra du jeu. Dessins en niveaux de gris pour être teintés dans le jeu, sauf le dôme d'explosion et les onomatopées.</p>")
S.append("<h2>5. Ordre de travail</h2><ol><li>Fumée toon Niagara : prototype sur la poussière de dash, rendu soumis en image.</li>"
         "<li>Planches ChatGPT P1 à P7, montées en Niagara à 12 i/s.</li><li>Explosions (dôme + fumée), flash bichrome, onomatopées.</li>"
         "<li>Sang au sol, annonces ennemies, chiffres manga.</li></ol>")
S.append("<h2>Sources</h2><p class='src'>animetudes.com/2021/06/06/artist-spotlight-shinya-ohira · features.japantimes.co.jp/akira-new · gizmodo.com/the-amazing-ways-akira-uses-light-to-tell-its-story-1789488147 · animationobsessive.substack.com/p/dangerous-light · thevarsity.ca/2021/09/11/akira-animated-film-influence · animetudes.com/2021/04/23/graviton-gainax-and-the-itano-school · knowyourmeme.com/memes/cultures/impact-frames · slashfilm.com/807579 · gdcvault.com/play/1022031 (Guilty Gear Xrd) · gdcvault.com/play/1034330 (Hi-Fi Rush) · sidefx.com/community/spider-man-into-the-spider-verse · dev.epicgames.com (Niagara quick start, flipbook baker, post-process materials) · 80.lv (shockwave) · forums.unrealengine.com (toon explosion UE 5.5)</p>")
CSS = """@page{size:A4;margin:14mm 13mm}
body{font-family:'Segoe UI',sans-serif;font-size:10pt;color:#2a2622;line-height:1.4}
h1{font-size:20pt;color:#7a2e3a;margin:0 0 4px} h2{font-size:14pt;color:#7a2e3a;border-bottom:2px solid #e3d6cc;padding-bottom:3px;margin:18px 0 6px;page-break-after:avoid} h3{font-size:11pt;color:#4a3a34;margin:10px 0 3px}
ul,ol{margin:4px 0 6px 18px;padding:0} li{margin:2px 0}
table{border-collapse:collapse;width:100%;margin:6px 0 10px;font-size:9pt} tr{page-break-inside:avoid}
th{background:#f1e8e1;text-align:left;padding:4px 6px} td{border-bottom:1px solid #eee3da;padding:4px 6px;vertical-align:top}
td:first-child{font-weight:600;white-space:nowrap}
.cadre{background:#f7f1ec;border-left:4px solid #7a2e3a;padding:8px 10px;margin:8px 0}
.ver{font-size:9pt;color:#9a8f86;margin-bottom:6px}
.v,.d{display:inline-block;padding:0 5px;border-radius:7px;font-size:8pt;font-weight:700}
.v{background:#d9efd6;color:#2d6a27}.d{background:#fde7c4;color:#8a5a00}
.src{font-size:8pt;color:#7a716a}"""
H = os.path.join(SRC, "_etude.html")
open(H, "w", encoding="utf-8").write("<!doctype html><html lang='fr'><head><meta charset='utf-8'><style>%s</style></head><body>%s</body></html>" % (CSS, "\n".join(S)))
subprocess.run([r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe", "--headless=new", "--disable-gpu", "--no-pdf-header-footer",
                "--print-to-pdf=" + OUT, "file:///" + H.replace("\\", "/")], check=True)
os.remove(H)
print(OUT, os.path.getsize(OUT))
