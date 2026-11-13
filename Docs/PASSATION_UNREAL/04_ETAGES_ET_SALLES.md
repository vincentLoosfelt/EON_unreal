# 04 — Étages et salles

Unités : tuiles (1 tuile = 16 px = 100 cm) et px. Données : `DONNEES/salles.json`.
Dans le code : `genFloor()`, `genTiles()`, `comTiles()`, `buildRoom()`, `loadRoom()`, `enterCell()`, `setInfest()`, `updateRift()`, `eventsUpdate()`, `buildTraining()`, `buildPasserelle()`, `buildDock()`.

## 1. Les types de tuile

| Code | Nom | Règle |
|---|---|---|
| `F` | Sol | On marche |
| `W` | Mur | Bloque tout, y compris les tirs |
| `P` | Vide | On tombe. Les tirs passent. Les volants survolent. |
| `D` | Porte | 2 tuiles de large, au milieu d'un mur. Solide quand la salle est verrouillée. |
| `X` | Décor solide | Caisse, terminal, tonneau : bloque le passage et les tirs |
| `K` | Cuve | Solide. Sert aux cuves de réveil, à la cuve du gardien, au cœur du C.O.M |

Certaines tuiles de vide sont marquées « passage » (`gap`) : ce sont des fossés de 2 tuiles, prévus pour être franchis au dash. Le générateur les compte comme praticables quand il vérifie que la salle est traversable.

## 2. Générer un étage

Un étage est une grille de 7 × 7 cases. Chaque case occupée est une salle.

**Étapes** (`genFloor`)
1. Le **C.O.M** est au centre (3,3). Ses 4 voisines existent toujours : ce sont les 4 bras.
2. Le **gardien** est tiré au hasard parmi les 4 bras.
3. On fait pousser l'étage depuis les 3 autres bras : chaque case en file d'attente a 50 % de chances de s'étendre vers chaque voisine libre, jusqu'à atteindre la taille visée.

| Étage | Nombre de salles |
|---|---|
| 1 | 14 à 16 |
| 2 | 16 à 18 |
| 3 et 5+ | 17 à 19 |
| 4 | 8 ou 9 |

4. **Les portes suivent l'arbre de pousse.** Deux salles voisines ne communiquent donc pas forcément. On ajoute quelques boucles : chaque paire de voisines non reliées a 10 % de chances de recevoir une porte.
5. **L'autre entrée du gardien** : on cherche les voisines du gardien (autres que le C.O.M) qu'on peut atteindre depuis le C.O.M sans passer par lui, par un chemin d'au moins 3 salles. On en tire une, on la relie au gardien. Ce chemin est le « chemin principal ».
6. Il faut au moins 3 culs-de-sac hors du chemin principal (1 à l'étage 4). Sinon on recommence (jusqu'à 400 essais).

**Distribution des rôles**
1. C.O.M → `DEPART`. Gardien → `ASCENSEUR`.
2. **Carte rouge** (`SECURITE`) : 35 % dans une petite branche collée au chemin principal, sinon sur le chemin lui-même (n'importe où sauf la première salle). Elle est toujours atteignable sans passer par le gardien.
3. Les culs-de-sac les plus éloignés reçoivent, dans l'ordre : `EVENEMENT`, `MARCHAND`, `OPPORTUNITE`.
4. `TRESOR` : 70 % de chances. `CARTO` : 50 % de chances (étages 1 à 3).
5. Le reste : `COMBAT` 62 %, `TENSION` 28 %, `CALME` 10 %.
6. Une salle Combat ou Tension en ligne droite (portes Nord-Sud ou Est-Ouest seulement) devient **le Corridor**. Une de ses voisines devient une `OPPORTUNITE` : le corridor est dur, la salle d'après récompense.
7. Une salle Combat ou Tension devient **la Faille**.
8. 50 % de chances (étages 1 à 3) : une salle Combat ou Tension devient **l'Irruption**.

**Résultat garanti** (vérifié sur 3 000 étages générés dans le proto) : le C.O.M a 4 portes, le gardien lui est collé, il a 2 entrées, la carte rouge est atteignable sans passer par lui, et on va de la carte au gardien sans repasser par le C.O.M.

## 3. Les formes de salle

| Forme | Taille (tuiles) | Pour |
|---|---|---|
| `std` | 32 × 20 | La plupart |
| `large` | 40 × 26 | Combat, Tension, Faille |
| `small` | 24 × 16 | Opportunité, parfois Calme et Combat |
| `corH` | 50 × 9 | Couloir horizontal |
| `corV` | 12 × 32 | Couloir vertical |

Une salle Combat, Tension ou Calme qui n'a que deux portes face à face a 60 % de chances d'être un couloir.

**Intérieur** (`genTiles`)
- Combat, Tension, Calme (hors couloir) : 2 fois sur 3, la forme de base est modifiée. Quatre variantes : `cross` (les 4 coins murés), `L` (un coin muré), `donut` (un bloc de mur au centre), `notch` (des encoches dans les murs).
- **Combat** : au choix, des piliers 2 × 2, des trous, un grand trou central, des caisses, ou un mélange.
- **Tension** : beaucoup de vide. Un gouffre qui coupe la salle avec 1 ou 2 ponts, ou des îlots reliés par des passerelles de 2 tuiles (avec parfois un fossé à dasher), ou une dizaine de trous.
- **Couloirs** : des fossés en travers (à dasher), des trous sur les côtés, des caisses, ou des trous épars.
- Devant chaque porte, un dégagement de 4 × 3 tuiles de sol est garanti.
- **Validation** : toutes les portes doivent être reliées, et au moins 45 % du sol (et 40 tuiles) doit être accessible. Sinon on retire (16 essais), puis on tombe sur une salle vide.

Le détail de chaque variante est dans `genTiles`. C'est une fonction longue mais mécanique : porte-la telle quelle.

## 4. Les rôles de salle

| Rôle | Ce qui s'y passe |
|---|---|
| **C.O.M** (`DEPART`) | Voir section 5. Sûre. |
| **Combat** | Se verrouille. 1 vague pour les 2 premières salles hostiles de la run, 2 vagues ensuite. Première vague 0,6 s après l'entrée. |
| **Tension** | **Ne se verrouille pas.** 1 vague réduite (×0,6). Le danger vient du terrain : on peut traverser sans se battre. |
| **Calme** | Rien. Du décor. À l'étage 4, un cœur. |
| **Opportunité** | 3 objets sur des socles. On en prend un, les deux autres disparaissent. Gratuit. |
| **Carte rouge** (`SECURITE`) | La carte est sur une plateforme 4 × 4, entourée d'un fossé qu'on franchit au dash. La ramasser lance un dialogue puis une **embuscade** : la salle se verrouille, 2 vagues. |
| **Événement** | Un PNJ ou un mini-boss. Voir section 8. |
| **Marchand** | Un mur pivote au nord, le marchand apparaît. 3 objets payants. On peut en acheter plusieurs. Le stock change à chaque rencontre. |
| **Trésor** | Voir section 8. |
| **Irruption** | Voir section 8. |
| **Cartographe** | Voir section 8. |
| **Corridor** | Couloir avec des caisses. Se verrouille. **3 vagues** de corps à corps qui arrivent du fond. |
| **Faille** | Voir section 8. |
| **Gardien** (`ASCENSEUR`) | Le boss. Voir `03`. |

**Nettoyer une salle** : les portes se déverrouillent, un ramassable d'énergie apparaît au centre, bannière « Salle nettoyée ». Si un niveau est en attente, l'écran de choix s'ouvre à ce moment-là.

**Changer de salle** : on franchit une porte ouverte en marchant dedans. Fondu de 0,36 s. Jira arrive devant la porte opposée. L'ADN encore en vol dans la salle qu'on quitte est crédité.

**Chaque salle garde son état** : objets au sol, salle nettoyée ou non. On peut revenir.

## 5. Le C.O.M

**La salle** (`comTiles`) : une salle standard remplie de **vide**. Au centre, une plateforme ovale (rayons 6,3 × 5,3 tuiles). Une passerelle de 2 tuiles de large part vers chaque porte. Au milieu, la sphère du C.O.M (4 × 4 tuiles, solide). Dans le vide défilent les entrailles de la station.

**La sphère** : une « gacha ball » en cassette futurism. Coque basse en métal usé et plastique jauni, dôme de verre fumé ambre, noyau vert d'écran cathodique.

**Le terminal** : à moins de 56 px de la sphère, l'invite « Consulter le C.O.M » apparaît. Le jeu est en pause. 5 onglets (détail dans `06`). L'onglet **Descente** a un levier :
- Verrouillé tant que le gardien vit. Il indique où est le gardien et si on a la carte.
- Prêt ensuite. On le tire : la sphère s'emballe pendant 1,7 s, les entrailles défilent, fondu, étage suivant.

**La porte rouge** : toute porte qui mène chez le gardien est fermée et marquée en rouge tant qu'on n'a pas la carte. Elle s'ouvre dès qu'on ramasse la carte.

**La passerelle infestée** : la passerelle du C.O.M vers le gardien est couverte de chair mutante, infranchissable. Dans le lore, c'est elle qui a arrêté le C.O.M à cet étage. Côté gardien, la même chair mure la porte (la porte et 2 tuiles devant). S'en approcher (moins de 24 px) déclenche une réplique, puis une simple bannière les fois suivantes. **À la mort du gardien**, la chair se rétracte en 1,6 s : on rentre directement au C.O.M.

**À l'arrivée sur un étage** : bannière avec le nom de l'étage, puis un dialogue de l'IA (`iaIntro` la première fois, `cuves`, `labos`, `serres` ensuite). La carte rouge est retirée. Le bouclier de cassette est rechargé.

## 6. Le Kibo : le début d'une run

Trois salles fixes, hors génération.

1. **La salle de réveil** (`buildTraining`, 36 × 20 tuiles). Sept cuves contre les murs : scellées, brisées, et celle de Jira. Quatre **cibles d'entraînement** increvables qui affichent les dégâts. Un fossé de 2 tuiles pour apprendre le dash, un petit trou pour apprendre la chute. Une porte au nord.
   - Première entrée : **cinématique de réveil** (2,6 s). La cuve se vidange, s'ouvre, Jira sort. Puis le dialogue `wake`.
   - Les PNJ du hub s'y tiennent : Ulysse une fois sauvé, le Cobaye une fois débloqué.
2. **La passerelle** (12 × 30 tuiles) : un pont de 4 tuiles de large, du vide de chaque côté.
3. **Le C.O.M à quai** : la même salle que le C.O.M d'étage, avec une seule porte (sud). Le levier est déjà prêt. C'est ici qu'on achète les améliorations de cassette.

## 7. Les quatre étages

| Étage | Nom | Palette | Mobilier | Fin |
|---|---|---|---|---|
| 1 | Les coursives | Chaque salle tire une des trois : Coursive (violet, néon rose), Salle des machines (brun, néon ambre), Labo (sarcelle, néon cyan) | Terminaux, câbles, tuyaux, caisses, tonneaux | Le Tentaculaire |
| 2 | Les cuves | Ocre sale, néon ambre (85 % des salles) | Cuves à spécimens ratés contre les murs nord et sud (2 × 3 tuiles), certaines brisées. Biomasse, flaques. Référence : Alien 4. | Mechakuve |
| 3 | Les labos | **La seule ambiance claire** : carrelage blanc cassé, murs gris clair, écrans bleus. Obscurité réduite. | Postes à 2 écrans, écrans muraux, analyseurs, étagères de bocaux, tables d'opération, un silo cryo dans les grandes salles. Référence : Akira. | Le Chercheur |
| 4 | Les serres | Vert et lumineux : herbe, terre, lampes chaudes | Arbres, bacs de culture, rayonnages hydroponiques, fleurs | Pas de gardien |
| 5+ | (mélange) | Au hasard | | Le Tentaculaire renforcé |

**Règle de pose du mobilier** : un meuble n'est posé que s'il laisse un tour de sol libre et ne coupe aucun passage. Le générateur revérifie l'accessibilité après chaque pose.

**L'étage 4 est une respiration** :
- 8 ou 9 salles. Pas de carte rouge, pas de Corridor, pas de Faille, pas d'infestation.
- Les combats n'ont qu'une vague, à moitié budget.
- Chaque salle Calme offre un cœur.
- La salle du gardien contient un **grand saule pleureur** et une **vieille dame** en blouse, assise sur un banc. Le dialogue `vieille` se lance quand on s'approche (moins de 46 px). Ensuite le C.O.M est prêt. On peut revenir lui parler : elle dit une de ses 4 répliques au hasard.
- Rôles de l'étage 4 : Combat 40 %, Tension 35 %, Calme 25 %.

**Décision de l'équipe (06/11)** : le jeu final aura 4 étages, et la démo 1 seul. Le proto en a déjà 4 jouables, plus un mode sans fin au-delà.

## 8. Les events

### La Faille
Une paroi extérieure cède. Ce n'est pas une salle de combat, c'est une **salle de fuite**.
1. 1,2 s après l'entrée : la paroi se déchire (jamais un mur qui porte une porte). Dialogue `rift`. Les tuiles de la brèche deviennent du vide.
2. À 6 s : la brèche s'élargit.
3. **Aspiration** vers la brèche, deux fois plus forte au deuxième stade. Jira dérive d'environ 32 px/s par stade. Les ennemis (21 px/s) et les ramassables (17 px/s) aussi.
4. **Oxygène** : une jauge qui se vide en 32 s. À zéro : 1 PV toutes les 2 s.
5. Une seule vague (×0,9). Une alarme sonne, de plus en plus serrée.
6. Quand la salle est vide, la **console de confinement** s'active. Elle est sur la tuile de sol la plus éloignée de la brèche, pour forcer la traversée. Un repère au bord de l'écran l'indique. La toucher referme la paroi et libère la salle.

### Le Trésor
Un cul-de-sac. Deux variantes à égalité :
- **Stock** : 10 scrap, 2 Données, et une disquette sur un socle. **4 fois sur 10, c'est un piège** : prendre la disquette réveille le « Dormeur des pièces » (élite) et verrouille la salle.
- **Carnage** : des corps, 27 ADN gratuit (9 ramassables de 3), 2 ramassables d'énergie.

### L'Irruption
Une salle avec des cuves, dont une vide.
1. 1,8 s après l'entrée, **les lumières lâchent**. Il ne reste que la lampe de Jira.
2. **La chose** apparaît loin de Jira. Elle a 1 PV.
3. Elle **rôde** : elle tourne autour de Jira à distance (110 × 80 px) pendant 2,5 à 4 s.
4. Elle s'arrête, deux yeux rouges s'allument, elle rugit (0,65 s).
5. Elle **charge** à 240 px/s pendant 0,75 s, tout droit. Puis elle retourne rôder.
6. **Un contact tue.** Seuls le bouclier de cassette et la Renaissance sauvent. Un coup suffit aussi pour la tuer.
7. Elle meurt : la lumière revient, un objet rare ou épique sur un socle, 3 Données.

### Le Cartographe
Un gamin des conduits, dans un cul-de-sac.
1. À moins de 70 px : dialogue `cartoStart`. La salle se verrouille, 2 vagues.
2. Il a 6 PV. La moitié des ennemis le visent, lui. Chaque contact lui enlève 1 PV (au plus un toutes les 0,8 s).
3. S'il survit : dialogue `cartoWin`, et **toute la carte de l'étage est révélée**, avec le rôle de chaque salle.
4. S'il meurt : dialogue `cartoLost`, rien.

### Ulysse et son équipage (salle Événement)
- **Première fois** : Ulysse, capitaine de la station, est menacé par un Kama élite, l'« Ex-quartier-maître ». La salle est verrouillée. Une fois l'élite mort, on parle à Ulysse (`ulysse1`). Il donne l'insigne du quartier-maître et un objet rare ou épique. Il part se réfugier au **Kibo**, où il reste pour toujours.
- **Ensuite** : la salle Événement est gardée par un **membre d'équipage muté** (élite Kama ou Brainless, nommé). Il lâche son **insigne**. Six membres en tout, jamais deux fois le même. Une fois tous trouvés : un « survivant muté » sans insigne.
- Les insignes sont gardés même si on meurt. On les rend à Ulysse au Kibo, au début de la run suivante. Récompenses dans `05`.

### La gamine (salle Événement de l'étage 2, une seule fois)
- À l'étage 1, une **peluche** (Lapin) est cachée dans une salle.
- À l'étage 2, une gamine recroquevillée devant une grille d'aération.
- **Avec la peluche** (`newtGood`) : elle se calme et laisse un objet rare ou épique.
- **Sans** (`newtBad`) : elle mute en élite. Une fois tuée : « Elle ne souffre plus », et un objet (épique si possible).

### Les logs
Un par étage, posé dans une salle au hasard. Le ramasser affiche le texte **en surimpression, sans pause**, lettre par lettre. Huit logs, lus dans l'ordre d'une run à l'autre. Textes dans `DONNEES/dialogues.json`.
