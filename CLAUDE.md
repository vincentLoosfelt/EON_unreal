# EON — consignes pour Claude (portage Unreal du proto pixel)

Ce fichier est écrit pour toi, le Claude qui travaille avec Vincent dans le projet Unreal. Il te dit où tu es, ce qu'on attend, et comment travailler. Le détail du jeu est dans les fichiers numérotés du dossier `PASSATION_UNREAL`.

## La situation

EON est un action-roguelite SF en vue de dessus, hommage aux animes et aux jeux des années 80-90, développé par un petit studio indépendant (Briscard Prod). L'équipe : Sabri (direction artistique, chef de projet) et Vincent (programmeur, c'est avec lui que tu travailles), plus des artistes 3D.

Le jeu avait une version 3D sur Unity. En parallèle, Sabri a construit avec un autre Claude un **proto en pixel art** (un seul fichier HTML, 3 800 lignes) pour tester les mécaniques en les jouant. Ce proto a beaucoup avancé : une soixantaine de versions entre le 16/10 et le 03/11/2026, quatre étages, trois boss, une méta-progression. L'équipe l'aime tel qu'il est.

Décision du 13/11/2026 : on passe sur Unreal, et on **refait ce proto en 3D depuis zéro**, sans reprendre le code Unity. Le but est de comparer les deux versions côte à côte.

## Ce qu'on attend de toi

Un jeu Unreal qui **se joue comme le proto pixel v0.60.1** : mêmes règles, mêmes chiffres, mêmes sensations, avec les modèles 3D du studio à la place des sprites, et des placeholders là où le modèle n'existe pas encore.

Ce n'est pas le moment d'améliorer le design. Si une règle du proto te paraît bancale, reproduis-la quand même et note ta remarque dans `MANQUANTS.md` (section « Remarques de design »). Sabri tranchera. La comparaison n'a de valeur que si les deux versions font la même chose.

## Où est la vérité

Quand deux sources se contredisent, voici l'ordre :

1. **Le code du proto** : `REFERENCE_PROTO/eon-gunfeel.html`. C'est la seule source qui ne se trompe jamais sur ce que fait le proto. Tout est dans une balise `<script>`. Les noms de fonctions utiles sont cités dans chaque fichier de ce dossier pour que tu les retrouves par recherche.
2. **Les JSON de `DONNEES/`** : extraits automatiquement du code. Exacts pour les tables.
3. **Les fichiers `01` à `09`** : écrits à la main à partir du code. Ils expliquent les règles et donnent les chiffres, mais une erreur de recopie est possible. En cas de doute, lis la fonction dans le code.
4. Les captures et les PNG : pour l'aspect. `PLACEHOLDERS_PIXEL/_FORMAT_DES_PNG.md` dit ce que représente chaque frame.
5. `REFERENCE_PROTO/JOURNAL_DU_PROTO.md` et `LORE_ACTUEL.md` : le journal des versions et le lore, pour comprendre pourquoi une règle existe. Ils contiennent des idées abandonnées et des questions ouvertes : ne t'en sers pas comme spécification.

Vincent peut aussi ouvrir `REFERENCE_PROTO/EON_v0.60.1_DEV.html` dans un navigateur et te décrire ce qu'il voit, ou t'envoyer une capture.

## Les unités

Le proto compte en pixels (écran interne 480 × 270, tuiles de 16 px). Règle de conversion pour tout le projet :

**1 tuile = 16 px = 100 cm Unreal, donc 1 px = 6,25 cm.**

Jira fait 29 px de haut dans le proto, soit 1,81 m. Garde les données en unités proto (px, px/s) dans les JSON, et convertis en un seul endroit du code avec une constante. Comme ça, un réglage changé dans le proto se reporte tel quel.

## Comment travailler avec Vincent

- **Il parle français, toi aussi.** Ton direct, réponses courtes.
- **Avance par jalons** (`08_PLAN_DE_CONSTRUCTION.md`). Un jalon = quelque chose de jouable que Vincent peut comparer au proto. Ne commence pas le jalon suivant avant qu'il ait joué celui en cours.
- **Dis ce que tu n'as pas pu vérifier.** Tu ne peux pas lancer l'éditeur ni jouer. Quand du code compile mais n'a pas été joué, dis-le. Quand tu as besoin d'une manipulation dans l'éditeur (importer un FBX, créer un asset, brancher un mesh), donne à Vincent la liste précise des étapes.
- **Tu écris du texte.** Le C++, les `.ini`, les JSON, les scripts Python de l'éditeur sont à ta portée. Les `.uasset` (Blueprints, matériaux, niveaux) sont binaires. Privilégie donc le gameplay en C++ et les données dans des fichiers texte, et garde les Blueprints fins (un Blueprint qui hérite d'une classe C++ pour y brancher les meshes et les anims). Si Vincent préfère une autre répartition, suis-le. Si sa version d'Unreal fournit un outil pour piloter l'éditeur depuis un assistant, vérifie dans la doc de sa version ce qu'il sait faire avant de compter dessus.
- **Tout doit rester réglable.** Le proto a un banc d'une trentaine de réglages modifiables en jeu (`DONNEES/reglages_gamefeel.json`). C'est ce banc qui a permis de trouver le bon feeling. Expose les mêmes réglages (console, DataAsset ou widget de debug) dès le jalon 1.
- **Les données dans des fichiers, pas dans le code.** Greffes, ennemis, objets, déblocages, dialogues : charge-les depuis `DONNEES/` (ou des DataTables générées depuis ces JSON).

## Les assets

- Les modèles, anims et textures sont dans le dossier `3D` du studio. Vincent les importe. `07_ASSETS_3D.md` dit quel fichier sert à quoi.
- **Quand un modèle manque, mets un placeholder et continue.** Ne bloque jamais un jalon sur un asset. Deux sortes de placeholder, décrites dans `07_ASSETS_3D.md` : un sprite pixel du proto affiché en billboard (pour les créatures et les PNJ), ou une forme simple colorée (pour le décor).
- Chaque placeholder posé s'ajoute à `MANQUANTS.md`, avec la taille attendue et ce qu'il représente. C'est la liste que Sabri et les artistes suivront pour produire les vrais assets.
- Tout placeholder doit pouvoir être remplacé sans toucher au code : le mesh ou le sprite se choisit dans une donnée (DataAsset, table), pas en dur.

## Ce qu'il ne faut pas faire

- Ne reprends rien du projet Unity.
- N'invente pas de contenu (mob, objet, salle, dialogue) qui n'est pas dans le proto. Le proto a déjà eu ce problème une fois, et ça a coûté un retour arrière.
- N'intègre pas de toi-même les décisions listées dans `09_APPORTS_DU_PROTO.md` section 3 (3 cœurs, une seule greffe au départ, etc.). Elles sont prises mais pas encore dans le proto. Prévois juste que ces valeurs soient réglables.
- Ne passe pas de temps sur le rendu final (shaders, post-process, éclairage soigné) avant que le jalon 7 soit jouable. Un éclairage simple aux couleurs de l'étage suffit.

## Vocabulaire

| Terme | Sens |
|---|---|
| Jira | Le personnage joué. Un clone, mi-cybernétique, qui mute. |
| Cobra | Le bras-arme de Jira. |
| Greffe | Un mode de tir du Cobra (Standard, Rafale, Laser, Lame, Shotgun, plus Griffe, Rayon, Fauche). |
| Kibo | Le hub : la salle de réveil d'où part chaque run. |
| C.O.M | La sphère-ascenseur. Elle arrive au centre de chaque étage et sert de terminal. « C.O.M » et « Kibo » désignent le même lieu dans le lore. |
| Gardien | Le boss d'un étage. Sa salle a le rôle `ASCENSEUR` dans le code (nom hérité d'une ancienne version). |
| ADN | L'expérience. Perdue à la mort. |
| Scrap | La monnaie de la run (marchand). Perdue à la mort. |
| Données | La monnaie permanente, gardée entre les runs. |
| Nuke, Onibi, Kama, Ikiryo, Ubume | Les noms officiels des mobs. Dans le code ils s'appellent encore `goule`, `kamikaze`, `slasher`, `sniper`, `invocateur`. Dans le dossier 3D : Ghoul, Kamikaze, Slasher, Shooter, Mage. |
