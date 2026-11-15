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
