# 01 — Vision et périmètre

## 1. Le jeu en une phrase

« Un perso qui se joue comme un vaisseau dans FTL, des évolutions comme Vampire Survivors, un mapping comme Binding of Isaac », avec une histoire qui avance run après run comme dans Hades.

- **FTL** : le corps de Jira est un ensemble de systèmes à gérer. L'énergie est partagée entre le tir et les sorts. Quand elle tombe à zéro, la mutation prend le relais.
- **Vampire Survivors** : l'ADN fait monter de niveau, et chaque niveau offre un choix 1 parmi 3.
- **Isaac** : une grille de salles, une carte, des salles spéciales.

Le premier retour de playtest dit que la base du jeu, c'est « la satisfaction de tirer sur des monstres ». Le game feel du tir passe avant tout le reste.

## 2. La boucle d'une run

1. Jira se réveille dans une cuve au **Kibo** (salle de réveil avec des cibles d'entraînement).
2. Elle traverse la passerelle et monte dans le **C.O.M à quai**. Elle tire le levier.
3. Le C.O.M descend et arrive au **centre de l'étage**. Quatre passerelles partent vers quatre portes.
4. La passerelle qui mène au **gardien** est infestée par la mutation : infranchissable. Il faut faire le tour.
5. Sur le chemin, Jira trouve la **carte rouge** (ce qui déclenche une embuscade), puis entre chez le gardien par son autre porte.
6. Gardien vaincu : la chair se rétracte, retour direct au C.O.M, levier, étage suivant.
7. À la mort : retour au Kibo. On garde les Données, les déblocages, les quêtes.

## 3. Ce qu'on refait

**Tout ce qui est dans le proto v0.60.1**, dans l'ordre du plan (`08_PLAN_DE_CONSTRUCTION.md`). La priorité est l'étage 1 complet avec son gardien : c'est le périmètre de la future démo (décision de l'équipe du 06/11 : démo = 1 étage).

| Bloc | Contenu |
|---|---|
| Joueur | Déplacement, visée, dash, course, 8 greffes, énergie, mutation en 3 stades, armure de mutation, onde, hologramme, ultime en QTE |
| Ennemis | 7 mobs, la petite Nuke pondue, les élites, 3 boss |
| Monde | Génération d'étage, 16 rôles de salle, 5 formes de salle, le vide, 4 étages à thème |
| Progression | ADN et niveaux, 36 objets, scrap et marchand, Données, 12 déblocages, cassette Shield |
| Narratif | Dialogues avec portraits, 8 logs, Ulysse et son équipage, la gamine, le cartographe, la vieille dame |
| Interface | HUD, mini-carte, terminal du C.O.M à 5 onglets, écran de level-up, écran de mort, écran titre |

## 4. Ce qu'on ne refait pas

- Le panneau de chargement de PNG et le système de sprites remplaçables (c'était l'outil du pixel art).
- L'érosion de la chair (les tirs percent des trous dans les sprites) : c'est un effet purement 2D. En 3D, un flash et des particules à l'impact suffisent pour l'instant.
- Les contrôles tactiles.
- Le filtre CRT et le « rendu peinture » : à voir plus tard, quand le reste tourne.
- Le « Banc de mobs » et la « Branche greffes » : deux outils de test à part, hors proto.

## 5. L'échelle

**1 tuile = 16 px = 100 cm. 1 px = 6,25 cm.**

| Élément | Proto | Unreal |
|---|---|---|
| Jira (hauteur) | 29 px | 181 cm |
| Rayon de collision de Jira | 4 px | 25 cm |
| Vitesse de marche | 82 px/s | 512 cm/s |
| Dash | 345 px/s pendant 0,15 s (≈ 52 px) | ≈ 323 cm |
| Portée du tir standard | 250 px/s × 0,9 s = 225 px | ≈ 14 m |
| Salle standard | 32 × 20 tuiles | 32 × 20 m |
| Grande salle | 40 × 26 tuiles | 40 × 26 m |
| Couloir | 50 × 9 ou 12 × 32 tuiles | 50 × 9 m ou 12 × 32 m |
| Ce que la caméra montre | 480 × 270 px = 30 × 16,9 tuiles | 30 × 16,9 m au sol |

Les tailles de salle incluent une tuile de mur sur chaque bord.

## 6. Le passage de la 2D à la 3D

Le proto est un plan vu de dessus. Voici comment lire ses coordonnées et ce qui change.

- **Le plan de jeu** : x du proto = X Unreal, y du proto (vers le bas de l'écran) = Y Unreal. Tout le gameplay se passe sur le sol, à Z constant. Pas de saut, pas d'étages superposés.
- **La caméra** : fixe en rotation, plongeante (environ 55 à 60° vers le bas), elle suit Jira. Elle doit montrer environ 30 m de large. Elle ne sort pas de la salle (dans le proto, elle est bridée aux bords de la salle, et centrée si la salle est plus petite que l'écran). Une projection orthographique ou une perspective à focale longue donnent la lecture la plus proche du proto.
- **La visée** : dans le proto, l'angle va de Jira vers le curseur à l'écran. En 3D, projette le curseur sur le plan du sol et vise ce point. Au stick, c'est la direction du stick droit.
- **Les hitbox** : le proto triche avec des capsules verticales (des pieds à la tête) parce que les sprites sont debout sur un plan vu de dessus. En 3D, une capsule normale par personnage fait le même travail. Les rayons du proto (`r`, `hr`) donnent le rayon de la capsule, `hh` sa hauteur.
- **Le vide** : dans le proto, une tuile « vide » est un trou dans le sol. En 3D : pas de sol, un fond qui défile loin dessous, et un volume qui détecte la chute. Les règles sont dans `02` (chute de Jira) et `03` (chute des ennemis).
- **Les projectiles** : ils volent à plat, à hauteur du bras de Jira (17 px au-dessus des pieds dans le proto, soit ≈ 105 cm). Les projectiles ennemis volent à la même hauteur. Il n'y a aucune notion d'altitude dans le proto : avec des capsules et des projectiles à hauteur de buste, on obtient le même résultat en 3D.
- **Les volants** (Méduse) : ils flottent, survolent le vide, mais se font toucher comme les autres.

## 7. Ce qui compte comme « pareil que le proto »

À chaque jalon, Vincent joue les deux versions. C'est réussi si :

1. **Les chiffres sont les mêmes** : vitesses, cadences, dégâts, PV, durées, rayons. Ils viennent des JSON.
2. **Les règles sont les mêmes** : ce qui déclenche quoi, dans quel ordre.
3. **Le ressenti est proche** : le tir a du poids (recul, hitstop, tremblement, douilles), le dash est sec, les ennemis s'annoncent avant de frapper.
4. **La lecture est la même** : on voit autant de terrain, on distingue les mêmes choses (un trou, une porte verrouillée, un télégraphe d'attaque).

Le troisième point ne se règle qu'en jouant. Les chiffres du proto sont un point de départ : le passage en 3D changera un peu la perception des vitesses et des distances, et il faudra retoucher. D'où l'importance du banc de réglages.

## 8. Direction artistique (rappel court)

- Référence : animation japonaise et jeux des années 80-90, « cassette futurism » (plastique jauni, écrans cathodiques, verre fumé ambre).
- La version 3D se distingue par un rendu **anime 2D en 3D** (cel shading). Ce n'est pas l'objet de ce portage, mais évite les choix qui le rendraient difficile plus tard.
- Chaque étage a sa palette (`DONNEES/salles.json`, clé `palettes`) : une couleur de sol, de mur, un néon et une teinte d'ombre. L'ombre n'est jamais noire, elle est teintée.
- Captures de référence par étage : `CAPTURES_PROTO/`.
