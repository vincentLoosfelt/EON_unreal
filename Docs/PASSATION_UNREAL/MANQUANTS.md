# Manquants — liste vivante

**Ce fichier est tenu à jour par le Claude de Vincent.** Chaque placeholder posé dans Unreal y gagne une ligne. Chaque asset livré passe à ✅. C'est la liste que Sabri et les artistes suivent pour savoir quoi produire.

État de départ au 13/11/2026, d'après les noms de fichiers du dossier `3D` (voir `07_ASSETS_3D.md`).

Légende : ❌ rien · 🟡 existe mais pas prêt pour Unreal · 🔲 placeholder en place · ✅ livré et intégré

## 1. Exports à faire (le travail existe, il faut le sortir en FBX)

| Asset | Où il est | État |
|---|---|---|
| Nuke : modèle, rig, 3 anims (walk, attack, death) | `RIG/Ghoul.rig_MASTER.blend`, `Fichier_blend/Animation/Ghoul/` | 🟡 |
| Onibi : modèle, rig, 5 anims | `RIG/Kamikaze.rig_MASTER.blend`, `Fichier_blend/Animation/Kamikaz/` | 🟡 |
| Kama : modèle, rig, 5 anims | `Fichier_blend/Slasher.mode_MASTER.blend`, `Fichier_blend/Animation/Slasher/` | 🟡 |
| Ubume : modèle | `Fichier_blend/Mage.mode_MASTER.blend` | 🟡 |
| Table d'opération | `Fichier_blend/Asset_Futuristic_operating_table.blend` | 🟡 |
| Générateur | `Fichier_blend/Asset_Generator.blend` | 🟡 |
| Mur | `Fichier_blend/Asset_Wall.blend` | 🟡 |
| Écran mural | `Fichier_blend/Asset_Ecran_Com.blend` | 🟡 |
| Passerelle | `Fichier_blend/UV/Asset_Pont_UV.blend` | 🟡 |

> **15/11/2026 : plus aucun sprite pixel.** Les ennemis sont en 3D (table `Looks` de `EONEnemy.cpp`). En attendant les exports :
> Nuke = Ghoul d'Unity (vrai modèle, anim de marche) ; Onibi = Minion (Unity) ; Kama = Mutant (Unity, anim) ; Ikiryo = `Shooter_mode.fbx` du studio importé dans `/Game/EON/Ennemis/Ikiryo` (corps, bras, mâchoire, sans anim).
> Le lacet de chaque modèle (`YawOff`) reste à vérifier à l'image.

## 2. Personnages et créatures à produire

| Asset | Détail | Placeholder | État |
|---|---|---|---|
| Ikiryo : animations | Marche, marche arrière, visée, tir, mort | Modèle FBX sans anim | ❌ |
| Ubume : rig et animations | Marche, ponte, mort | 🔲 Monster05 (Unity, xeno), sans anim | 🔲 |
| **Méduse** | Design 2D, modèle, rig, anims (flottement, tir, mort) | 🔲 Chimera Meshy (Unity, xeno), flotte, sans anim | 🔲 |
| **Brainless** | Design 2D, modèle, rig, anims (marche, verrouillage, charge, étourdi, mort) | 🔲 Brain_Crawler (Unity) avec son anim | 🔲 |
| **Le Tentaculaire** | Corps en 3 phases, 4 tentacules au repos, tentacules d'attaque, œil, sortie de cuve | 🔲 Formes simples 3D (AEONBoss) : boule de chair, œil qui suit Jira, tentacules en chapelets de sphères, vitre de cuve en cylindre | 🔲 |
| **Mechakuve** | 4 phases : cuve fermée, bouclier, cuve ouverte, spécimen sorti. Pinces, queue, câbles. | `mechakuve.png` | ❌ |
| **Le Chercheur** | Forme humaine, bras mutant en 3 tailles, anims de balayage, d'écrasement, d'allonge | `chercheur.png` | ❌ |
| Le Cobaye | Un skin de Jira (cheveux rouges, bandage, cape, bras à griffes) | Jira reteintée | ❌ |
| Ulysse | PNJ, debout | `ulysse.png` | ❌ |
| Le marchand | PNJ, dans son alcôve | `marchand.png` | ❌ |
| La gamine | Recroquevillée, puis calmée avec sa peluche | `gamine.png` | ❌ |
| La gamine mutée | Variante d'élite | `gamine_mutee.png` | ❌ |
| Le cartographe | Gamin à casquette et lunettes : debout, à terre | `cartographe.png` | ❌ |
| La vieille dame | Assise sur un banc, port cortical à la tempe | `vieille_dame.png` | ❌ |
| La chose (Irruption) | Silhouette noire, yeux rouges | Kama noirci | ❌ |
| Variantes d'élite | Un signe distinctif par mob | Mob à 130 % teinté | ❌ |
| Œuf d'Ubume | | Sphère | ❌ |

## 3. Jira : ce qui manque

| Asset | Placeholder | État |
|---|---|---|
| Jira en jeu | 🔲 Sprite `jira_a0` en billboard (18 × 29 px → 113 × 181 cm), en attendant le branchement de `Cyborg_mode.fbx` dans BP_Jira (J1) | 🔲 |
| Armure niveau 1, 2, 3 (bras transformé, armure, casque) | Teinte du matériau | ❌ |
| Veines de mutation | Teinte rouge qui pulse | ❌ |
| Bras Rafale, Laser, Lame, Shotgun, Griffe, Rayon, Fauche | Couleur du bras | ❌ |
| Anim de chute dans le vide | Réduction d'échelle | ❌ |
| Anim de mêlée (Lame, Fauche) | `attack_light` accélérée | ❌ |
| Anim de la Griffe | | ❌ |
| Anim de l'onde, pose de l'ultime | `attack_charged` | ❌ |
| Anim de sortie de cuve | `idle` | ❌ |
| Illustration du cut-in de l'ultime | Bande de couleur | ❌ |

## 4. Décor à produire

| Asset | Placeholder | État |
|---|---|---|
| **La sphère du C.O.M** (2 états : en attente, prête) | `com.png` ou sphère | ❌ |
| Le fond du vide (entrailles de la station) | Noir | ❌ |
| L'infestation (3 variantes) | `infestation.png` | ❌ |
| Sols des 6 ambiances | `sol_*.png` | ❌ |
| Murs des 6 ambiances | `mur_*.png` | ❌ |
| Matériaux couleur de tous les props (seules les curvatures existent) | Couleur unie | ❌ |
| Porte : 4 états | `porte_h.png` | ❌ |
| Socle d'objet | `piedestal.png` | ❌ |
| Alcôve du marchand (mur pivotant) | Cube | ❌ |
| Console de la Faille | `console_faille.png` | ❌ |
| Tas du Trésor (stock, carnage) | `tresor.png` | ❌ |
| Silo cryo (occupé, vide, brisé) | `silo_test.png` | ❌ |
| Étagère de bocaux | `etagere_bocaux.png` | ❌ |
| Analyseur | `analyseur.png` | ❌ |
| Arbres (vert, fruitier, muté) | `arbre_serre.png` | ❌ |
| Bac de culture, rayonnage hydroponique | `bac_plantes.png`, `hydroponie.png` | ❌ |
| **Le saule pleureur** | `saule.png` | ❌ |
| Le banc | Cube | ❌ |
| Herbe et fleurs des serres | Rien | ❌ |
| Cible d'entraînement | 🔲 Cube de 75 cm (12 px) sur un poteau, `EONTrainingDummy`, rayon de touche 7 px, centrée à la hauteur du bras (J1) | 🔲 |
| Sols des 6 ambiances (étage 1 Coursive) | 🔲 `M_TileFloor` : tuiles 1 m tirées au hasard parmi les 3 couleurs de sol de la palette, joints en couleur `g` (J0) | 🔲 |
| Murs (étage 1 Coursive) | 🔲 Cubes 1 × 1 × 2,5 m, `MI_Mur_Coursive` = couleur `w[1]` (J0) | 🔲 |
| Cuve brisée, cuve à spécimen (4 spécimens ratés) | `cuve_specimen.png` | ❌ |

## 5. Ramassables et icônes

| Asset | Placeholder | État |
|---|---|---|
| Gemme d'ADN | Sphère verte | ❌ |
| Butin des ennemis (J3) : ADN, scrap, Données crédités d'office au directeur de combat ; énergie (25 %) et cœur (4 %) pas lâchés | aucun | ❌ jalon 6 |
| Scrap, énergie, cœur | `ramassables.png` | ❌ |
| Puce de Données | `donnee.png` | ❌ |
| Insigne, log, peluche Lapin | `ramassables_quete.png` | ❌ |
| Carte rouge | `carte_rouge.png` | ❌ |
| Icônes de catégorie (disquette, cassette, greffe, écho, perk) | `item_icones.png` | ❌ |
| Une icône par objet (36) | Icône de catégorie | ❌ |
| Icônes de boutons manette | `boutons_manette.png` | ❌ |

## 6. Interface

| Asset | État |
|---|---|
| Portraits de dialogue : 9 personnages × 5 expressions (neutre, sourire, inquiet, colère, triste) | ❌ |
| Habillage du HUD selon la charte UI | ❌ |
| Cadre du terminal du C.O.M | ❌ (`com_terminal.png` en placeholder) |
| Le levier de descente | ❌ |
| Écran titre | ❌ |

## 7. Effets

Voir la liste dans `07_ASSETS_3D.md`, section 5. Essais existants dans `BRISCARD/VFX/` (dash, tir rafale, tir shotgun, tir chargé), à évaluer.

| Effet | État |
|---|---|
| Projectile Standard (J1) | 🔲 Sphère étirée 50 × 10 cm, matériau `M_Flat` non émissif, couleur de la greffe |
| Flash de bouche (J1) | 🔲 Point light 0,06 s, couleur de la greffe. Le sprite du flash du proto manque. |
| Chiffres de dégâts et « DPS » de la cible (J1) | 🔲 `DrawDebugString` : absent des builds Shipping. À remplacer par un widget. |
| Douille éjectée et rebondissante | ❌ pas fait |
| Réticule qui s'écarte au tir | ❌ pas fait |
| Impact de balle (étincelles) et mort de balle au mur | ❌ pas fait |
| Traînée de fantômes du dash, poussière de la course | ❌ pas fait |
| Dash : vide franchi | attend le jalon 2 |
| Ennemis (J3) : sprites | 🔲 Sprite pixel du proto en billboard (`UEONSpriteComponent`), clignotement blanc par la teinte, élites agrandies × 1,45 sans teinte |
| Ennemis (J3) : tirs et œufs | 🔲 Sphères `M_Flat` (magenta #ff3d8b, orange #e8923a, œuf vert #d8f06a) sans halo ni lumière |
| Ennemis (J3) : télégraphes, barres de vie, nom d'élite, marqueurs d'apparition, anneau d'explosion, chiffres | 🔲 `DrawDebug*` : absents des builds Shipping. À remplacer par des decals / widgets |
| Ennemis (J3) : trous dans le sprite (punchHole), éclats de mort, éclosion d'œuf, poussière d'apparition | ❌ pas fait |
| Ennemis (J3) : écran « Signal perdu », bilan | 🔲 Titre par `AEONHUD`, bilan en messages `GEngine` (pas en Shipping) |

## 8. Sons

Le proto synthétise tout. Rien n'existe en fichier. Les 31 sons du proto, par leur clé :

- **Tir** : `shoot`, `burst`, `shotgun`, `laser`, `slash`, `charge`
- **Impact** : `hit`, `kill`, `explode`, `hurt`
- **Jira** : `dash`, `fall`, `mut` (passage de stade), `grow` (l'armure pousse), `spell` (onde), `holo`
- **Ennemis** : `eshot` (tir), `tele` (annonce d'attaque ou d'apparition), `efall` (chute), `roar`, `whip` (tentacule), `mortar`
- **Monde** : `door`, `lift` (C.O.M), `glass` (cuve qui éclate), `hiss` (vidange), `alarm` (Faille), `deny` (porte refusée)
- **Butin** : `pickup`, `item`, `card`

Plus une petite voix synthétique par personnage pour le défilement des dialogues, et aucune musique.

## 9. Écarts de réglage

Quand une valeur du proto a dû être corrigée pour la 3D. Une ligne par écart.

| Réglage | Valeur du proto | Multiplicateur appliqué | Pourquoi | Jalon |
|---|---|---|---|---|
| Vitesse de Jira (marche, course ; pas le dash) | marche 82 px/s (5,1 m/s), course ×1,7 (8,7 m/s) | SpeedMul3D = 0,55 (2,8 et 4,8 m/s) | Vincent (14/11) : en 3D Jira « speed vachement ». Le dash garde 345 px/s : sa portée (52 px) doit franchir les fossés de 2 tuiles. La course est aussi raccourcie (anims Jira_walk / Jira_run recalées sur ces vitesses) | J2 |
| Vitesse de croisière des ennemis (J3) | ED.spd : Nuke 42, petite Nuke 48, Ikiryo 34, Onibi 68, méduse 36, Slasher 38, Brainless 30, Ubume 34 px/s ; écart de 30 px/s, recul de prise d'élan 18 px/s | × SpeedMul3D (0,55), comme Jira | Sinon ils rattrapent une Jira ralentie. Ne changent pas : dash du Slasher et charge du Brainless (235 px/s), recul des coups (95 px/s), poussée du Brainless (170 px/s), et tous les tirs ennemis (82 et 170 px/s) : leurs portées sont liées aux tuiles et aux télégraphes | J3 |

## 10. Remarques de design

Ce que le Claude de Vincent a remarqué en portant, à soumettre à Sabri. Ne rien changer sans son accord.

- (J1, technique) À 0 d'énergie le proto passe en mutation et le tir ne coûte plus rien. La mutation n'existe pas avant le jalon 4 : en attendant, l'énergie reste à 0 et Jira continue de tirer gratuitement, sans double dégât ni ralentissement.
- (J1, technique) Hitstop = dilatation du temps global à 0,0001 pendant X ms de temps réel. Il fige donc tout, effets compris, alors que le proto laisse tourner ses effets à 15 % pendant l'arrêt. Aucune arme du jalon 1 ne le déclenche encore (il faut un coup à 3 dégâts ou plus).
- (J0, technique, pas de design) Les touches clavier sont liées par caractère et non par position physique : ZQSD et WASD sont liés tous les deux. Effet de bord : sur AZERTY, A va aussi à gauche, et sur QWERTY, Q aussi. Aucune action n'utilise A ni Q, donc sans conséquence en jeu.

## Jalons 4 et 5 (14/11) : écarts provisoires
- Effets du jalon 4 (rayon Laser, arc de Lame, anneau de l'onde, frappes de l'ultime) : traits DrawDebug provisoires, à remplacer par des Niagara.
- Le corps de Jira ne change pas avec l'armure de mutation (le proto change le sprite).
- LOCK_G du proto : toutes les greffes sont en main dès le départ (pas de déblocage).
- Onibi : la perte de 6 PV au leurre non porté n'est pas faite.
- Pas de système de dialogue : les répliques (carte rouge, porte rouge, chair) passent en bannière ; le délai de l'embuscade (1,4 s) remplace la fin du dialogue.
- Terminal du C.O.M : pas d'interface à onglets ; « Interagir » près de la sphère tire directement le levier (onglet Descente).
- Gardien (salle ASCENSEUR) : vide jusqu'au jalon 7 ; y entrer neutralise l'étage.
- Chair de l'infestation et carte rouge : sphères et pavé provisoires (pas encore de modèle).
- Kibo (réveil, passerelle, C.O.M à quai), grande carte et test du générateur sur 1 000 étages : pas encore faits.
- Zone de touche des ennemis : capsule couchée nord-sud à hauteur du bras, longue de ehh + 2·ehr, pour reproduire hitTest() du proto en vue de dessus.

## Jalon 6 : ADN, niveaux, objets (14/11)

Fait : ADN aspiré et barre de niveau, écran de niveau à 3 cartes (jeu en pause), 36 objets de items.json avec leurs effets, socles (Opportunité gratuits, Marchand payants, Trésor stock ou carnage), scrap, énergie, cœurs, bouclier, Renaissance.

Écarts restants :
- Les objets de catégorie GREFFE ne sortent jamais (LOCK_G du proto) : à ouvrir avec le système de greffes à débloquer.
- Les perks propres à la Griffe n'ont pas encore d'effet visible.
- Les « Données » (disquette Données) ne sont comptées nulle part : pas encore d'écran de méta-progression.
- Pas d'onglet Améliorations au C.O.M.
- Pas de piège au Trésor.
- Les ramassables au sol ne sont pas mémorisés par salle : l'ADN est crédité en sortant, le scrap et l'énergie restants sont perdus.
- Socles et ramassables : formes provisoires (cylindre, cube, sphères).
- Taux de touche du pilote : 12 % environ, comme au jalon 5 (ennemis rapides qui tournent autour de Jira).
- Commandes de test ajoutées : eon.adn N, eon.scrap N, eon.salle ROLE, eon.debut "cmd1;cmd2" (commandes lancées 2 s après le début).

## Salles en Blueprint et murs du HUB (14/11)

- Les salles ont les murs du HUB : WallA320 avec son matériau d'origine MI_Unity_Mur (gris texturé). Peinture rose MI_BG_walll_painted et bandeaux MI_BG_Room_light retirés à la demande de Vincent (15/11). Hauteur des murs : 220 cm au lieu de 300 pour garder la vue en plongée (réglable par salle : WallHeightCm).
- Les salles dessinées sont des Blueprints enfants d'EONTileRoom, rangés dans /Game/EON/Salles (17 au départ, tirés du générateur du proto ; le Marchand est redessiné à la main). Le générateur d'étage prend un Blueprint qui correspond au rôle (RoomRole ou ExtraRoles) et à la forme (Shape). Sinon, il génère la salle. Weight = 0 retire une salle du tirage.
- Le plan se dessine dans le champ Layout : # mur, . sol, _ vide, = fossé, X caisse, K cuve. Les portes sont posées automatiquement au milieu des côtés. Aperçu immédiat dans le viewport du Blueprint. Bouton « Plan Depuis Generateur » pour repartir d'une salle du proto. Le décor posé à la main dans le Blueprint (meshes) apparaît en jeu, mais il n'a pas de collision de gameplay : ce sont les tuiles qui comptent.
- Pas encore de cadre de porte du HUB : les meshes porte et walldooreno sont importés retournés.

## Menus de test F6 (Tweak) et F7 (Triche) — 15/11

- **F6 Tweak** : le banc de réglages (§12 de 02) en jeu, par rubriques : Jira (marche, course, distance du dash), tir, impacts, caméra, ennemis (PV, vitesse, dégâts en coeurs, taille des vagues, PV des boss), mutation, rendu et son. Une valeur différente du préréglage studio passe en ambre avec la valeur studio à côté. Préréglages studio / sec / max. « Exporter » écrit `Saved/EON_Reglages.txt` (et le log, lignes EON_REGLAGE) pour reporter les bonnes valeurs dans DT_GameFeel.
- **F7 Triche** (panneau DEV du proto) : invincible, énergie infinie, dash infini, tue en un coup, soigner, +100 ADN, +100 scrap, pilote automatique, tuer les ennemis, lancer une vague, aller à une salle de l'étage, passer une porte, étage suivant. Les triches restent actives quand on change de préréglage.
- Touches : flèches pour naviguer et changer (Maj x10, Ctrl /5, Maj+haut/bas = rubrique), Entrée pour une action, Échap pour fermer ; Jira reste jouable en ZQSD et souris. F1 à F5 sont pris par Unreal (modes de vue en PIE), d'où F6 / F7.
- Nouveaux réglages ajoutés au banc (pas dans le proto) : DashMul, EnemyHp, EnemySpeed, EnemyDmg, WaveMul, InfEnergy, InfDash, OneShot.
- Rubrique « Rendu » du menu F6 : « Filtre peinture » (poids du blendable M_PP_EON_Peinture) et « Post-process complet » (active/coupe PP_Etage ou PP_HUB entier : exposition manuelle, bloom). Réglages PPPaint / PPAll.
- Retirés du menu F6 car branchés sur rien dans le portage : Crt, Paint, Dark, Holes, Light, Ambient, Sound, Vol, Shells, Regen, BossHp. Ils restent dans DT_GameFeel (données du proto) ; à rebrancher avec le son, les douilles, les boss, ou à supprimer.

## Jalon 8 : la méta (16/11), première partie

Fait :
- **Sauvegarde** : un emplacement `EON_Meta` (`Saved/SaveGames/EON_Meta.sav`). Il garde tous les champs de META (05, section 10). Il est sauvé dès qu'il change, et vérifié toutes les 0,5 s.
- **Écran titre** : il s'affiche au lancement, seulement s'il existe une progression. Deux choix : Continuer, ou Nouvelle partie (le deuxième appui efface tout).
- **Stats cumulées et 12 déblocages** :
  - stats suivies : runs et morts (à la mort), kills, étage le plus lointain, stade de mutation le plus haut, gardiens par étage ;
  - encart « DÉBLOQUÉ » de 3,2 s, et récapitulatif sur l'écran de mort ;
  - un élément verrouillé est absent du tirage et de la liste des greffes : Jira démarre avec Standard et Rafale.
- **Données** :
  - ramassable « donnée » qui ne tombe jamais, multiplié par la Disquette Données ;
  - un élite en lâche 2, le gardien 4 paquets de 3 ;
  - premier ramassage : dialogue `donnees`.
- **Cassette Shield** :
  - NV1 trouvée en run, puis gardée pour toutes les runs (elle ne sort plus au tirage) ;
  - NV2 : recharge après 10 s sans dégâts ;
  - NV3 A (onde nettoyante) ou NV3 B (miroir), un choix définitif ;
  - achat dans l'onglet Cassettes, seulement à quai.
- **Terminal du C.O.M** :
  - la touche Interagir ouvre le terminal, avec ses 5 onglets : Améliorations, Logs, Quêtes, Descente (le levier), Cassettes ;
  - le jeu est en pause tant qu'il est ouvert.
- **Dialogues** :
  - boîte, portrait, texte lettre par lettre ; Espace, E ou A pour la suite, Échap ou B pour tout passer ;
  - le texte vient de `dialogues.json`, via `Saved/Scripts/gen_dialogues.py`, qui écrit `EONDialogData.inl` ;
  - déjà déclenchés : `iaIntro`, `cuves`, `labos`, `serres` (et la phrase générique des autres étages), `card` (l'embuscade suit la fin du dialogue), `redDoor`, `infestCom`, `infestBoss`, `marchand`, `donnees`.
- **Tests** :
  - menu F7 : « Tout débloquer » et « +50 Données » ;
  - console : `eon.meta` (voir et modifier la sauvegarde, `eon.meta effacer`), `eon.toutdebloquer`, `eon.terminal ONGLET [LIGNE] [1]`, `eon.dialogue CLE`, `eon.ui continuer|nouvelle|suite|passer`, `eon.objet ID`, `eon.blesse N` ;
  - `eon.debut` et `eon.apres` comptent maintenant en temps réel : ils partent même pendant une pause (dialogue, terminal).

Provisoire ou manquant :
- **Kibo, première partie (16/11) : le C.O.M à quai.**
  - Toute nouvelle run commence au C.O.M à quai (`FloorN` = 0). C'est le hub 3D actuel, avec la bannière « Kibo - C.O.M a quai » et le HUD « KIBO / A quai ».
  - Le levier est prêt ; le tirer lance l'étage 1. Le dialogue `dock` ne joue qu'une fois (`bDockSeen`). On n'installe les cassettes que là.
  - `eon.salle` et `eon.forme`, lancés depuis le quai, descendent d'abord à l'étage 1.
  - Au quai, il ne reste qu'une passerelle, celle du sud. Au nord, à l'est et à l'ouest, les grilles, rambardes, murs percés et sas sont retirés, et les trous des portes sont murés avec le même WallA320 que le reste du hub. Une barrière invisible empêche de tomber au bout des moignons de passerelle (ils font partie du maillage COMv1).
  - Le terminal et l'arrivée de Jira sont côté sud. Au départ du C.O.M, tout revient tel que l'étage l'utilise (`SetDockLayout`).
  - Captures : `Docs/J8_quai.png`, `J8_quai_dialogue.png`, `J8_quai_levier.png`, `J8_quai_retour_etage.png`.
- **Kibo, deuxième partie (16/11) : salle de réveil et passerelle.**
  - Une nouvelle run commence dans la cuve de Jira, avec une cinématique de 2,6 s : Jira flotte dans le tube de sa cuve ; à 1,7 s, secousse et le tube descend autour du socle ; à 2,2 s, Jira est relâchée et saute hors de la cuve, devant le socle (la caméra reste au niveau du sol) ; à 2,6 s, le dialogue `wake` se lance. Pendant la cinématique, Jira ne répond pas.
  - La salle de réveil suit `buildTraining` : 36 × 20, 7 cuves, fossé à dasher, petit trou, porte nord. Les 4 cibles d'entraînement sont aux positions du proto. La consigne « Teste chaque greffe sur les cibles » s'affiche en bas, avec les greffes en main et leur touche (« 1 Standard  2 Rafale… », version clavier ; capture `Docs/J8_kibo_consigne_greffes.png`).
  - La passerelle suit `buildPasserelle` : 12 × 30, pont de 4 tuiles, portes nord et sud.
  - Enchaînement du proto : réveil (N) → passerelle → (N) C.O.M à quai, dont le sas sud est ouvert ; retour possible dans l'autre sens.
  - Tests : `eon.kibo reveil|passerelle|quai` ; `eon.go N|S` pose Jira sur le seuil d'une porte du Kibo.
  - Captures : `Docs/J8_kibo_cuves.png`, `J8_kibo_reveil.png`, `J8_kibo_passerelle.png` ; vidéos du réveil : `Docs/J8_kibo_cuves_reveil.mp4` (vue du jeu), `Docs/J8_kibo_cuve_jira_proche.mp4` (caméra rapprochée, CamViewTiles 9).
- **Descente du C.O.M (16/11)** : le C.O.M est un centre d'opération mobile qui descend la colonne du hub d'un étage à l'autre.
  - Au levier : mise en marche (3 à-coups), puis descente de 32 m en accélérant (ease-in cubique). Le maillage COMv1 (dôme compris), le champ de force, le terminal, Jira et les arcs électriques descendent ensemble.
  - Fondu au noir doux (smoothstep), puis chargement de l'étage dans le noir et fondu d'ouverture. Le C.O.M arrive de 22 m plus haut et freine (ease-out quintique) jusqu'à la hauteur des passerelles, puis un petit choc à l'arrêt. Le dialogue d'arrivée suit.
  - Pendant la séquence, Jira ne répond pas et la caméra reste à hauteur des passerelles. Durée : 3,7 s au départ, 3,3 s à l’arrivée. Vidéo : `Docs/J8_com_descente.mp4`.
  - Becs du C.O.M (les 4 « anchor ») : ils doivent pivoter de 90° vers le bas avant le départ. Animation à faire par Vincent dans Blender ; un premier essai en C++ a été retiré le 16/11.
  - Pas de son de moteur ni de grincement (§8).
- **Kibo, provisoire** :
  - les 7 cuves de la salle de réveil (16/11) sont des `AEONWakePod` : Cuve_1 (FBX studio, identique à Unity) séparée dans Blender en corps + tube de verre (`blender_cuve_reveil.py`, `eon_cuve_reveil.py`), posée au sol avec son socle (le tube commence à 130 cm, Jira y est levée pendant le réveil). Les cuves « plus avancées » du Drive que cherche Vincent ne sont pas encore identifiées (ni Assets_Cuves, ni Assets_Cuve.blend). Scellées : liquide vert du proto, silhouette sombre en formes simples (provisoire : pas de modèle de spécimen), voyant rouge qui clignote. Brisées : dents de verre sur les bords du tube, éclats et flaque verte au sol. Celle de Jira : vidange (0,6–1,6 s, son `hiss`), le tube descend autour du socle (1,7 s, son `door`), vapeur, bouffée à 2,6 s (son `lift`). La vapeur est faite de sphères translucides, en attendant Niagara (jalon 10) ;
  - toutes les cuves ont la vitre tournée vers la caméra, même contre le mur sud : vues de dos, elles cachaient leur contenu (et Jira pendant le réveil) ;
  - les sons `hiss`, `door` et `lift` sont la synthèse du proto rendue en WAV (`eon_sons_proto.py`, `SFX_Proto_*`) ; Cuve_1 n'a pas de texture de couleur (seulement une carte de curvature) : matériaux unis de Unity ;
  - la salle de réveil et la passerelle utilisent l'éclairage et la palette des salles générées, et non une ambiance propre au Kibo.
- **Attendent le Kibo et les salles Événement (jalon 9)** :
  - Ulysse : sauvetage, refuge, insignes, PV permanents ;
  - le Cobaye jouable : seul son déblocage est en place, avec la bannière « Une cuve s'est ouverte au Kibo » ;
  - la gamine, la vieille dame, les logs à ramasser, le cartographe, le chercheur, la rupture de paroi.
  - Leurs champs de sauvegarde, leurs entrées dans l'onglet Quêtes et l'onglet Logs sont déjà prêts.
- **Portraits** : un carré provisoire à la couleur du personnage, avec son initiale et l'expression en petit. Les vrais portraits restent à produire (§6).
- **Sons** : `item` (déblocage) et les voix de dialogue (une par personnage) n'ont pas de fichier (§8).
- **Interface** : le terminal et l'écran titre sont dessinés au Canvas, en attendant l'habillage UMG selon la charte (§6).

## Jalon 9 : les events (16/11)

- **La Faille** (`buildRoom`, `riftOpen`, `updateRift`) : fait.
  - Un pan de mur sans porte est choisi à la génération (6 tuiles au nord ou au sud, 4 à l'est ou à l'ouest). La console de confinement est sur la tuile de sol la plus éloignée de la brèche.
  - À 1,2 s, le bord du mur devient du vide (bannière « Rupture de paroi », dialogue `rift`). À 6 s, le rang intérieur cède aussi (« La breche s'elargit »).
  - Aspiration vers la brèche : 70 px/s par stade, ×0,45 pour Jira, ×0,3 pour les ennemis, ×0,25 pour les ramassables. Oxygène : 32 s, puis 1 PV toutes les 2 s. L'alarme sonne toutes les 3 s, puis 2,1 s, puis 1,2 s.
  - Une seule vague (×0,9). La salle ne se libère pas à la dernière mort : la console s'allume (« Confinement disponible »), et la toucher (12 px) referme la paroi (« Paroi confinee »), sans bannière « Salle nettoyée » ni énergie, comme dans le proto.
  - HUD : jauge OXYGENE (ASPHYXIE, CONFINEMENT PRET), étiquettes « HORS LIGNE » / « CONFINEMENT » au-dessus de la console et « BRECHE », repère de bord d'écran avec la distance.
  - Sons `alarm`, `explode`, `card` et `roar` : synthèse du proto rendue en WAV (`eon_sons_proto.py`).
  - Test : `eon.salle FAILLE`, puis `eon.place console` pose Jira sur la console (`eon.place X Y` : n'importe où dans la salle, en px).
  - Captures : `Docs/J9_faille_combat.png`, `J9_faille_breche.png`, `J9_faille_confinee.png` ; vidéo `Docs/J9_faille.mp4`.
- **Faille, provisoire** :
  - console de confinement : un bloc de 0,9 × 0,9 × 1 m (éteint gris-bleu, puis cyan). Le proto a un sprite `console_faille` à 3 frames ; modèle 3D à produire ;
  - la brèche est un trou noir dans le mur. Le proto ajoute une bande de danger rouge clignotante le long du mur, un courant d'air (particules aspirées) et une gerbe de 40 éclats à chaque ouverture : à faire avec Niagara (jalon 10).
- **L'Irruption** (`eventsUpdate`, `stalkUpdate`) : fait.
  - La salle est une salle Calme avec 2 à 4 cuves (`placeVats` porté : 2 × 3 tuiles le long des murs nord et sud, sans couper de passage) ; la première est brisée et vide, les autres scellées. Ce sont les cuves de la salle de réveil (`AEONWakePod`).
  - La salle se verrouille. Les lampes vacillent, puis à 1,8 s « Les lumieres lachent » : il ne reste que la lampe de Jira (58 px). La chose (Kama élite, 1 PV, « La chose ») apparaît sur la tuile de sol la plus loin de Jira, à moins de 260 px.
  - Elle rôde autour de Jira (110 × 80 px) 2,5 à 4 s, s'arrête 0,65 s (rugissement, yeux rouges plus forts), charge à 240 px/s pendant 0,75 s, puis recommence. Un contact hors rôde inflige PV + 5 : seuls le bouclier et la Renaissance sauvent.
  - Morte : « La lumiere revient », un objet rare ou épique sur un socle au centre, 3 Données.
  - Test : `eon.salle IRRUPTION` (si l'étage n'en a pas, une salle Combat ou Tension pas encore visitée en devient une).
  - Captures : `Docs/J9_irruption_cuves.png`, `J9_irruption_noir.png` ; vidéo `Docs/J9_irruption.mp4`.
  - Noms des salles alignés sur le proto : « Irruption » (l'ancien portage affichait « Calme »), « Le corridor », « La faille ».
- **Irruption, provisoire** :
  - le noir passe par la compensation d'exposition de la caméra (−5 EV), la lampe de Jira et les yeux de la chose par deux lumières ponctuelles ; à régler à l'œil par Vincent (intensités dans le constructeur de `AEONFloorManager`) ;
  - le vacillement avant la coupure est global (−1,5 EV par à-coups), le proto éteint des lampes une par une ;
  - la chose utilise le modèle du Kama élite ; le proto n'a pas de sprite à part non plus ;
  - les traînées de particules de la charge : Niagara (jalon 10).
