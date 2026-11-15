# Fabrique Docs/EON_Generation_Station_Salles.html (puis PDF via Edge).
import io, os, html

ICI = os.path.dirname(os.path.abspath(__file__))
SORTIE = os.path.join(os.path.dirname(ICI), "EON_Generation_Station_Salles.html")

CARREE = """################################
#..............................#
#..............................#
#..............................#
#..............................#
#...XXX..................XXX...#
#..............................#
#..............................#
#..............................#
#........##..........##........#
#........##..........##........#
#..............................#
#..............X..........K....#
#..............................#
#.............====.............#
#.....K.....X.====.............#
#.............====.X...........#
#.............====.............#
#..............................#
#...............X..............#
#..............................#
#........##....................#
#........##...........##########
#.....................#........#
#.....................#......K.#
#.....................#........#
#...X.................#....XX..#
#...X.................#....XX..#
#...X.................#........#
#.....................#.K......#
#.....................#........#
################################""".split("\n")

COUL = {"#": "#3b4252", ".": "#e6e9ef", "X": "#b07a3c", "K": "#4b8bbe", "=": "#151820", "D": "#3ec9b8", "I": "#ff9628"}

def grille_salle(rows, portes, interieures, case=13):
    h, w = len(rows), len(rows[0])
    g = [list(r) for r in rows]
    for (x, y) in portes: g[y][x] = "D"
    for (x, y) in interieures: g[y][x] = "I"
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">' % (w * case, h * case, w * case, h * case)]
    for y in range(h):
        for x in range(w):
            out.append('<rect x="%d" y="%d" width="%d" height="%d" fill="%s" stroke="#ffffff" stroke-width="0.6"/>' % (x * case, y * case, case, case, COUL.get(g[y][x], "#e6e9ef")))
    # flèches des composants Porte EON
    def fleche(cx, cy, dx, dy, c):
        L = case * 2.6
        x2, y2 = cx + dx * L, cy + dy * L
        out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="3"/>' % (cx, cy, x2, y2, c))
        px, py = -dy, dx
        out.append('<polygon points="%.1f,%.1f %.1f,%.1f %.1f,%.1f" fill="%s"/>' % (x2 + dx * 7, y2 + dy * 7, x2 + px * 6, y2 + py * 6, x2 - px * 6, y2 - py * 6, c))
    cy_ = "#0f8f80"
    fleche(10 * case, 3 * case, 0, -1, cy_)        # N (x = -600 cm)
    fleche(16 * case, 29 * case, 0, 1, cy_)        # S
    fleche(29 * case, 16 * case, 1, 0, cy_)        # E
    fleche(3 * case, 16 * case, -1, 0, cy_)        # W
    fleche(20 * case, 26 * case, 1, 0, "#d9700f")  # porte intérieure
    out.append("</svg>")
    return "".join(out)

SVG_CARREE = grille_salle(CARREE, [(9, 0), (10, 0), (15, 31), (16, 31), (31, 15), (31, 16), (0, 15), (0, 16)], [(22, 25), (22, 26)])

# Étage réel tiré le 15/11 (log du test) : rôles seulement, portes non dessinées.
ETAGE = [
    ".......",
    ".......",
    "..CGSOM",
    "..OD*Ip",
    "...OOFE",
    ".......",
    ".......",
]
NOMS = {"C": ("Carto", "#c9b6f2"), "G": ("Gardien", "#ff3d55"), "S": ("Sécurité", "#ffd166"), "O": ("Combat", "#f4a3a3"),
        "M": ("Marchand", "#9ad8a6"), "D": ("C.O.M", "#3ef2e0"), "*": ("Combat", "#f4a3a3"), "I": ("Irruption", "#cfe3ff"),
        "p": ("Opportunité", "#9ad8a6"), "F": ("Faille", "#ff9f6e"), "E": ("Événement", "#c9b6f2")}

def svg_etage():
    c = 74
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">' % (7 * c, 7 * c, 7 * c, 7 * c)]
    for y in range(7):
        for x in range(7):
            ch = ETAGE[y][x]
            if ch == ".":
                out.append('<rect x="%d" y="%d" width="%d" height="%d" fill="none" stroke="#d8dbe2" stroke-dasharray="3 3"/>' % (x * c + 3, y * c + 3, c - 6, c - 6))
                continue
            nom, col = NOMS[ch]
            out.append('<rect x="%d" y="%d" width="%d" height="%d" rx="8" fill="%s" stroke="#2b2f3a" stroke-width="1.5"/>' % (x * c + 4, y * c + 4, c - 8, c - 8, col))
            out.append('<text x="%d" y="%d" font-size="12" text-anchor="middle" font-family="Calibri, Arial" fill="#1b1e26">%s</text>' % (x * c + c / 2, y * c + c / 2 + 4, html.escape(nom)))
    out.append('<text x="%d" y="%d" font-size="11" font-family="Calibri, Arial" fill="#666">(3,3)</text>' % (3 * c + 8, 4 * c - 10))
    out.append("</svg>")
    return "".join(out)

CSS = """
@page { size: A4; margin: 16mm 15mm 16mm 15mm; }
body { font-family: Calibri, Arial, sans-serif; font-size: 11.5pt; color: #1d2029; line-height: 1.42; }
h1 { font-size: 24pt; margin: 0 0 2mm 0; color: #10131a; }
.sous { color: #5a6070; font-size: 12pt; margin-bottom: 8mm; }
h2 { font-size: 16pt; border-bottom: 2px solid #3ec9b8; padding-bottom: 1.5mm; margin-top: 9mm; page-break-after: avoid; }
h3 { font-size: 12.5pt; margin: 5mm 0 1.5mm 0; page-break-after: avoid; }
.encadre { background: #eefaf8; border-left: 4px solid #3ec9b8; padding: 3mm 4mm; margin: 3mm 0; }
.attention { background: #fff4e8; border-left: 4px solid #ff9628; padding: 3mm 4mm; margin: 3mm 0; }
table { border-collapse: collapse; width: 100%; margin: 2mm 0 3mm 0; font-size: 10.5pt; page-break-inside: avoid; }
th, td { border: 1px solid #cfd3dc; padding: 1.6mm 2.4mm; text-align: left; vertical-align: top; }
th { background: #f0f2f6; }
code { font-family: Consolas, monospace; background: #f0f2f6; padding: 0 1mm; border-radius: 2px; font-size: 10pt; }
ol li, ul li { margin-bottom: 1mm; }
.fig { text-align: center; margin: 4mm 0; page-break-inside: avoid; }
.leg { font-size: 9.5pt; color: #5a6070; margin-top: 1.5mm; }
.pastille { display: inline-block; width: 10px; height: 10px; border: 1px solid #888; margin: 0 1mm 0 3mm; vertical-align: middle; }
.saut { page-break-before: always; }
.etapes { counter-reset: e; list-style: none; padding-left: 0; }
.etapes > li { counter-increment: e; position: relative; padding-left: 10mm; margin-bottom: 2.5mm; }
.etapes > li::before { content: counter(e); position: absolute; left: 0; top: 0; width: 6.5mm; height: 6.5mm; border-radius: 50%; background: #3ec9b8; color: #fff; text-align: center; font-weight: bold; line-height: 6.5mm; font-size: 10pt; }
"""

def pastilles():
    it = [("#3b4252", "mur  #"), ("#e6e9ef", "sol  ."), ("#b07a3c", "caisse  X"), ("#4b8bbe", "cuve  K"), ("#151820", "fossé  ="), ("#3ec9b8", "porte extérieure"), ("#ff9628", "porte intérieure")]
    return "".join('<span class="pastille" style="background:%s"></span>%s' % (c, html.escape(t)) for c, t in it)

H = []
H.append("<!doctype html><html lang='fr'><head><meta charset='utf-8'><title>EON - Génération de la station et des salles</title><style>%s</style></head><body>" % CSS)
H.append("<h1>EON — comment la station et les salles sont générées</h1>")
H.append("<div class='sous'>Portage UE 5.8 · état du 15/11/2026 · branche <code>portage-j1-j7</code></div>")

H.append("""<div class='encadre'><b>En deux phrases.</b> À chaque étage, le jeu tire au hasard un <b>plan de station</b> :
des salles posées sur une grille autour du C.O.M, avec un rôle chacune (Combat, Marchand, Gardien…).
Quand Jira entre dans une salle pour la première fois, le jeu choisit son <b>intérieur</b> : une salle dessinée à la main
(un Blueprint) si l'une convient, sinon un intérieur généré automatiquement.</div>""")

# ---------------------------------------------------------------- 1. la station
H.append("<h2>1. La station : le plan d'un étage</h2>")
H.append("<h3>La grille</h3>")
H.append("""<p>Un étage est une grille de <b>7 × 7 cases</b>. Chaque case occupée est une salle. Le <b>C.O.M</b> est toujours au centre,
en (3,3) : c'est le hub 3D, la seule salle qui n'est pas en tuiles. Deux salles voisines peuvent être reliées par une porte
au milieu de leur côté commun : <b>nord</b> (haut), <b>sud</b> (bas), <b>est</b> (droite), <b>ouest</b> (gauche).</p>""")
H.append("<h3>Comment le plan est tiré, étape par étape</h3>")
H.append("""<ol class='etapes'>
<li><b>Le C.O.M et ses 4 bras.</b> On pose le C.O.M en (3,3) et une salle de chaque côté. L'un de ces 4 bras, tiré au hasard,
devient la <b>salle du gardien</b> (l'ascenseur vers l'étage suivant).</li>
<li><b>La croissance.</b> Les 3 autres bras poussent : chaque salle a une chance sur deux d'ouvrir chacun de ses côtés libres,
et ainsi de suite, jusqu'au nombre de salles voulu. Le gardien, lui, ne pousse jamais.</li>
<li><b>Quelques boucles.</b> Deux salles voisines qui ne sont pas encore reliées ont 10 % de chances de l'être
(jamais vers le C.O.M ni vers le gardien) : on peut tourner en rond.</li>
<li><b>La seconde entrée du gardien.</b> On cherche un chemin d'au moins 3 salles qui mène au gardien par un autre côté que le C.O.M.
S'il n'y en a pas, on recommence le tirage.</li>
<li><b>Les culs-de-sac.</b> Il en faut au moins 3 (1 à l'étage 4) pour y ranger les salles spéciales. Sinon on recommence
(400 essais au plus).</li>
<li><b>Les rôles</b> sont distribués (voir le tableau plus bas), puis la <b>forme</b> de chaque salle.</li>
</ol>""")
H.append("""<table><tr><th>Étage</th><th>Nombre de salles (C.O.M compris)</th></tr>
<tr><td>1</td><td>14 à 16</td></tr><tr><td>2</td><td>16 à 18</td></tr><tr><td>3</td><td>17 à 19</td></tr><tr><td>4 (les serres)</td><td>8 à 9</td></tr></table>""")
H.append("<div class='fig'>%s<div class='leg'>Un étage 1 réellement tiré pendant les tests du 15/11 (14 salles). Seuls les rôles sont dessinés, pas les portes.</div></div>" % svg_etage())

H.append("<h3>L'infestation et la porte rouge : pourquoi on ne va pas droit au gardien</h3>")
H.append("""<ul>
<li><b>L'infestation</b> (tous les étages) : la passerelle du C.O.M qui mène au gardien est murée par la chair. Il faut faire le tour
par la seconde entrée trouvée à l'étape 4. Quand le gardien est vaincu, la chair se rétracte et le chemin direct s'ouvre pour revenir au C.O.M.</li>
<li><b>La porte rouge</b> (étages 1 à 3) : la porte qui donne sur le gardien reste fermée tant que Jira n'a pas pris la carte rouge,
posée dans la salle <b>Sécurité</b>.</li>
<li><b>Changer d'étage</b> : une fois le gardien vaincu, on revient au C.O.M et on s'approche du cube (à moins de 5 m) pour descendre.
Un nouvel étage est tiré.</li></ul>""")

H.append("<h3>Les rôles des salles</h3>")
H.append("""<table><tr><th>Rôle</th><th>Combien</th><th>Où</th><th>Ce qui s'y passe</th></tr>
<tr><td>C.O.M (Départ)</td><td>1</td><td>centre</td><td>hub 3D, cube de changement d'étage</td></tr>
<tr><td>Gardien (Ascenseur)</td><td>1</td><td>un des 4 bras</td><td>fin de l'étage</td></tr>
<tr><td>Sécurité</td><td>1 (étages 1–3)</td><td>sur le chemin vers la seconde entrée, ou à côté</td><td>carte rouge</td></tr>
<tr><td>Événement, Marchand, Opportunité</td><td>1 chacun</td><td>culs-de-sac, du plus loin au plus proche</td><td>événement ; 3 objets payants ; 3 objets gratuits</td></tr>
<tr><td>Trésor</td><td>70 % (50 % à l'étage 4)</td><td>cul-de-sac suivant</td><td>butin (scrap + disquette, ou ADN + énergie)</td></tr>
<tr><td>Cartographe</td><td>50 % (étages 1–3)</td><td>cul-de-sac suivant</td><td>événement</td></tr>
<tr><td>Combat / Tension / Calme</td><td>tout le reste</td><td>partout</td><td>62 % / 28 % / 10 % (étage 4 : 40 / 35 / 25)</td></tr>
<tr><td>Corridor</td><td>1 (étages 1–3)</td><td>une salle Combat ou Tension en ligne droite</td><td>3 vagues, portes verrouillées ; une salle voisine devient Opportunité</td></tr>
<tr><td>Faille</td><td>1 (étages 1–3)</td><td>une salle Combat ou Tension</td><td>1 vague, verrouillée</td></tr>
<tr><td>Irruption</td><td>50 % (étages 1–3)</td><td>une salle Combat ou Tension</td><td>se joue comme une salle Calme pour l'instant</td></tr>
</table>""")
H.append("""<table><tr><th>Rôle</th><th>Ennemis</th></tr>
<tr><td>Combat</td><td>portes verrouillées ; 1 vague pour les 2 premiers combats, puis 2 vagues (1 à l'étage 4)</td></tr>
<tr><td>Tension</td><td>1 vague, portes ouvertes</td></tr>
<tr><td>Corridor</td><td>3 vagues, verrouillée</td></tr>
<tr><td>Faille</td><td>1 vague, verrouillée</td></tr>
<tr><td>Calme, Opportunité, Marchand, Trésor, Sécurité, Événement</td><td>aucun</td></tr></table>""")

# ---------------------------------------------------------------- 2. les formes
H.append("<h2 class='saut'>2. La forme de chaque salle</h2>")
H.append("<p>Après les rôles, chaque salle reçoit une <b>forme</b>, c'est-à-dire une taille en tuiles. 1 tuile = 1 m.</p>")
H.append("""<table><tr><th>Forme</th><th>Taille</th><th>Donnée à</th></tr>
<tr><td><code>std</code></td><td>32 × 20</td><td>la plupart des salles (Gardien, Marchand, Sécurité, Événement…)</td></tr>
<tr><td><code>carree</code> <b>(nouvelle)</b></td><td>32 × 32</td><td>Combat (1 chance sur 5), Tension (1 chance sur 3)</td></tr>
<tr><td><code>large</code></td><td>40 × 26</td><td>Combat, Tension, Faille</td></tr>
<tr><td><code>small</code></td><td>24 × 16</td><td>Opportunité, Calme, parfois Combat</td></tr>
<tr><td><code>corH</code> / <code>corV</code></td><td>50 × 9 / 12 × 32</td><td>Corridor ; et 60 % des Combat / Tension / Calme qui n'ont que 2 portes face à face</td></tr></table>""")

# ---------------------------------------------------------------- 3. les salles
H.append("<h2>3. L'intérieur d'une salle</h2>")
H.append("""<p>L'intérieur n'est choisi qu'à la <b>première visite</b> de la salle, puis il est mémorisé : en revenant, on retrouve
la même salle (ennemis tués compris). Une seule salle existe à la fois dans le monde : elle est reconstruite à chaque passage de porte,
à 120 m à l'est du hub.</p>""")
H.append("<h3>Le choix, dans l'ordre</h3>")
H.append("""<ol class='etapes'>
<li><b>Une salle dessinée à la main</b> (un Blueprint du dossier <code>/Game/EON/Salles</code>) qui convient. Elle convient si :
<ul><li>son <b>RoomRole</b> (ou un de ses <b>ExtraRoles</b>) est le rôle de la case ;</li>
<li>sa <b>Shape</b> est la forme de la case (Shape vide = toutes les formes) ;</li>
<li>si elle a des portes posées : elle a une porte <b>de chaque côté</b> où la case a un voisin ;</li>
<li>son <b>Weight</b> est supérieur à 0.</li></ul>
S'il y en a plusieurs, on tire au sort en fonction du <b>Weight</b> : une salle de poids 6 sort 6 fois plus souvent qu'une salle de poids 1.</li>
<li><b>Un rôle de secours</b> : sans salle pour Trésor ou Irruption, on cherche une salle Calme ; sans salle pour Cartographe, une salle Événement.</li>
<li><b>Sinon, l'intérieur est généré</b> automatiquement comme dans le proto : une forme (croix, L, anneau, encoche ou rectangle simple),
puis selon le rôle des piliers, des trous, des caisses ou un îlot central.</li></ol>""")
H.append("""<div class='encadre'>Aujourd'hui, <b>18 salles dessinées</b> : Calme A et B, Combat A, B, C, D et Carrée, Corridor H et V, Événement, Faille A et B,
Gardien, Marchand, Opportunité, Sécurité, Tension A et B.</div>""")

# ---------------------------------------------------------------- 4. créer une salle
H.append("<h2 class='saut'>4. Créer ou modifier une salle dessinée</h2>")
H.append("""<ol class='etapes'>
<li>Dans le Content Browser, ouvre <code>/Game/EON/Salles</code>. Duplique une salle proche de ce que tu veux (clic droit, puis Duplicate) :
les matériaux et les réglages suivent.</li>
<li>Ouvre-la. Dans <b>Class Defaults</b>, catégorie <b>EON | Salle</b> : règle <b>RoomRole</b> et <b>Shape</b>.
Catégorie <b>EON | Salle | Plan</b> : <b>ExtraRoles</b> (autres rôles possibles) et <b>Weight</b>.</li>
<li><b>Layout</b> est le plan de la salle : une ligne de texte par rangée de tuiles, de haut en bas.
Le bouton <b>Plan Depuis Generateur</b> le remplit avec un intérieur généré, comme point de départ.</li>
<li>Pose les <b>portes</b> avec le composant <b>Porte EON</b> (chapitre suivant).</li>
<li>Ajoute ce que tu veux par-dessus : lumières, meshes, effets. Ils apparaîtront avec la salle en jeu.
Les tirs et le chemin des ennemis ne connaissent que le <b>Layout</b> : un obstacle qui doit vraiment compter en combat se dessine dans le plan.</li>
<li>Compile et enregistre. Le jeu lit toutes les salles du dossier au démarrage : rien d'autre à faire.</li></ol>""")
H.append("""<table><tr><th>Signe</th><th>Tuile</th><th>Effet</th></tr>
<tr><td><code>#</code></td><td>mur</td><td>bloque tout</td></tr>
<tr><td><code>.</code></td><td>sol</td><td>on y marche ; les ennemis peuvent y apparaître</td></tr>
<tr><td><code>X</code></td><td>caisse</td><td>décor solide, couvert contre les tirs</td></tr>
<tr><td><code>K</code></td><td>cuve</td><td>décor solide</td></tr>
<tr><td><code>=</code></td><td>fossé</td><td>vide franchissable au dash</td></tr>
<tr><td><code>_</code> ou espace</td><td>vide</td><td>on y tombe</td></tr></table>""")
H.append("""<div class='attention'>Le plan doit faire au moins 8 × 6 tuiles et compter au moins 12 tuiles de sol. Sinon la salle est refusée
et le log affiche « Salle … inutilisable » ; le jeu génère alors un intérieur à sa place.</div>""")

# ---------------------------------------------------------------- 5. les portes
H.append("<h2>5. Les portes : le composant Porte EON</h2>")
H.append("""<p>Dans le Blueprint, panneau <b>Components</b>, bouton <b>Add</b>, puis <b>Porte EON</b>. C'est une flèche, visible seulement dans l'éditeur.
<b>Le sens de la flèche donne le côté</b> :</p>
<table><tr><th>La flèche pointe vers</th><th>Porte</th><th>Rotation Z (lacet)</th></tr>
<tr><td>le haut de l'écran (−Y)</td><td>nord</td><td>−90</td></tr>
<tr><td>le bas de l'écran (+Y)</td><td>sud</td><td>90</td></tr>
<tr><td>la droite (+X)</td><td>est</td><td>0</td></tr>
<tr><td>la gauche (−X)</td><td>ouest</td><td>180</td></tr></table>
<h3>Porte extérieure (case « Exterieure » cochée, flèche cyan)</h3>
<ul><li>C'est une sortie vers la salle voisine. Elle se cale sur le bord vers lequel pointe la flèche,
<b>à l'endroit où tu l'as posée le long de ce bord</b> : tu peux la décaler vers un coin.</li>
<li><b>4 au maximum</b>, une par côté. Une deuxième porte du même côté est ignorée et signalée dans le log.</li>
<li>Elle fait toujours 2 tuiles de large.</li>
<li>Si la case de l'étage n'a pas de voisin de ce côté, la porte devient un mur.</li>
<li>La salle n'est tirée que pour une case dont <b>tous</b> les voisins tombent sur une de ses portes.
Avec 4 portes, elle peut aller partout ; avec seulement nord et sud, jamais sur une case qui a aussi un voisin à l'est.</li>
<li>Une salle <b>sans aucune</b> Porte EON garde l'ancien système : une porte au milieu de chaque côté qui a un voisin.</li></ul>
<h3>Porte intérieure (case décochée, flèche orange)</h3>
<ul><li>Un obstacle qu'on ouvre : elle bloque Jira, les balles et le chemin des ennemis tant qu'elle est fermée.</li>
<li>Elle se place <b>en travers du passage</b> indiqué par la flèche : une flèche vers l'est donne un panneau nord-sud, sur <b>Largeur</b> tuiles (1 à 8).</li>
<li>Jira l'ouvre avec <b>Interagir</b>, à 2,5 tuiles au plus ; l'invite « Interagir : ouvrir la porte » s'affiche à portée. Elle reste ouverte.</li>
<li>Pose-la dans une ouverture du <b>Layout</b> (entre deux murs), sinon on peut la contourner.</li></ul>
<div class='attention'>Range les Porte EON directement sous la racine du Blueprint, sans les attacher à un autre composant :
leur position serait mal lue.</div>""")

# ---------------------------------------------------------------- 6. exemple
H.append("<h2 class='saut'>6. Exemple : la salle carrée</h2>")
H.append("""<p><code>/Game/EON/Salles/BP_Salle_Combat_Carree</code> · rôle Combat (et Tension) · forme <code>carree</code> 32 × 32 · Weight 6.
Comme c'est la seule salle dessinée de cette forme, elle sort à chaque fois qu'une case Combat ou Tension reçoit la forme carrée.</p>
<ul><li>4 portes extérieures : la porte <b>nord est décalée</b> de 6 m vers l'ouest, pour montrer qu'on peut placer une porte où on veut le long d'un bord ;
les 3 autres sont au milieu.</li>
<li>Une <b>réserve</b> dans le coin sud-est, fermée par une porte intérieure de 2 tuiles (flèche vers l'est), avec deux cuves et des caisses.</li>
<li>3 piliers, un fossé central de 4 × 4 à franchir au dash, des caisses pour se couvrir.</li></ul>""")
H.append("<div class='fig'>%s<div class='leg'>%s<br>Les flèches montrent les composants Porte EON tels qu'ils sont posés dans le Blueprint.</div></div>" % (SVG_CARREE, pastilles()))

# ---------------------------------------------------------------- 7. commandes
H.append("<h2>7. Commandes pour tester</h2>")
H.append("""<p>Dans la console du jeu (touche ² ou `) :</p>
<table><tr><th>Commande</th><th>Effet</th></tr>
<tr><td><code>eon.map</code></td><td>plan de l'étage dans le log (rôles, case de Jira entre crochets)</td></tr>
<tr><td><code>eon.forme carree</code></td><td>va dans une salle de cette forme ; s'il n'y en a pas, une salle Combat ou Tension pas encore visitée la prend</td></tr>
<tr><td><code>eon.salle MARCHAND</code></td><td>va dans la salle de ce rôle</td></tr>
<tr><td><code>eon.go</code> / <code>eon.go N</code></td><td>passe une porte (vers une salle pas visitée, ou du côté indiqué)</td></tr>
<tr><td><code>eon.etage</code></td><td>tire un nouvel étage</td></tr>
<tr><td><code>eon.kill</code></td><td>tue les ennemis de la salle (pour déverrouiller les portes)</td></tr></table>""")

H.append("<p class='leg'>Code : EONFloorManager (plan de l'étage, rôles, formes), EONTileGrid (formes, intérieurs générés), EONTileRoom (salles dessinées, portes), EONDoorComponent (Porte EON).</p>")
H.append("</body></html>")
io.open(SORTIE, "w", encoding="utf-8").write("\n".join(H))
print(SORTIE)
