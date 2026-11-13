# 07 — Les assets 3D : ce qui existe, ce qui manque, les placeholders

**Attention** : ce relevé est fait **d'après les noms de fichiers** du dossier `BRISCARD_PROD/3D`, au 13/11/2026. Les `.blend` et les `.fbx` n'ont pas été ouverts. Vincent doit vérifier à l'import ce que chaque fichier contient vraiment (échelle, orientation, rig, nombre d'anims). Corrige ce document au fil des imports.

Chemins relatifs au dossier `3D`.

## 1. Jira

| Élément | Fichier | État |
|---|---|---|
| Modèle | `FBX/Chara/Cyborg/Cyborg_mode.fbx` | ✅ FBX |
| Textures | `Textures/Textures_Chara/T_Cyborg_V2/` (BaseColor, BaseColor_2, Displacement) | ✅ |
| Rig | `RIG/Cyborg_rig_2.blend` | `.blend` |
| Animations | `FBX/Animation/Cyborg/Jira_*.fbx` | ✅ 16 FBX |

**Les animations et leur usage dans le proto**

| Fichier | Usage |
|---|---|
| `Jira_idle` | Immobile |
| `Jira_walk` | Déplacement normal (82 px/s) |
| `Jira_run` | Course, bouton de dash maintenu (×1,7) |
| `Jira_dash` | Dash (0,15 s) |
| `Jira_shoot_front`, `_back`, `_left`, `_right` | Tir en se déplaçant. Jira vise dans une direction et marche dans une autre : choisis l'anim selon l'angle entre la visée et le déplacement. |
| `Jira_attack_light` | Tir simple à l'arrêt (Standard, Shotgun) |
| `Jira_attack_charged` | Charge puis tir (Rafale, Laser, Rayon). Le dossier `Old` contient une version découpée en préparation / tir / retour : utile pour caler la charge de 0,6 à 0,7 s. |
| `Jira_hit_light`, `_hit_heavy`, `_hit_knockback` | Coup reçu. `knockback` pour les projections (Brainless, coup de bouclier). |
| `Jira_death` | Mort |
| `Jira_hacking` | Interaction : terminal du C.O.M, console de la Faille |
| `Jira_all_anims` | Toutes les anims dans un fichier, au choix de Vincent |

**Ce qui manque pour Jira** (placeholder entre parenthèses)
- Chute dans le vide (réduire l'échelle et faire descendre le mesh pendant 0,5 s).
- Coup de mêlée pour la Lame et la Fauche (rejouer `attack_light` plus vite, avec un arc en effet).
- Griffe : le bras qui se détend en ligne (un cylindre étiré, ou un rayon).
- Lancer de l'onde, pose de l'ultime (`attack_charged`).
- Sortie de cuve pour la cinématique de réveil (`idle`, la cuve fait le travail).
- **Les 4 niveaux d'armure** : combinaison nue, bras transformé, armure, casque. Le modèle actuel est une seule tenue. (Teinter le matériau ou afficher un texte de debug.)
- Les **veines de mutation** par-dessus le corps (teinte rouge qui pulse).
- Un **modèle de bras par greffe**. (Changer la couleur du bras : chaque greffe a sa couleur dans `greffes.json`.)

## 2. Les mobs

| Mob | Modèle | Rig | Animations | Textures | À faire |
|---|---|---|---|---|---|
| **Nuke** (Ghoul) | Dans le rig | `RIG/Ghoul.rig_MASTER.blend` | `.blend` seulement : attack, death, walk | `T_Ghoul_BaseColor.png` | **Exporter en FBX** |
| **Onibi** (Kamikaze) | Dans le rig | `RIG/Kamikaze.rig_MASTER.blend` | `.blend` seulement : walk, run, run_anticipation, turnaround, explosion | `T_Kamikaze_BaseColor.png` | **Exporter en FBX** |
| **Kama** (Slasher) | `Fichier_blend/Slasher.mode_MASTER.blend` | `RIG/Slasher.mode_MASTER.blend` | `.blend` seulement : run, attack, provocation, death, mix | `T_Slasher/` | **Exporter en FBX** |
| **Ikiryo** (Shooter) | ✅ `FBX/Chara/Shooter/Shooter_mode.fbx` | `RIG/Shooter.mode_MASTER.blend` | **Aucune trouvée** | `T_Shooter/` V1 et V2 | Anims à produire |
| **Ubume** (Mage) | `Fichier_blend/Mage.mode_MASTER.blend` | **Aucun trouvé** | **Aucune trouvée** | `T_Invocateur/` (corps, lunettes, tentacules) | Export, rig, anims |
| **Méduse** | ❌ | | | | Tout. Le design 2D n'est pas arrêté non plus. |
| **Brainless** | ❌ | | | | Tout. Jamais dessiné. |
| Nuke (petite) | Réutiliser la Nuke à 60 % | | | | Rien |
| Élites | Réutiliser le mob à 130 %, avec une teinte | | | | Rien pour l'instant |

**Les anims dont chaque mob a besoin, d'après son comportement**

| Mob | Anims |
|---|---|
| Nuke | Marche, attaque au contact, mort. (Les 3 existent.) |
| Onibi | Course, **armement** (0,45 s avant l'explosion : `run_anticipation` ou `turnaround` ?), explosion. |
| Kama | Marche, **préparation** (0,55 s : `provocation` ?), **charge** (0,32 s), récupération, mort. |
| Ikiryo | Marche, marche arrière, **visée** (1,1 s), tir, mort. |
| Brainless | Marche, **verrouillage** en reculant (0,65 s), **charge** tête baissée, **étourdi** contre un mur (1,6 s), mort. |
| Ubume | Marche, **ponte** (0,75 s), mort. Plus l'œuf (une sphère). |
| Méduse | Flottement, tir, mort. |

Les points d'interrogation sont des suppositions sur le contenu d'un fichier, à vérifier.

Un `Drone_MASTER.blend` existe, mais le proto n'a pas de drone : ne pas l'utiliser.

## 3. Les boss et les PNJ : rien en 3D

| Qui | Placeholder pixel fourni | Taille dans le proto |
|---|---|---|
| Le Tentaculaire | `boss.png` (3 frames = 3 phases, corps seul) | 34 × 31 px, affiché ×1,75. Les 4 tentacules au repos, l'œil qui suit Jira et les tentacules d'attaque sont à faire en formes simples. |
| Mechakuve | `mechakuve.png` (4 frames = 4 phases) | 48 × 92 px |
| Le Chercheur | `chercheur.png` (5 frames : humain, bras phase 1 / 2 / 3, bras levé) | 112 × 112 px, pieds en x = 40, y = 108 |
| Générateur de Mechakuve | `generateur.png`. Un `Asset_Generator.blend` existe dans le dossier 3D : à essayer. | 16 × 30 px |
| Le Cobaye | `cobaye.png`, `cobaye_course.png`. En 3D : Jira reteintée en rouge. | 18 × 28 px |
| Ulysse | `ulysse.png` | 12 × 25 px |
| Le marchand | `marchand.png` | 12 × 25 px |
| La gamine | `gamine.png` (2 états), `gamine_mutee.png` | 24 × 28 px |
| Le cartographe | `cartographe.png` (debout, à terre) | 16 × 22 px |
| La vieille dame | `vieille_dame.png` (2 états) | 24 × 32 px |
| La chose (Irruption) | Le Kama, tout noir, avec deux yeux rouges | |

Pistes de concept sur le disque du studio, pour les artistes : `BRISCARD/MOBS/boss_phase0_cuveferme.webp`, `boss_phase4.jpg`, et les planches `BRISCARD_PROD/DOC/BIBLE/JPG/mis-en-page_3_boss.jpg`. Le Cobaye a ses planches dans `BRISCARD/COBAYE/`.

`3D/humain.fbx` existe à la racine : s'il s'agit d'un humain de base, il peut servir de placeholder 3D pour les PNJ à la place des sprites.

## 4. Le décor

**Ce qui existe en FBX**

| Dossier | Contenu | Usage dans le proto |
|---|---|---|
| `Assets_Cuves/` | Cuve_1 à 5 et variantes (8 fichiers) | Cuves de réveil du Kibo, cuves à spécimens de l'étage 2, cuve du gardien, cuve vide de l'Irruption |
| `Assets_Hitech/Assets_Door/` | DoorB_1 à 3, All_DoorB | Les portes. Il faut 4 états : ouverte, verrouillée, rouge, rouge éteinte. |
| `Assets_Barriers/` | 7 barrières | Garde-corps des passerelles et de la plateforme du C.O.M |
| `Assets_Hitech/Assets_Computer/` | 13 ordinateurs, plus 9 tables assemblées | Postes de recherche (étage 3), terminaux, décor solide |
| `Assets_Hitech/Assets_Control_panel/` | 4 pupitres | Console de confinement de la Faille, pupitre du C.O.M |
| `Assets_Hitech/Assets_Hitech_big/`, `_medium/`, `_small/` | 27 modules | Décor solide (les tuiles `X` : caisses, machines), analyseurs |
| `Assets_Hitech/Assets_Ventilation/` | ModuleFanBig | Décor mural |
| `Assets_Pipes/` | 8 gros tuyaux, 8 petits | Habillage des murs |

**Ce qui n'existe qu'en `.blend`** (à exporter) : `Asset_Wall`, `Asset_Ecran_Com` (écran mural), `Asset_Futuristic_operating_table` (table d'opération de l'étage 3), `Asset_Generator`, `Assets_Doors`, et un `Asset_Pont` dans le dossier UV (passerelle).

**Textures du décor** : les props n'ont que des cartes de **curvature**. Il n'y a pas de couleur. Pose un matériau uni aux couleurs de la palette de l'étage en attendant.

**Ce qui manque** (placeholder pixel fourni quand il existe)

| Élément | Placeholder | Note |
|---|---|---|
| Sol | `sol_<ambiance>.png` (10 tuiles) | Un plan de 100 × 100 cm par tuile, ou un grand plan texturé |
| Murs | `mur_<ambiance>.png` (6 tuiles) | Un bloc de 100 × 100 cm, haut de 200 à 300 cm |
| **La sphère du C.O.M** | `com.png` (2 états) | 80 × 84 px. Pièce maîtresse, à modéliser. En attendant : une sphère. |
| Plateforme et passerelles du C.O.M | (dans le code) | Du sol posé sur le vide, avec les barrières existantes |
| Le fond du vide | (dans le code) | Les entrailles de la station qui défilent. En attendant : noir, avec quelques lumières lointaines. |
| L'infestation | `infestation.png` (3 variantes) | Masse de chair. En attendant : des sphères rouges qui pulsent. |
| Socle d'objet | `piedestal.png` | |
| Icônes d'objet | `item_icones.png` (disquette, cassette, greffe, écho, perk) | Un sprite qui flotte au-dessus du socle |
| Ramassables | `ramassables.png` (scrap, énergie, cœur), `donnee.png`, `ramassables_quete.png` (insigne, log, peluche), `carte_rouge.png` | Des sprites qui flottent |
| ADN | (dans le code) | Une petite gemme verte |
| Console de la Faille | `console_faille.png` (3 états) | Ou un pupitre existant |
| Alcôve du marchand | (dans le code) | Un pan de mur qui pivote |
| Tas du Trésor | `tresor.png` (stock, carnage) | |
| Silo cryo | `silo_test.png` (3 états) | Une cuve existante peut faire l'affaire |
| Étagère de bocaux | `etagere_bocaux.png` | |
| Analyseur | `analyseur.png` | Ou un module Hitech |
| Écran mural | `ecran_mural.png` (5 contenus) | Ou `Asset_Ecran_Com` |
| Arbres | `arbre_serre.png` (vert, fruitier, muté) | |
| Bac de culture, hydroponie | `bac_plantes.png`, `hydroponie.png` | |
| **Le saule pleureur** | `saule.png` | 112 × 112 px. Devient transparent quand Jira passe dessous. |
| Le banc | (dans le code) | |
| Œuf d'Ubume | (dans le code) | Une sphère jaune-vert |
| Cible d'entraînement | (dans le code) | Un cube sur pied |

## 5. Les effets

Le dossier `BRISCARD/VFX/` contient des essais Blender : `puff_dash`, `shot_rafalle`, `shot_shotgun`, `idée_chot_chargé`. À voir avec Sabri s'ils sont exploitables dans Unreal.

Tous les effets du proto sont des particules carrées dessinées par le code. Pour le portage, des effets simples suffisent, mais **aucun ne doit manquer** : ils portent la lecture du combat.

| Effet | Rôle |
|---|---|
| Flash de bouche, douille éjectée | Poids du tir |
| Traînée du projectile, étincelles à l'impact sur un mur | Lecture du tir |
| Flash blanc de l'ennemi touché, gerbe à sa mort | Retour de coup |
| Marqueur d'apparition d'un ennemi (0,7 s) | Laisse le temps de réagir |
| **Télégraphes** : cercle au sol (mortier, pieux, écrasement), ligne (tentacule, visée de l'Ikiryo), cône (balayage) | Indispensables. Toute attaque dangereuse s'annonce. |
| Anneau de l'onde, rayon du Laser, arc de la Lame | Sorts et greffes |
| Silhouettes fantômes du dash | |
| Flaque d'acide, matière organique au sol | Zones dangereuses |
| Rayon de l'ultime | |

## 6. Comment faire un placeholder

**Sprite pixel en billboard** (créatures, PNJ, objets)
1. Importe le PNG depuis `PLACEHOLDERS_PIXEL/`. Réglages de texture : filtre **Nearest**, pas de mipmaps, pas de compression qui bave (preset UI ou équivalent).
2. Un PNG contient plusieurs frames côte à côte. La taille d'une frame et leur nombre sont dans `PLACEHOLDERS_PIXEL/_index.json` (`fw`, `fh`, `n`).
3. Affiche-le sur un plan avec un matériau non éclairé, en masque alpha, toujours tourné vers la caméra.
4. **Échelle : 1 px = 6,25 cm.** Un sprite de 14 × 25 px fait 87 × 156 cm. Le bas du sprite est aux pieds.
5. Les sprites regardent vers la droite : retourne-les quand l'ennemi regarde à gauche.
6. Pour un sprite à 2 frames, alterne-les 4 à 6 fois par seconde pendant le déplacement.

Un composant C++ unique, « placeholder sprite », qui prend un nom de fichier et lit `_index.json`, évite de refaire le réglage à chaque fois.

**Forme simple** (décor) : un cube, un cylindre ou une sphère, à la bonne emprise en tuiles, avec la couleur de la palette de l'étage et un nom lisible.

**La règle** : la collision et le gameplay sont portés par l'acteur, jamais par le visuel. Remplacer un placeholder par le vrai modèle ne doit rien changer au jeu.
