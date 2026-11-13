# 03 — Ennemis et boss

Unités proto : px, px/s, secondes. Données : `DONNEES/ennemis.json`.
Dans le code : `ED`, `mkEnemy()`, `spawnWave()`, le `switch(e.type)` dans `update()`, `chaseDir()`, `computeFlow()`, `bossUpdate()`, `mechaFight()`, `chercheurFight()`.

## 1. Les noms

| Nom officiel | Nom dans le code | Nom dans le dossier 3D | Rôle |
|---|---|---|---|
| **Nuke** | `goule` | Ghoul | Mêlée en meute |
| Nuke (petite) | `gouleP` | Ghoul, plus petite | Pondue par l'Ubume |
| **Onibi** | `kamikaze` | Kamikaze | Fonce et explose |
| **Kama** | `slasher` | Slasher | Charge à la lame |
| **Ikiryo** | `sniper` | Shooter | Tireur à distance |
| **Ubume** | `invocateur` | Mage (Invocateur) | Pond des œufs |
| **Méduse** | `meduse` | (aucun modèle) | Volante, salve de 3 tirs |
| **Brainless** | `brainless` | (aucun modèle) | Charge tête baissée |

Méduse et Brainless sont des noms provisoires : ils n'existent que dans le proto.

## 2. Tableau des stats

| Mob | PV | Vitesse | Masse | Rayon | Hitbox (h × r) | Contact | Vole | ADN |
|---|---|---|---|---|---|---|---|---|
| Nuke | 3 | 42 | 1,3 | 5 | 16 × 5 | oui | | 1 |
| Nuke (petite) | 3 | 48 | 1 | 5 | 10 × 5 | oui | | 0 |
| Onibi | 2 | 68 | 0,8 | 6 | 20 × 7 | | | 1 |
| Méduse | 4 | 36 | 1,3 | 6 | 18 × 6 | | oui | 2 |
| Ikiryo | 4 | 34 | 1,3 | 6 | 22 × 5 | | | 2 |
| Brainless | 8 | 30 | 2,6 | 7 | 22 × 6 | | | 2 |
| Kama | 6 | 38 | 1,7 | 6 | 22 × 5 | oui | | 2 |
| Ubume | 7 | 34 | 1,5 | 6 | 22 × 6 | | | 3 |

- **Montée en difficulté** : les PV sont multipliés par `1 + 0,07 × combatN` (arrondi au supérieur). `combatN` est le nombre de salles hostiles déjà entamées dans la run. Il ne se remet pas à zéro entre les étages.
- **Contact** : « oui » = toucher Jira lui enlève 1 PV. Les autres ne blessent que par leur attaque.
- **Masse** : divise le recul subi.
- Tous les mobs font 1 PV de dégât par coup.

## 3. Règles communes

**Se déplacer** : chaque mob calcule une vitesse voulue, et sa vitesse réelle la rejoint avec un lissage (`min(1, dt × 8)`). Le recul s'ajoute par-dessus et s'amortit (`dt × 9`).

**Trouver son chemin** (`computeFlow`, `chaseDir`) : un champ de distances est calculé par parcours en largeur depuis la tuile de Jira, à travers le sol et les portes (pas le vide, pas les murs). Il est recalculé quand Jira change de tuile. Un mob suit la pente : il va vers le voisin (8 directions, sans couper les coins) le plus proche de Jira. Sur la même tuile ou une tuile voisine, il va droit sur elle. En 3D, tu peux reproduire ce champ tel quel sur la grille de la salle, ou utiliser la navigation d'Unreal si le résultat est le même : les mobs contournent les trous et les piliers.

**Le vide** : un mob au sol traite le vide comme un mur. **Sauf** s'il est projeté (recul supérieur à 45 px/s) ou en pleine charge : là, il passe par-dessus le bord et **tombe**. Chute de 0,45 s, puis mort. Elle donne son ADN, +1 scrap, et affiche « Chute +1 ». Les volants ne tombent jamais.

**Apparition** : une vague s'annonce 0,7 s à l'avance par un marqueur au sol (plus 0,05 s par ennemi, pour qu'ils n'arrivent pas tous à la même image). Les points d'apparition sont à plus de 95 px de Jira et jamais collés à un trou.

**À la mort** : gerbe de particules dans le sens du coup, hitstop 55 ms, secousse. Butin : l'ADN du mob, du scrap avec `12 % × valeur d'ADN` de chances, un ramassable d'énergie une fois sur quatre, un cœur 4 % du temps.

## 4. Comportements

### Nuke : la nuée
Elles apparaissent par meute de 3 ou 4 (45 % de chances d'en avoir 4).
- **En meute** (au moins 3 dans un rayon de 70 px, ou autant qu'il en reste) : elles foncent sur Jira en débordant par le côté. Chacune a un côté préféré et ajoute 35 % de vitesse latérale.
- **Pas assez nombreuses** : elles rejoignent la Nuke la plus proche (pendant 3 s au plus), à 110 % de leur vitesse.
- **Seule** : elle garde ses distances (recule si Jira est à moins de 120 px) pendant 3,5 s, puis s'enrage et charge à 130 %. Ce délai évite qu'une Nuke isolée bloque la salle.
- Elles se repoussent entre elles à moins de 10 px.

### Onibi
- Fonce sur Jira en ondulant (vitesse × `1 + 0,3 × sin`).
- À moins de 18 px : il s'arme (0,45 s, signal sonore), puis **explose**. Rayon 28 px, 3 dégâts aux autres ennemis, 1 PV à Jira si elle est à moins de 80 % du rayon.
- **Tué avant** : il explose quand même, mais plus petit (rayon 26, 2 dégâts) et sans blesser Jira. Tuer un Onibi au milieu d'un groupe est rentable.

### Méduse
- Volante. Reste entre 70 et 115 px de Jira et tourne autour (60 % de vitesse latérale, change de sens au hasard).
- Toutes les 1,7 à 2,4 s : une salve de 3 projectiles en éventail (écart 0,22 rad), 82 px/s, vie 3 s.

### Kama
- Marche vers Jira. À moins de 85 px : **préparation** 0,55 s (il fige sa direction, signal sonore), puis **charge** en ligne droite à 235 px/s pendant 0,32 s, puis récupération 0,6 s. Recharge 1,2 s.
- S'il heurte un mur pendant la charge : récupération de 0,7 s.

### Ikiryo
- Garde ses distances : recule sous 170 px, avance au-delà de 280 px, et se décale sur le côté.
- Quand il est à plus de 110 px et prêt : il **vise** pendant 1,1 s. Une ligne de visée suit Jira. Puis il tire **un seul projectile rapide** (170 px/s, gros). Recharge de 2,6 à 3,5 s.
- Il marche : on peut le pousser dans le vide.

### Brainless
- Marche vers Jira. À moins de 150 px : il **verrouille** 0,65 s en reculant un peu, puis **charge** à 235 px/s pendant 1,7 s au plus, tout droit.
- S'il touche Jira : 1 PV, elle est projetée (170 px/s), il est sonné 0,9 s.
- S'il percute un mur : **étourdi 1,6 s**. C'est la fenêtre pour le tuer.
- Recharge 2 s.

### Ubume
- Garde ses distances (recule sous 120 px, avance au-delà de 200 px).
- Toutes les 3,4 à 4,6 s, s'il y a moins de 3 œufs en jeu : il pond (0,75 s). L'œuf vole en cloche à 45-100 px, vers Jira, et **éclot en petite Nuke** 2 s après. Rien n'éclot si l'œuf retombe dans le vide.
- La petite Nuke fonce simplement sur Jira.

## 5. Les vagues

Dans `spawnWave(mul)`. Une vague a un **budget** :

`budget = min(13, (5 + 0,75 × combatN) × surface × mul)`, divisé par 2 à l'étage 4.

- `surface` = nombre de tuiles de sol accessibles ÷ 150, borné entre 0,85 et 1,35.
- `mul` dépend de la salle : Combat 1, Tension 0,6, Faille 0,9, Corridor 1,15 puis 1,5.

On tire des mobs au hasard jusqu'à épuiser le budget :

| Mob | Coût | Disponible à partir de |
|---|---|---|
| Meute de Nuke (3 ou 4) | 3,2 | combatN ≥ 1 |
| Onibi | 1,1 | 1 |
| Méduse | 1,7 | 2 |
| Ikiryo | 1,9 | 2 |
| Brainless | 1,8 | 2 |
| Kama | 2,1 | 3 |
| Ubume | 2,6 | 3 |

- À partir de l'étage 2 : Brainless, Méduse et Ubume ont deux fois plus de chances de sortir.
- Dans le **Corridor** : seulement Nuke, Onibi, Kama, Brainless, et ils arrivent du fond du couloir.
- La vague suivante part **dès qu'il reste un seul ennemi** : la pression ne retombe pas.

**Les élites**
- Seulement sur la première vague d'une salle, jamais dans le Corridor ni la Faille, et seulement si `combatN ≥ 3`.
- 40 % de chances. Un Kama, Ikiryo, Brainless ou Ubume de la vague devient élite.
- PV ×2,2, rayon ×1,3, hitbox ×1,45 en hauteur et ×1,3 en largeur, masse ×2. Son nom s'affiche (« Kama élite »).
- Après une élite : **2 salles hostiles sans élite**. Jamais deux dans la même salle.
- Butin : +6 ADN, 2 Données, +3 scrap.

## 6. Les gardiens

Règles communes :
- Le gardien attend dans sa salle (rôle `ASCENSEUR`), devant une cuve. La salle se verrouille à l'entrée.
- Une intro, un carton-titre (nom et sous-titre, 2,6 s), puis le combat.
- Une barre de vie en bas de l'écran, avec des encoches aux seuils de phase.
- Changement de phase : 1,5 s d'invulnérabilité, rugissement, les projectiles sont effacés.
- Toucher le boss : 1 PV.
- L'onde et la Griffe ne le déplacent pas.
- Mort : 1,9 s d'explosions en chaîne, puis le butin : 12 Données (4 × 3), 12 scrap, 25 ADN, 2 cœurs, et **2 objets au choix** sur des socles (on en prend un).
- Message : « La chair se rétracte, retourne au C.O.M ».

### Le Tentaculaire (étage 1, et étage 5 et plus)
**170 PV** × `1 + 0,35 × (étage − 1)`. Un spécimen à tentacules avec un œil, qui sort de sa cuve. Rayon 20.

**Intro (3 s)** : la cuve se fissure, la vitre éclate, il sort.

| Phase | PV | Déplacement | Attaques |
|---|---|---|---|
| **1** | 100 → 66 % | Oscille de ±52 px devant sa cuve | **Éventail** : 5 tirs sur 0,7 rad, 95 px/s, toutes les 1,25 s. **Couronne** : 16 tirs en cercle, 58 px/s, toutes les 3,6 s (le cercle tourne de 0,2 rad à chaque fois). |
| **2** | 66 → 33 % | Idem | **Éventail** réduit : 3 tirs, 100 px/s, toutes les 2 s. **Mortiers** : 3 cercles au sol (un sur Jira, deux à 30-80 px), rayon 22, explosion après 1,1 s, toutes les 2,8 s. **Tentacule** : une ligne vers Jira, annoncée 0,75 s, toutes les 2,3 s. |
| **3** « Rupture » | < 33 % | Quitte sa cuve et avance sur Jira (22 px/s). **Charge** : annonce 0,6 s, puis 200 px/s pendant 0,55 s, toutes les 4,5 s. | **Double spirale** : 2 tirs opposés (68 px/s) toutes les 0,16 s, qui tournent de 0,42 rad. **2 tentacules** (un direct à 0,55 s, un qui anticipe à 0,6 s) toutes les 1,8 s. **2 mortiers** toutes les 4,2 s. |

**Un tentacule** : une ligne jusqu'à 230 px (arrêtée par les murs), large de 9 px. Annonce, frappe en 0,16 s, reste 0,4 s, se rétracte en 0,3 s. Il blesse pendant la frappe et la tenue.

### Mechakuve (étage 2)
**240 PV.** Une machine-cuve mécano-organique, **ancrée** (elle ne bouge qu'en phase 4). Rayon 17, hitbox haute de 58 px. Ses attaques s'accélèrent à chaque phase (×1 / 1,1 / 1,25 / 1,45).

| Phase | PV | Ce qui se passe |
|---|---|---|
| **1** Cuve fermée | 100 → 75 % | **Jets d'acide** : 2 jets visant Jira (avec anticipation), toutes les 2,3 s. Chute en 0,95 s, puis une **flaque** de rayon 15 pendant 4,5 s, qui blesse toutes les secondes. **Pieux** : des tentacules jaillissent de points fixes de la salle (8 points choisis au début, entre 60 et 200 px du boss). 3 points à la fois, annoncés 0,9 s par un cercle, rayon 13, toutes les 4,2 s. |
| **2** Bouclier | 75 → 50 % | **Invulnérable** tant que ses **2 générateurs** tiennent (14 PV chacun, posés loin du boss et loin l'un de l'autre, reliés par des câbles verts). **Coup de bouclier** si Jira est à moins de 64 px : annonce 0,45 s, rayon 70, 1 PV et forte projection, toutes les 2,2 s. Générateurs détruits : bouclier brisé, boss **sonné 2,5 s**. L'acide (toutes les 2,8 s) et les pieux (3,4 s) continuent. |
| **3** La cuve s'ouvre | 50 → 25 % | 3 jets d'acide, 4 pieux. **Les pinces** : elles attrapent si Jira est à moins de 58 px, ou si elle reste **immobile plus de 1,3 s** à moins de 170 px. Annonce 0,55 s : **un dash l'évite**. Sinon 1 PV, et Jira est tenue 0,9 s et tirée vers le boss. Recharge 3 s. |
| **4** Le spécimen sort | < 25 % | Tout ce qui précède, plus : la machine **avance** (14 px/s), et de la **matière organique** apparaît sous Jira toutes les 2,6 s (rayon 16-22, dure 6 s) : elle ralentit fortement et blesse toutes les 1,4 s. |

### Le Chercheur (étage 3)
**300 PV.** Un assistant du docteur qui s'est injecté le remède. Rayon 12, hitbox haute de 34 px.

**Intro** : il est humain et accueille Jira (dialogue `chercheur`). Puis son bras droit enfle en une masse de chair qui avale le métal (2,6 s). Référence : Tetsuo dans Akira.

Il marche vers Jira (40 / 46 / 30 px/s selon la phase) et s'arrête à 44 px. Entre deux attaques : 1,1 / 0,9 / 0,75 s. Le bras grossit à chaque phase (facteur A = 1 / 1,3 / 1,6).

| Attaque | Condition | Détail |
|---|---|---|
| **Balayage** | Jira proche (moins de 64 + 16 × A px), 55 % du temps | Annonce 0,55 s par un cône rouge. Cône de ±1,15 rad, rayon 60 + 16 × A. |
| **Écrasement** | Par défaut | Un cercle au sol vers Jira (jusqu'à 110 px), rayon 24 + 6 × A. Annonce 0,8 s (0,65 en phase 3). À l'impact : une couronne de 8 / 12 / 14 tirs (70 px/s). |
| **Bras étiré** | Phase 2+, Jira à plus de 60 px | Le bras part en ligne droite (comme un tentacule, large de 12 px), annoncé 0,55 s. En phase 3, un second bras part en biais. |
| **Jet de débris** | Phase 2+ | 2 mortiers (3 en phase 3), comme ceux du Tentaculaire. |

- **Phase 2** (66 %) : « le bras grossit ».
- **Phase 3** (33 %) : « perte de contrôle ». De la chair au sol sous Jira toutes les 3 s (ralentit et blesse). Le bras frappe **au hasard** toutes les 2,4 s.
- À sa mort : dialogue `chercheurFin`.

### Étage 4 : pas de gardien
La salle du gardien contient un saule pleureur et une vieille dame. Voir `04`.

## 7. Ennemis spéciaux des events

| Qui | Où | Stats |
|---|---|---|
| Membre d'équipage muté | Salle Événement | Kama ou Brainless élite, 20 PV × `1 + 0,3 × (étage − 1)`, masse ×2,2. Lâche son insigne. |
| La gamine mutée | Salle Événement, étage 2 | Brainless élite, 22 PV × le même facteur |
| Dormeur des pièces | Salle Trésor (piège) | Brainless élite, 18 PV × le même facteur |
| La chose | Irruption | Un Kama à **1 PV** qui tue en un coup. Comportement à part, voir `04`. |
| Cible d'entraînement | Kibo | Immobile, increvable, affiche les dégâts cumulés |
| Générateur | Mechakuve, phase 2 | 14 PV, immobile |
