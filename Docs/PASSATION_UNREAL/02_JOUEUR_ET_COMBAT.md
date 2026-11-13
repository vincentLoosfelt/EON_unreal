# 02 — Le joueur et le combat

Unités proto : px, px/s, secondes. Conversion : 1 px = 6,25 cm.
Données : `DONNEES/greffes.json`, `DONNEES/reglages_gamefeel.json`.
Dans le code : `basePlayer()`, `newPlayer()`, `update()` (blocs `--- input ---`, `--- mouvement / dash ---`, `--- énergie / mutation ---`, `--- armes ---`), `GREFFES`, `hitEnemy()`, `hurtPlayer()`.

## 1. Contrôles

| Action | Clavier / souris | Manette Xbox |
|---|---|---|
| Se déplacer | ZQSD (ou flèches) | Stick gauche |
| Viser | Souris | Stick droit |
| Tirer | Clic gauche (maintenir pour charger Rafale, Laser, Rayon) | RT. Pousser le stick droit à fond tire aussi. |
| Dash | Espace ou Maj | A ou LT |
| Courir | Maintenir la touche du dash | Maintenir A ou LT |
| Onde | E ou clic droit (appui court ou maintenu) | RB ou LB |
| Hologramme | F | X |
| Ultime | R | Y |
| Changer de greffe | 1 à 5 | Croix gauche / droite |
| Interagir, parler, C.O.M | C ou Entrée | B |
| Grande carte | Tab ou M (maintenu) | Vue (Back) |
| Pause | P ou Échap | Menu (Start) |
| Dialogue : suite / passer tout | Espace, E, Entrée / Échap | A / B |

Les touches clavier sont lues par position physique : ZQSD sur un clavier français, WASD sur un clavier américain.

**Invites à l'écran** : clavier par défaut. Dès qu'un bouton ou un stick de manette bouge, toutes les invites passent aux icônes Xbox, et inversement à la première touche clavier.

**Visée au stick** : zone morte 0,18. Le jeu mémorise le dernier angle « franc » (stick poussé à plus de 55 %) et le garde pendant une charge, une salve, et 0,45 s après le relâchement. Sans ça, le retour du stick au centre fait tirer dans le sens de la marche.

**Aide à la visée** (`aimAssist` = 0,15) : si un ennemi est à moins de 220 px et dans un cône autour de la visée, l'angle est tiré vers lui. La force dépend du niveau d'armure : ×0,4 / 0,6 / 0,8 / 1 pour les niveaux 0 à 3.

## 2. Jira : valeurs de départ

| Stat | Valeur |
|---|---|
| PV | 6 (affichés en 6 carrés), plus le bonus permanent des insignes (0 à 2) |
| Vitesse | 82 px/s |
| Accélération | la vitesse rejoint la cible avec un lissage `k = min(1, dt × 16)` : quasi instantané, mais pas sec |
| Énergie | 100 |
| Charges de dash | 2 |
| Rayon de collision | 4 px |
| Greffes disponibles au tout début | Standard et Rafale (les autres se débloquent, voir `05`) |

Tout coup reçu fait **1 PV** (2 si Jira est en mutation). Après un coup : 1 s d'invulnérabilité, recul de 120 px/s à l'opposé de la source, flash rouge, tremblement.

## 3. Dash et course

- **Dash** : à l'appui. Direction = le déplacement en cours, ou la visée si Jira est immobile. Vitesse 345 px/s pendant 0,15 s. Invulnérable 0,2 s. Pendant le dash, Jira **ne tombe pas dans le vide** : c'est le seul moyen de franchir un trou.
- **Recharge** : une charge toutes les 1,1 s. Les charges se rechargent une par une.
- **Course** (façon Hades) : si on garde le bouton enfoncé plus de 0,2 s et qu'on se déplace, Jira court à ×1,7 (`runMul`). En course : pas de tir, pas de visée (elle regarde où elle court), toute charge en cours est annulée.
- Effets : traînée de silhouettes fantômes pendant le dash, poussière pendant la course.

## 4. Le vide

- Jira tombe si elle est sur une tuile de vide **et** qu'au moins 3 de ses 4 points d'appui sont au-dessus du vide (points à ±2,5 px en x, −1,5 et +2 px en y). Ça laisse frôler un bord sans tomber.
- Chute : 0,5 s, puis retour au **dernier point sûr**, −1 PV (`fallDmg`), 1,2 s d'invulnérabilité.
- Le point sûr est mémorisé à chaque instant où aucun point dans un rayon de 6 px autour de Jira n'est au-dessus du vide. L'équipe a demandé (06/11) que le retour ne se fasse pas juste au bord du trou : ce rayon de 6 px est le garde-fou actuel.
- Le recul des tirs peut pousser un **ennemi** dans le vide : il meurt. C'est une arme (voir `03`).

## 5. Les greffes

Une greffe = un mode de tir du bras. On en a une active à la fois, on change à volonté parmi celles qu'on possède.

| Greffe | Type | Cadence | Dégâts | Coût énergie | Détail |
|---|---|---|---|---|---|
| **Standard** | Auto | 9 tirs/s | 1 | 0,7 | Projectile 250 px/s, vie 0,9 s, dispersion 3° |
| **Rafale** | Charge puis salve | charge 0,7 s | 1 par balle | 0,9 par balle | De 2 balles (charge minimale) à 8 (charge pleine), une toutes les 0,045 s. 235 px/s, vie 0,8 s, dispersion 14° |
| **Laser** | Rayon chargé | charge 0,6 s | 6 × le taux de charge (min 1) | 10 × (0,4 + 0,6 × charge) | Ligne instantanée de 300 px, s'arrête au mur, traverse tous les ennemis. Ne part pas sous 20 % de charge. |
| **Lame** | Mêlée | 3,2 coups/s | 3 | 0 | Arc de 2,0 rad, portée 22 px. **Détruit les tirs ennemis** dans l'arc. Petite avancée de Jira (40 px/s). |
| **Shotgun** | Gerbe | 1,8 tirs/s | 1 par plomb | 3 | 7 plombs, cône de 40°, 240 px/s ± 20 %, vie 0,32 s |
| **Griffe** | Fouet en ligne | 2,4 coups/s | 2 | 1,6 | Ligne de 78 px, largeur 5 px. Touche tout sur la ligne et **ramène** les cibles vers Jira (recul inversé ×1,3). |
| **Rayon** | Rayon chargé | charge 0,5 s | 5 × charge | 9 × (0,4 + 0,6 × charge) | Comme le Laser, 220 px |
| **Fauche** | Mêlée | 1,6 coups/s | 4 | 2 | Arc de 3,4 rad, portée 32 px |

Dans le proto, Griffe, Rayon et Fauche appartiennent au « Cobaye », un second perso. Voir la section 11.

**Règles communes**
- Le projectile part du bout du bras : 12 px devant Jira, à 17 px de hauteur.
- Chaque projectile a une vitesse tirée entre 94 % et 106 % de la valeur.
- En mutation (stade 1 ou plus), la cadence est multipliée par 0,8.
- Changer de greffe annule la charge et la salve en cours.
- Un projectile meurt au premier mur ou au premier ennemi, sauf perks (perforation, ricochet).
- La Lame ne coûte pas d'énergie et ne compte pas comme une « dépense » : frapper à la Lame ne nourrit pas la mutation (section 7).

**Recul et mise en scène d'un tir** (`onShot`) : flash de bouche 0,06 s, la caméra recule de `kick × 1,6` px à l'opposé du tir, Jira recule de `kick × 14` px/s, le réticule s'écarte, petite secousse (`0,04 × kick`), une douille est éjectée sur le côté et rebondit au sol. La valeur `kick` de chaque greffe est dans le JSON (Standard 1, Shotgun 3,6).

## 6. L'énergie

- 100 points. Chaque tir et chaque sort en consomme.
- Elle remonte de **16 par seconde**, après 0,7 s sans rien dépenser.
- Une salle nettoyée lâche un ramassable d'énergie au centre. Les ennemis en lâchent une fois sur quatre. Un ramassable rend 14.
- La barre clignote sous 25 %.

## 7. La mutation

C'est le cœur du système. Quand l'énergie tombe à **zéro**, Jira ne s'arrête pas de tirer : elle **mute**.

**Montée**
- Énergie à 0 → stade 1.
- En mutation, tirer et lancer des sorts ne coûte plus rien, mais chaque action compte comme une « dépense ».
- Tant qu'on a dépensé dans les 2,5 dernières secondes, un chrono tourne. Toutes les **4 s** (`mutDur`), le stade monte : 1 → 2 → 3.

**Descente**
- Après 2,5 s sans rien dépenser, le stade baisse de 1 toutes les 1,2 s.
- Revenue à 0 (« état sain »), Jira récupère 30 % de son énergie.

**Effets par stade**

| Stade | Effets |
|---|---|
| 1 | Cadence ×0,8. **Les coups reçus font double dégât.** Écran teinté rouge, parasites. |
| 2 | Vitesse ×0,8. **Onde et hologramme bloqués.** Aberration chromatique. |
| 3 | La visée tremble (±0,175 rad). Jira dérape (bruit de ±20 px/s sur sa vitesse). Gros parasites. |

**L'armure de mutation** : la mutation laisse une trace.
- Au retour à l'état sain, si le stade le plus haut atteint dépasse le niveau d'armure actuel, Jira gagne **+1 niveau d'armure permanente** (pour la run), jusqu'à 3.
- Par niveau d'armure : 20 % de chances de **bloquer** un coup (`armBlock`), mais −12 % d'énergie max (`armCost`) et des stades de mutation 15 % plus rapides (`armMut`).
- Le corps de Jira change : 0 combinaison nue, 1 le bras se transforme, 2 l'armure pousse, 3 casque fermé. Le niveau affiché est le plus haut entre l'armure permanente et le stade de mutation en cours.
- Le HUD de ciblage évolue avec : au niveau 2, un bip quand une cible est verrouillée ; au niveau 3, « ciblage assisté ».

**Objets qui modifient la mutation** : Catalyseur Berserk (+35 % de dégâts par stade), Stabilisateur (stades 2× plus longs), Régulateur (bloquée au stade 1).

## 8. Les sorts

**L'onde** (E / RB)
- **Appui court** : onde de choc autour de Jira. Rayon 56 px, 1 dégât, gros recul (×3,4), efface les tirs ennemis dans le rayon. Coût 18, recharge 0,9 s. C'est l'outil pour pousser un groupe dans le vide.
- **Maintenu plus de 0,25 s** : onde continue. Une pulsation toutes les 0,22 s : rayon 44 px, 1,2 dégât, recul ×1,7. Coût 30 par seconde. Jira avance à moitié vitesse.
- Sur un boss : les dégâts passent, pas le recul.

**L'hologramme** (F / X)
- Pose un leurre à la place de Jira : 5 s, 8 PV. Les ennemis à moins de 240 px le visent à la place de Jira. Coût 28, recharge 7 s.
- Les tirs ennemis qui le touchent lui enlèvent 1 PV. Les ennemis au contact l'usent (5 PV/s pour les mobs de mêlée, 2 pour les autres). L'explosion d'un Onibi lui enlève 6.

**L'ultime** (R / Y) : « Protocole thoracique »
- La jauge monte de **0,8 par point de dégât infligé**, jusqu'à 100.
- À 100, l'appui lance un **QTE** : 5 directions tirées au hasard, à saisir en 3,2 s. Le jeu est figé pendant le QTE.
- Raté ou trop lent : la jauge retombe à 50.
- Réussi : arrêt sur image d'1 s avec une illustration plein écran (cut-in, 1,1 s), puis un rayon tombe sur **chaque ennemi de la salle**, l'un après l'autre (0,15 s puis +0,09 s par ennemi). 14 dégâts par frappe, 30 sur un boss.

Onde et hologramme sont bloqués au stade 2 de mutation.

## 9. Infliger des dégâts

Dans `hitEnemy()` :
- L'ennemi perd `dégâts × multiplicateurs` PV, clignote en blanc 0,07 s.
- **Recul** : `95 × knock × facteur de l'attaque ÷ masse de l'ennemi` px/s, qui s'amortit vite. Le facteur vaut 1 pour une balle, 1,6 pour un rayon, 2 pour la Lame, 3 pour une explosion, 3,4 pour l'onde courte.
- Un chiffre de dégâts s'envole (jaune si ≥ 3).
- **Hitstop** : à la mort d'un ennemi, tout le jeu se fige 55 ms (`hitstop`). Un coup à 3 dégâts ou plus fige 30 % de cette durée. Une explosion : 60 %. Un coup reçu par Jira : 120 % + 30 ms.
- **Tremblement d'écran** (« trauma ») : chaque événement ajoute une valeur (tir 0,04 × kick, mort 0,18, explosion 0,45, coup reçu 0,55, mort de boss 1). Elle décroît de 1,8 par seconde.

## 10. La caméra

- Elle suit Jira avec un lissage (`min(1, dt × 7)`).
- **Anticipation** : elle se décale de 22 px (`lookahead`) dans la direction visée.
- **Kick** : chaque tir la pousse à l'opposé, et elle revient vite.
- Contre un gardien : elle cadre la cuve pendant l'intro, puis se place à mi-chemin entre Jira et le boss.
- Elle reste dans les limites de la salle.

## 11. Le Cobaye, et le principe retenu pour la 3D

Dans le proto, on peut incarner un second perso, **le Cobaye**, débloqué après le premier gardien : 7 PV, 80 px/s, un seul dash mais rechargé en 0,75 s, et ses trois greffes à lui (Griffe, Rayon, Fauche). La mutation est sa force : +20 % de dégâts par stade, pas de double dégât reçu, pas de ralentissement, sorts bloqués seulement au stade 3, 12 % de chances de regagner 1 PV par ennemi tué en étant muté.

**Décision du 06/11/2026 (à appliquer en 3D)** : il n'y a qu'un seul personnage, Jira. Un « perso » n'est qu'un **kit de départ** : une greffe de départ, un passif, des stats, un skin. Jamais un gameplay à part. Tous les perks et toutes les greffes marchent pour toutes les variantes.

Pour le portage :
- Code les 8 greffes comme un seul système, toutes utilisables par Jira.
- Code le « perso » comme une donnée (PV, vitesse, dash, greffes de départ, modificateurs de mutation, skin). Jira est la première entrée. Le Cobaye est une deuxième entrée, à ajouter au jalon 8, avec le modèle de Jira reteinté en rouge comme placeholder.

## 12. Le banc de réglages

Les réglages de `DONNEES/reglages_gamefeel.json` doivent être modifiables en jeu. Les plus utiles :

| Clé | Rôle | Valeur |
|---|---|---|
| `shake` | Force du tremblement | 1 |
| `hitstop` | Arrêt sur image à la mort d'un ennemi (ms) | 55 |
| `knock` | Recul des ennemis | 1 |
| `recoil` | Recul de Jira au tir | 1 |
| `camKick` | Kick caméra au tir | 1 |
| `lookahead` | Anticipation de la caméra (px) | 22 |
| `bulletSpeed`, `spread`, `rateMul` | Multiplicateurs de vitesse, dispersion, cadence | 1 |
| `energyCost` | Multiplicateur du coût en énergie | 1 |
| `mutDur` | Durée d'un stade de mutation (s) | 4 |
| `aimAssist` | Aide à la visée | 0,15 |
| `runMul` | Vitesse de course | 1,7 |
| `fallDmg` | Dégâts de chute | 1 |
| `bossHp` | Multiplicateur des PV de boss | 1 |
| `armBlock`, `armCost`, `armMut` | Armure de mutation, par niveau | 0,2 / 0,12 / 0,15 |
| `god` | Invincible (test) | non |

Le proto a trois préréglages : « sec » (tout à zéro, pour sentir la différence), « eon » (les valeurs du studio) et « max ». Garder « sec » est utile : c'est en coupant tout qu'on voit ce que le game feel apporte.
