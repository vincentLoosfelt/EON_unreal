# 08 — Plan de construction

Onze jalons. Chacun se termine par quelque chose de **jouable**, que Vincent compare au proto. On ne commence pas un jalon avant que le précédent ait été joué.

L'ordre suit une logique simple : d'abord ce qui fait le plaisir du jeu (tirer, bouger), puis un étage complet (le périmètre de la démo), puis la méta, puis le reste du contenu. Les jalons 1 à 7 forment la boucle de l'étage 1. C'est la première vraie comparaison.

Pour chaque jalon : **le but**, **ce qu'on construit**, **avec quoi comparer** dans le proto, et **c'est bon quand**.

Dans la build `_DEV` du proto, le panneau de droite permet d'aller à l'étage suivant d'un clic, de tout débloquer et d'être invincible : pratique pour aller voir un point précis.

---

## Jalon 0 — Le socle

**But** : un projet qui compile, avec les fondations dont tout le reste dépend.

- Projet Unreal vide, module C++ de jeu.
- La constante d'échelle (1 px = 6,25 cm) et les fonctions de conversion, à un seul endroit.
- Le chargement des fichiers de `DONNEES/`. Décide avec Vincent : lire les JSON au démarrage, ou générer des DataTables. Dans les deux cas, les JSON restent la source.
- Les réglages de game feel (`reglages_gamefeel.json`) dans un objet modifiable en jeu.
- La caméra plongeante fixe, qui montre 30 m de large.
- Les entrées clavier/souris et manette, avec la détection du dernier périphérique utilisé.
- Une salle de test : un sol de 32 × 20 m, des murs.
- Le composant « placeholder sprite » (`07`, section 6).

**C'est bon quand** : Vincent lance le jeu et voit une salle vide sous le bon angle, et un réglage changé en jeu est pris en compte.

---

## Jalon 1 — Jira qui tire

**But** : le cœur du plaisir. C'est le jalon le plus important : prends le temps de le régler.

- Jira : modèle, déplacement, visée souris et stick, anims idle / marche / tir.
- Le dash (2 charges, recharge, invulnérabilité) et la course en maintenant le bouton.
- La greffe **Standard** : projectiles, cadence, dispersion, portée.
- Des **cibles d'entraînement** increvables qui affichent les dégâts.
- **Tout le game feel du tir** : flash de bouche, douille, recul de Jira, kick et anticipation de la caméra, tremblement, flash blanc à l'impact, chiffres de dégâts, hitstop.
- La barre d'énergie et sa recharge.
- L'aide à la visée.

**Comparer avec** : la salle de réveil du Kibo. Tirer sur les cibles, dasher, courir.

**C'est bon quand** : Vincent met les deux jeux côte à côte et le tir « pèse » pareil. Teste aussi le préréglage « sec » (tout le feel à zéro) : la différence doit sauter aux yeux, comme dans le proto.

---

## Jalon 2 — Le terrain

**But** : les salles en tuiles et le vide.

- Une salle construite à partir d'une grille de tuiles (sol, mur, vide, porte, décor solide, cuve).
- Les collisions : murs et décor bloquent Jira et les tirs. Le vide ne bloque pas les tirs.
- **La chute** : la règle des 3 points sur 4, le retour au dernier point sûr, −1 PV. Le dash franchit le vide.
- Le générateur de salle `genTiles` pour les rôles Combat, Tension et Calme, dans les 5 formes.
- Les portes : ouvertes, verrouillées.
- La salle de réveil du Kibo en dur (`buildTraining`) avec son fossé à dasher.

**Comparer avec** : traverser une dizaine de salles Tension dans le proto. Regarder les fossés, les îlots, les gouffres.

**C'est bon quand** : on tombe et on dashe aux mêmes endroits, et le générateur ne produit jamais une salle dont une porte est inaccessible.

---

## Jalon 3 — Les ennemis

**But** : des combats.

- La base commune : PV, recul selon la masse, mort, butin, apparition annoncée.
- Le champ de distances pour le déplacement (`computeFlow`).
- **La chute des ennemis** : un ennemi projeté tombe dans le vide et meurt.
- Les 7 mobs, dans cet ordre : Nuke (avec la nuée), Onibi, Kama, Ikiryo, Méduse, Brainless, Ubume (avec ses œufs et la petite Nuke).
- Les projectiles ennemis, et les coups reçus par Jira (invulnérabilité, recul, flash).
- Les vagues (`spawnWave`) : budget, coûts, seuils, vague suivante quand il reste un ennemi.
- Les élites.
- La salle Combat complète : elle se verrouille, ses vagues, elle se libère.
- La mort de Jira et l'écran « Signal perdu ».

Placeholders : sprites pixel pour la Méduse et le Brainless, et pour tout mob dont le FBX n'est pas encore exporté.

**Comparer avec** : des salles Combat à l'étage 1, puis à l'étage 2 pour voir tous les mobs. Pousser des ennemis dans le vide avec le tir.

**C'est bon quand** : chaque mob se reconnaît à son comportement sans regarder son apparence, et tuer un ennemi en le poussant dans un trou fonctionne.

---

## Jalon 4 — Les systèmes de Jira

**But** : tout ce qui fait de Jira « un vaisseau à gérer ».

- Les 7 autres greffes : Rafale, Laser, Lame, Shotgun, Griffe, Rayon, Fauche. Le changement de greffe.
- **La mutation** : les 3 stades, leurs effets, la montée et la descente.
- **L'armure de mutation** : les 3 niveaux, le blocage, le coût en énergie.
- L'**onde** (courte et maintenue), l'**hologramme**, l'**ultime** avec son QTE et son cut-in.
- Les effets plein écran de la mutation.
- Le HUD du joueur complet (`06`, section 1).

**Comparer avec** : vider son énergie volontairement et rester en mutation jusqu'au stade 3, puis redescendre et constater l'armure. Lancer l'ultime dans une salle pleine.

**C'est bon quand** : on ressent le même dilemme que dans le proto. Continuer à tirer fait muter, s'arrêter fait redescendre, et la mutation est à la fois un danger (double dégât) et un gain (l'armure).

---

## Jalon 5 — L'étage

**But** : un étage entier à explorer.

- `genFloor` : la grille, le C.O.M au centre, le gardien sur un bras, l'autre entrée, la distribution des rôles.
- Le passage d'une salle à l'autre, la mémoire de l'état de chaque salle.
- **La salle du C.O.M** : la plateforme sur le vide, les 4 passerelles, la sphère.
- **L'infestation** de la passerelle du gardien, et sa rétractation.
- **La carte rouge**, la porte rouge, l'embuscade.
- La mini-carte et la grande carte.
- Les rôles simples : Calme, Opportunité (vide pour l'instant), Tension.
- Le terminal du C.O.M avec l'onglet Descente : le levier, la descente, l'étage suivant.
- Les trois salles du Kibo : réveil, passerelle, C.O.M à quai. La cinématique de réveil.
- La palette par salle pour l'étage 1.

Écris un test automatique du générateur : générer 1 000 étages et vérifier les garanties listées dans `04`, section 2. Le proto l'a fait, et ça a trouvé des défauts que le jeu à la main ne montrait pas.

**Comparer avec** : faire un étage 1 complet dans le proto en regardant la carte. Remarquer que le gardien est toujours collé au C.O.M mais qu'il faut toujours faire le tour.

**C'est bon quand** : on part du Kibo, on descend, on trouve la carte, on atteint la salle du gardien (vide pour l'instant) et on peut redescendre.

---

## Jalon 6 — La progression dans la run

**But** : devenir plus fort au fil des salles.

- L'ADN : les gemmes, l'aspiration, les niveaux, l'écran de choix 1 parmi 3.
- Les 36 objets et leurs effets. Le tirage pondéré et ses filtres.
- Les socles : Opportunité, récompenses.
- Le scrap et le **marchand** (le mur qui pivote, les 3 objets payants).
- Les ramassables : énergie, cœur, scrap.
- L'onglet Améliorations du terminal.

**Comparer avec** : une run jusqu'au niveau 5 ou 6. Regarder à quel rythme les niveaux tombent (environ 3 par étage).

**C'est bon quand** : le choix de niveau ne s'ouvre jamais en plein combat, et un perk de greffe ne sort que si cette greffe est en main.

---

## Jalon 7 — Le gardien de l'étage 1

**But** : boucler l'étage 1. **Fin du périmètre de la démo.**

- Le cadre commun aux gardiens : la cuve, l'intro, le carton-titre, la barre de vie, les phases, la mort, le butin.
- **Le Tentaculaire** : ses 3 phases, ses éventails, sa couronne, ses mortiers, ses tentacules, sa charge.
- Tous les télégraphes.
- La caméra de boss.
- La rétractation de l'infestation, le retour au C.O.M, la descente.

Placeholder : `boss.png` en billboard, des cylindres pour les tentacules.

**Comparer avec** : le gardien de l'étage 1. Le battre plusieurs fois, en regardant chaque phase.

**C'est bon quand** : on fait une run complète de l'étage 1, du réveil à la descente. **C'est le moment de la grande comparaison avec l'équipe.** Prévois une build à faire tourner sur une autre machine.

---

## Jalon 8 — La méta

**But** : ce qui reste entre les runs.

- La sauvegarde (`05`, section 10) et l'écran titre Continuer / Nouvelle partie.
- Les stats cumulées et les 12 déblocages.
- Les Données, et la cassette Shield avec ses 3 niveaux. L'onglet Cassettes.
- Ulysse : la salle Événement, le sauvetage, le refuge au Kibo, les insignes, les PV permanents.
- Le Cobaye comme variante jouable au Kibo.
- Les onglets Quêtes et Logs du terminal.
- Le système de dialogues, les portraits, et tous les dialogues déjà déclenchables.

**Comparer avec** : enchaîner 3 ou 4 runs dans le proto en partant d'une « Nouvelle partie ». Regarder ce qui se débloque et quand.

**C'est bon quand** : on quitte le jeu, on le relance, et tout est là.

---

## Jalon 9 — Les events

**But** : la variété.

- La Faille, le Corridor, le Trésor, l'Irruption, le Cartographe.
- La gamine et sa peluche.
- Les logs à ramasser.

**Comparer avec** : chaque event, un par un. La build `_DEV` permet de changer d'étage jusqu'à tomber dessus.

**C'est bon quand** : la Faille se joue comme une fuite, pas comme un combat, et l'Irruption fait peur.

---

## Jalon 10 — Les étages 2, 3 et 4

**But** : tout le contenu du proto.

- Les palettes et le mobilier de chaque étage, avec la règle de pose.
- **Mechakuve** et ses 4 phases.
- **Le Chercheur** et ses 3 phases.
- L'étage 4 : ses règles à part, le saule, la vieille dame.
- L'étage 5 et au-delà : le mélange et le Tentaculaire renforcé.

**C'est bon quand** : une run va du Kibo aux serres.

---

## Tout au long

- Tiens `MANQUANTS.md` à jour à chaque placeholder posé.
- Quand un chiffre du proto donne un mauvais résultat en 3D (une vitesse qui paraît lente, une portée qui paraît courte), **ne le change pas en dur**. Garde la valeur du proto dans les données, ajoute un multiplicateur dans les réglages, et note-le dans `MANQUANTS.md`, section « Écarts de réglage ». Sabri doit pouvoir voir d'un coup d'œil ce qui a bougé et pourquoi.
- Mets le projet sous contrôle de version dès le jalon 0.
