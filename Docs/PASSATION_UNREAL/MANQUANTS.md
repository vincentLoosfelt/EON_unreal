# Manquants — liste vivante

**Ce fichier est tenu à jour par le Claude de Vincent.** Chaque placeholder posé dans Unreal y gagne une ligne. Chaque asset livré passe à ✅. C'est la liste que Sabri et les artistes suivent pour savoir quoi produire.

État de départ au 13/11/2026, d'après les noms de fichiers du dossier `3D` (voir `07_ASSETS_3D.md`).

Légende : ❌ rien · 🟡 existe mais pas prêt pour Unreal · 🔲 placeholder en place · ✅ livré et intégré

## 1. Exports à faire (le travail existe, il faut le sortir en FBX)

| Asset | Où il est | État |
|---|---|---|
| Nuke : modèle, rig, 7 anims (walk, attack ×2, death ×4) | `RIG/Ghoul.rig_MASTER.blend`, `Fichier_blend/Animation/Ghoul/` → `/Game/EON/Ennemis/Nuke` (23/11) | ✅ |
| Onibi : modèle, rig, 8 anims (walk, run ×3, attack, explosion, death ×2) | `RIG/Kamikaze.rig_MASTER.blend`, `Fichier_blend/Animation/Kamikaz/` → `/Game/EON/Ennemis/Onibi` (23/11) | ✅ |
| Kama : modèle, rig, 4 anims (run, attack, taunt, death) | `Fichier_blend/Slasher.mode_MASTER.blend`, `Fichier_blend/Animation/Slasher/` → `/Game/EON/Ennemis/Kama` (23/11) | ✅ |
| Ubume : modèle, rig, 3 anims (idle, walk, attack) | Drive `Invocateur` → `/Game/EON/Ennemis/Ubume` (23/11) | ✅ |
| Ikiryo : modèle riggé (sans anim) | Drive `Shooter` → `/Game/EON/Ennemis/Ikiryo/SK_Ikiryo` (23/11, remplace les pièces statiques) | ✅ |
| Drone (pas encore de type d'ennemi en jeu) | Drive `Drone` → `/Game/EON/Ennemis/Drone/SM_Drone` (statique, Nanite) | ✅ importé, non branché |
| Table d'opération | `Fichier_blend/Asset_Futuristic_operating_table.blend` | 🟡 |
| Générateur | `Fichier_blend/Asset_Generator.blend` | 🟡 |
| Mur | `Fichier_blend/Asset_Wall.blend` | 🟡 |
| Écran mural (module du C.O.M) | `Fichier_blend/Asset_Ecran_Com.blend` → `/Game/EON/Meshes/COM/SM_Ecran_Com` (907 k triangles, Nanite) | ✅ importé, pas encore posé |
| Passerelle | `Fichier_blend/UV/Asset_Pont_UV.blend` | 🟡 |

> **23/11/2026 : modèles du studio branchés** dans la table `Looks` de `EONEnemy.cpp`.
> - Export Blender : `Saved/Scripts/blender_ennemis_export.py`. Sources renommées dans `SourceAssets/Ennemis/<Nom>`. Import : `Saved/Scripts/eon_ennemis_import.py`.
> - Déplacement en boucle, au rythme de la vitesse : Nuke `Walk`, Onibi `Run`, Kama `Run`, Ubume `Walk`. À l'arrêt : Ubume `Idle`, Onibi `Walk`.
> - Coup, joué une fois (`AEONEnemy::UpdateAnim`) :
>   - Nuke `Attack` (petite Nuke `Attack_V0`), au contact de Jira, vitesse ×1,5 ;
>   - Kama `Attack` à l'élan (Wind) ;
>   - Ubume `Attack` à la ponte, ×2 ;
>   - Onibi `Explosion` quand il s'arme : l'anim continue sur le cadavre s'il explose.
> - Mort : le directeur sort l'ennemi de la liste, puis `BeginCorpse` joue une mort au hasard. Le cadavre reste 0,6 s, s'enfonce dans le sol en 0,5 s, puis disparaît. Il n'y a pas de cadavre pour une chute dans le vide, ni pour un modèle sans anim de mort (Ubume, Ikiryo, Brainless, Méduse).
> - Vérifié en combat automatique (salle large, combatN 10). Le journal `-LogCmds="LogEON Verbose"` liste `EON_ENEMY coup` et `EON_ENEMY cadavre`.
> - Lacet -90 vérifié à l'image : tous regardent devant.
> - Non utilisés : Onibi `Attack`, `Run_Anticipation`, `Run_Variation` ; Kama `Taunt`. Aucun état du proto ne leur correspond.
> - L'Ikiryo n'a toujours aucune anim.
>
> **Matériau et UV (23/11)**
> - Les ennemis utilisent `M_EON_ToonEnnemi` : Substrate Toon opaque, sans normal map (demande de Vincent).
> - Kama : le rig du studio (`Slasher_mode_MASTER.blend`, 26/07) portait d'anciennes UV, 36 % des faces tombaient hors des îlots peints. Les UV du corps de `Slasher_mode_MASTER_UV.blend` y sont reportées (`Saved/Scripts/blender_kama_uv.py`, copie `Slasher_rig_UV_EON.blend`) : 0,6 % hors îlots. Les boucles (`curves`) et la mâchoire (`jaw`) n'ont pas d'UV sur cette texture et restent rayées. Sur le rendu du studio, les boucles sont vertes et la mâchoire blanche. ❌
> - Ikiryo : aucun fichier .blend ni FBX du Drive n'a le dépliage de `T_Shooter_Base_Color_V2` (23 % hors îlots). Ce dépliage n'existe que dans `T_Shooter_V2.spp`.
>   - Il a été extrait du projet Painter : `paint/scene.assbin` lu avec h5py, converti en OBJ triangulé `Reference/3D_Studio/Drive_20261123/Shooter/Shooter_spp_V2.obj`.
>   - Puis il a été reporté sur le rig avec `Saved/Scripts/blender_uv_depuis_obj.py`. Le recalage est exact (écart nul). La copie s'appelle `Shooter_rig_UV_EON.blend`.
>   - Hors îlots : corps 0,4 %, mâchoires 0 %, bras 1,5 %. ✅

## 2. Personnages et créatures à produire

| Asset | Détail | Placeholder | État |
|---|---|---|---|
| Ikiryo : animations | Marche, marche arrière, visée, tir, mort | Modèle FBX sans anim | ❌ |
| Ubume : animations manquantes | Ponte, mort | Idle, marche, attaque livrées | 🟡 |
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
| Impact de balle (étincelles) et mort de balle au mur | ✅ Niagara (J10) : étincelles à l'impact, 4 éclats au mur, point blanc au ricochet |
| Traînée de fantômes du dash, poussière de la course | 🔲 Poussière du dash et de la course en Niagara (J10) ; la traînée de fantômes n'est pas faite |
| Dash : vide franchi | attend le jalon 2 |
| Ennemis (J3) : sprites | 🔲 Sprite pixel du proto en billboard (`UEONSpriteComponent`), clignotement blanc par la teinte, élites agrandies × 1,45 sans teinte |
| Ennemis (J3) : tirs et œufs | 🔲 Sphères `M_Flat` (magenta #ff3d8b, orange #e8923a, œuf vert #d8f06a) sans halo ni lumière |
| Ennemis (J3) : télégraphes, barres de vie, nom d'élite, marqueurs d'apparition, chiffres | 🔲 `DrawDebug*` : absents des builds Shipping. À remplacer par des decals / widgets. L'anneau d'explosion est en Niagara (J10) |
| Ennemis (J3) : trous dans le sprite (punchHole), éclats de mort, éclosion d'œuf, poussière d'apparition | 🔲 Éclats de mort, éclosion d'œuf, éclaboussure du punchHole en Niagara (J10). Les trous dans le sprite et la poussière d'apparition ne sont pas faits |
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
- Effets du jalon 4 (rayon Laser, arc de Lame, anneau de l'onde, frappes de l'ultime) : l'anneau de l'onde et les éclats (bout du rayon, bout du fouet, impact de l'ultime) sont en Niagara depuis le jalon 10 ; le rayon, l'arc et le trait de l'ultime restent des traits DrawDebug provisoires.
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
- 16/11, vu par Vincent : les façades WallA320 des salles étaient retournées (face travaillée vers l'extérieur). Corrigé dans `AEONTileRoom::BuildHubWalls`.
- 16/11, écart au proto voulu par Vincent : les salles de combat générées n'ont plus de piliers 2×2 au milieu (variantes « pillars » et piliers de « mix » retirées de `EONTileGrid.cpp`). Les salles en Blueprint gardent leur plan tel quel.
- 16/11, écart au proto voulu par Vincent : à l'arrivée du C.O.M sur un nouvel étage, Jira garde la place qu'elle avait sur le C.O.M au départ du voyage (le proto la repose au centre + 40 px). `LoadHub`, `ComJiraOff` dans `EONFloorManager`.
- 16/11, écart au proto voulu par Vincent : Maj n'est plus une touche de dash. Maintenue en se déplaçant, elle fait courir Jira tout de suite (pas d'attente de 0,2 s). Le dash reste sur Espace et la manette. Maj est retirée de `IA_Dash` dans `IMC_EON` (`Saved/Scripts/eon_maj_course.py`) et lue en C++ (`SetSprintHeld`).
- 16/11, voulu par Vincent : Jira est toujours au sol. Plus de contrainte de plan, la gravité agit. Si elle passe en chute, en vol ou en nage au-dessus d'un sol, `KeepOnGround` la repose sur le sol sous ses pieds (jusqu'à 2 m plus bas), et l'animation reçoit toujours « au sol ». Au réveil, elle ne flotte plus dans la cuve et n'en saute plus : elle est debout au sol et en sort à pied (`WalkOut`, 1,4 s vers le nord). Écart au proto. Vérifié : 73 relevés `eon.diagsol`, toujours en marche sur un sol marchable (`Docs/J10_sortie_cuve_au_sol.jpg`).
- 16/11, voulu par Vincent : les trous sont de vrais trous (le sol invisible `PitFloorISM` est supprimé). Jira y tombe avec la gravité ; la kill zone est à 10 m sous le dernier sol où elle a marché (`KillZoneDepth`). En la franchissant, elle revient sur le sol le plus proche de l'endroit de sa chute (centre de tuile dont les 9 points SAFEPTS sont sur du sol ; hors salle en tuiles, dernier point sûr), perd 1 PV (`FallDmg`) et a 1,2 s d'invulnérabilité (`VoidRespawn`). Vérifié : PV 6 → 5 → 4 sur deux chutes, retour au bord le plus proche de la chute et non au point de départ. L'animation du proto (Jira qui s'enfonce et rétrécit en 0,5 s) est supprimée. Vérifié sur la passerelle du Kibo (`Docs/J10_chute_kill_zone.jpg`).

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
  - la brèche est un trou noir dans le mur. Le proto ajoute une bande de danger rouge clignotante le long du mur, un courant d'air (particules aspirées) et une gerbe de 40 éclats à chaque ouverture. La gerbe est faite (Niagara, jalon 10) ; la bande de danger et le courant d'air ne le sont pas.
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
  - la traînée de particules de la charge est faite (Niagara, jalon 10).
- **Trésor piégé** (`eventsUpdate`) : fait. 4 stocks sur 10 sont piégés : prendre la disquette verrouille la salle et réveille le « Dormeur des pieces » (Brainless élite, 18 PV × (1 + 0,3 × (étage − 1)), rayon 7, masse ×2), bannière « Quelque chose dormait sous les pieces ». Mort, la salle se déverrouille. Le stock pose aussi ses 2 Données (oubliées jusqu'ici). Test : `eon.tresor 1` (stocks piégés) ou `2` (carnages), puis `eon.salle TRESOR`. Capture `Docs/J9_tresor_dormeur.png`.
- **Trésor, provisoire** : le tas de pièces (stock, avec deux yeux rouges qui clignotent quand il est piégé) et les corps du carnage (`drawTresor`) ne sont pas dessinés ; il n'y a que le butin au sol. Modèles à produire.
- **Le Cartographe** (`eventsUpdate`, `cartoStart` / `cartoWin` / `cartoLost`) : fait.
  - Le gamin attend au centre de la salle (« ! » au-dessus de lui). Jira ne passe pas à moins de 8 px de lui. À 70 px, dialogue `cartoStart` ; à la fin, la salle se verrouille et 2 vagues arrivent (son `door`).
  - La moitié des ennemis apparus pendant la défense le visent, lui. Chaque contact lui retire 1 PV (6 PV, au plus un toutes les 0,8 s, flash blanc, son `hurt`) ; barre de PV sous lui.
  - À 0 PV : il tombe, dialogue `cartoLost`, la salle continue. Salle nettoyée et lui vivant : dialogue `cartoWin`, puis toutes les salles de l'étage sont révélées sur la mini-carte (colorées par rôle), bannière « Carte de l'etage completee », son `item`, et il repart (disparaît en 1,2 s).
  - Si Jira sort pendant son premier dialogue, il se remet à attendre.
  - Test : `eon.salle CARTO` (si l'étage n'en a pas, une salle Combat ou Tension pas encore visitée en devient une).
  - Le pilote automatique (`eon.auto`) va lui parler. Capture `Docs/J9_carto_defense.png`, vidéo `Docs/J9_carto.mp4`.
- **Cartographe, provisoire** : le gamin est un cylindre vert (veste) et une sphère (visage), 20 × 7 px ; le proto a un sprite `cartographe.png` (16 × 22, casquette ambre). Modèle à produire (`AEONNpc`, réutilisable pour la gamine, Ulysse et l'équipage).
- **Salle Événement** (`buildRoom`, `loadRoom`, `eventsUpdate`) : fait. Le contenu se décide à la première entrée :
  - **la gamine** (étage 2, tant qu'elle n'a pas été rencontrée) : recroquevillée au nord, elle tremble, « ? » au-dessus. À 34 px :
    - avec la peluche : dialogue `newtGood`, elle se calme et laisse un objet (rare au moins) sur un socle ;
    - sans la peluche : dialogue `newtBad`, la salle se verrouille, elle mute (Brainless élite « La gamine », 22 PV × (1 + 0,3 × (étage − 1)), rayon 7, masse ×2, bannière « La gamine mute »). Morte : « Elle ne souffre plus », un objet épique d'abord sur un socle.
    - Les deux fins sont gardées dans la sauvegarde (`bNewtDone`, `NewtEnd` « calme » / « perdue ») et le journal des quêtes les affiche déjà.
  - **Ulysse** (tant qu'il n'est pas sauvé) : salle verrouillée, un Slasher élite « Ex-quartier-maitre » (20 PV × (1 + 0,3 × (étage − 1)), rayon 6, masse ×2,2) ; Ulysse tremble, « ?! ». Mort : « Ulysse est sauf », « ! » au-dessus de lui. À 28 px : dialogue `ulysse1` (ou `ulysse2` s'il a déjà été croisé dans la run), insigne du quartier-maître en poche, un objet (rare au moins) sur un socle, bannière « Ulysse t'attend au Kibo ».
  - **l'équipage** (Ulysse sauvé) : un membre d'équipage muté encore jamais trouvé (Slasher ou Brainless élite, à son nom) ; mort, il lâche son insigne (« Insigne : … - a rendre a Ulysse »). Équipage complet : un « Survivant mute » sans insigne.
- **Ulysse au Kibo** : il attend à côté des cuves (8 × 16, 16 × 16 + 8 px). À 26 px (une fois par passage), il prend les insignes en poche : +10 scrap chacun, tous les 3 rendus +1 PV max permanent (2 au plus), répliques d'`ulysseHubLines` (« Il en reste N », « Tout l'équipage est là »…).
- **Logs et peluche** (`markFloorPickups`, `readLog`, `drawLog`) : fait. Un log par étage dans une salle Calme, Opportunité, Combat ou Tension (à gauche du centre) ; à l'étage 1, tant que la gamine n'a pas été rencontrée, la peluche dans une autre salle (pas Combat si possible). Pris, ils ne reviennent plus ; laissés, ils restent dans leur salle. Le log s'affiche en haut sans pause (« TRANSMISSION - LOG n/8 », 40 caractères par seconde, puis 5 s) et compte dans le journal. La peluche en poche s'affiche sous les cœurs (« Lapin (peluche) »).
- Test : `eon.quete ulysse 0|1`, `gamine 0|1`, `peluche`, `poche N` (N insignes en poche), `log` (modifient la sauvegarde) ; la gamine : `eon.etage` deux fois puis `eon.salle EVENEMENT`. Le pilote automatique va parler à la gamine et à Ulysse.
- Captures : `Docs/J9_gamine_log.png` (la gamine, la peluche en poche, un log), `J9_ulysse_kibo.png` ; vidéos `J9_gamine_calme.mp4`, `J9_ulysse_kibo.mp4`. Ulysse sauvé et la gamine mutée ont été vérifiés dans les journaux (bannières, dialogues), pas filmés.
- **Gamine, Ulysse, équipage, provisoire** :
  - la gamine et Ulysse sont des `AEONNpc` (cylindre et sphère) : le proto a les sprites `gamine` (2 frames : tremble, calmée avec la peluche), `gamine_mutee` (Brainless teinté rose) et `ulysse` ; modèles à produire. La gamine mutée a le modèle du Brainless ;
  - la gerbe de 30 particules à la mutation est faite (Niagara, jalon 10) ; la grille d'aération derrière la gamine ne l'est pas ;
  - log, peluche et insigne sont des sphères (cyan, rose, ambre) : le proto a la feuille `ramassables_quete` (3 frames) ;
  - les couleurs d'Ulysse (veste brune) sont approximatives.

## Jalon 10 : effets Niagara (16/11)
- **Outil** : `UEONVfxLib` (C++, appelable en Python) règle par réflexion les émetteurs Lightweight de Niagara 5.8, que Python ne voit pas. `Saved/Scripts/eon_vfx_build.py` (commandlet) fabrique tout dans `/Game/EON/VFX` : matériaux `M_EON_Particule` et `M_EON_Anneau`, systèmes `NS_EON_Gerbe` (toutes directions), `NS_EON_GerbeDir` (cône ±63°) et `NS_EON_Anneau` (cercle à plat qui grandit en s'effaçant). Paramètres `User.*` : Count, Color, Life, Size, Speed, Drag, Radius, Dir. Le dossier est cuit d'office (`DefaultGame.ini`).
- **Code** : `EONFx.h` reprend `part()` et `{ring:true}` du proto avec ses chiffres (px, px/s, s). Commande de test : `eon.fx anneau|gerbe|dir` sur Jira.
- **Porté** : mort et impacts des ennemis, explosion du kamikaze (gerbe + anneau), bouclier des ennemis, éclosion d'œuf, traînées (slasher, brainless, stalk), bosse du brainless ; boss (coups, chaîne de mort, mortier, cuve brisée) ; Jira (poussière du dash et de la course, retour au bord, bouclier, armure, bout du rayon et du fouet, anneau de l'onde, hologramme, impact de l'ultime, objet pris) ; balles (mur, ricochet) ; events (carte rouge, socles, brèche, confineur, mutation de la gamine).
- **Approximations** : en 3D les éclats partent en demi-sphère vers le haut ; la vitesse est tirée entre 25 et 100 % du maximum ; les gerbes à deux couleurs sont deux systèmes lancés ensemble.
- **Vérifié en jeu** : éclats de mort, anneau du kamikaze, gerbes à deux couleurs (`Docs/J10_eclats_mort.png`, `Docs/J10_anneau_kamikaze.jpg`). Pas encore vu à l'écran : les effets de Jira et ceux des events.
- **Pas porté** : éclats des balles déviées par la lame et l'onde, ambiance des bouches d'aération et des lampes, traînée de fantômes du dash, marqueurs d'apparition, particules d'infestation, vapeur des cuves du C.O.M (encore des sphères), `swapChara`, effets du Mechakuve et du Chercheur.

## Outil : éditeur de salles (17/11)
- `Tools/EditeurSalles/EditeurSalles.html` : éditeur hors ligne (Chrome, Edge, Safari ; Mac et PC), fichiers `.eonsalle` (JSON, une rangée du plan par ligne). Le dossier `Salles/` contient les 18 salles du jeu exportées.
- `Content/Python/eon_salles.py` : import `.eonsalle` -> `/Game/EON/Salles/BP_<nom>` (plan, rôles, forme, poids, portes posées) et export inverse ; aller-retour vérifié identique. Menu « EON Salles » dans l'éditeur (`init_unreal.py`) : pas encore vu dans l'éditeur ouvert.
- `SourceAssets/FBX/` : copie des FBX d'Unity sans les personnages (`chara`, `xeno`, tentacules du boss).
- Bibliothèque d'assets : `Content/Python/eon_assets.py` crée un Blueprint par FBX (`/Game/EON/RoomAssets/.../BPA_<nom>`, mesh recentré : origine au centre de l'empreinte, au sol, sans collision) et écrit à côté de chaque FBX un `.eonasset` (empreinte en tuiles, taille, collision, catégorie, rotations) et un `.eonapercu` (géométrie simplifiée pour les aperçus). Un Blueprint existant n'est jamais recréé : on peut y ajouter scripts et colliders. Les assets dynamiques (Blueprints faits à la main) seront des cas particuliers.
- Objets posés dans une salle : `ChildActorComponent` du Blueprint de salle (tags `eonasset=` / `eonpos=`). Sous un objet solide, le plan du Blueprint reçoit des tuiles `O` (nouvelle tuile `EEONTile::Prop` : bloque comme une caisse, bloc de collision invisible `BlocsObjets`, sans décor). Vérifié en jeu (salle de test à 11 objets).
- Assets dynamiques : Blueprints faits à la main dans `/Game/EON/RoomAssets/Dynamiques`, leurs `.eonasset` dans `SourceAssets/Dynamiques`. Premiers : les lumières (`AEONLampe`, C++) `BPA_Neon_mural` (posé sur une tuile de mur, tourné vers le sol), `BPA_Lampe_sol`, `BPA_Point_lumineux`, avec les règles des lampes du proto (néon mural 52 px / 0,8, lampe au sol 24 px / 0,5 ; clignote, vacille ; éteintes pendant l'Irruption). Réglages par lumière posée (mode, couleur, portée, intensité) : clé `lumiere` de l'objet, tag `eonlum=` en jeu. Vérifié en jeu.
- Placeholder : le trait du néon mural et la dalle de la lampe au sol sont des cubes en `M_EON_Neon` (pas de modèle de lampe dans les FBX).
- Couleur par défaut des lampes : néon de la Coursive (#ff3d8b) ; le proto prend le néon de la palette de la salle, que le portage n'a pas encore (à brancher avec les palettes d'étage).
- Aperçu 3D de l'éditeur en WebGL2 : textures de base exportées en PNG (`SourceAssets/Textures`), couleurs, émissions, opacité, sol et murs de `SourceAssets/Salle.eontheme`, lumières animées, caméra du jeu (55°, FOV 25°). Pas de rendu toon, ni de passe peinture, ni de Lumen. Façades WallA320 posées comme `AEONTileRoom::BuildHubWalls` (17/11 : géométrie et matériaux exportés dans `Salle.eontheme` > `facade`). Les blocs de mur ne se voient plus, ni en jeu ni dans l'aperçu (Vincent, 17/11 : seulement le FBX) ; en jeu ils restent invisibles pour la collision. Vérifié le 18/11 avec `eon.murs` (balayage de la capsule de Jira vers les 4 directions) : arrêt par le composant `Murs` invisible sur une case solide, dans deux salles de combat. Tests sans clavier : `eon.debut "eon.ui continuer;eon.etage;attendre 3;eon.ui passer;eon.go"` et `eon.trace 2` (eon.ou + eon.murs toutes les 2 s).
- FBX modifié (Blender) : « Générer la bibliothèque » le réimporte (empreintes dans `SourceAssets/FBX/eon_fbx_empreintes.json`), remet ses matériaux et recentre le composant « Mesh » de son Blueprint ; bouton Actualiser dans l'onglet Objets de l'éditeur. Testé sur le Casier (date changée) ; le réimport passe par SourceAssets et non plus par la copie Unity, la boîte du Casier a varié de 4 cm.
- Drive maître : `G:/My Drive/EON_EditeurSalles` (`Salles/` lu et écrit par l'import et l'export, `Bibliotheque/` et `EditeurSalles.html` publiés à chaque génération ou par le menu). Le dossier `Salles/` du projet en est le miroir pour Git.
- Habillage des 18 salles (17/11) : bouton « Habiller comme le proto » de l'éditeur (`habiller()`), règles de `renderRoom` : néons sur les murs du haut (h > 0,62 ; clignote si h < 0,72, vacille si h > 0,95), lampes au sol (0,805 < h < 0,82), matériel au pied du mur du haut (k > 0,58, 5 types de `cassetteProp`). Graine tirée du nom de la salle. Correspondances choisies par moi faute d'assets dédiés : terminal = Computer_hitech V4 à V8 + point lumineux (couleur d'écran du proto), câbles = Hitech_medium V5/V7, tuyauterie = Pipe02, lecteur à bandes = Hitech_small V11, plaque gravée = Hitech_small V9/V10. Pas de palette de salle dans le jeu : la couleur des néons est tirée par salle parmi celles de l'étage 1 (#ff3d8b, #ffc23d, #3ef2e0) sans que le sol et les murs suivent. Non repris : câbles au sol, ombres portées, bandes de danger au bord du vide, chevrons devant les portes, décor des tuiles X (terminal / caisse / tonneau : le jeu y met toujours une caisse) ; labos, silos et serres (étages 3 et 4) attendront ces étages. Jusqu'à 39 lumières ponctuelles par salle (Faille_B), sans ombres.
- Canevas « À partir de zéro » (18/11) : Nouvelle salle propose quatre départs, le premier est un canevas (cases « ? » non définies, seule la grille se voit). On trace les murs, on pose les portes de sortie sur les murs (outil P ; clic sur une porte = la tourner ; orientation proposée vers le dehors), puis « Construire la salle » (ou l'enregistrement) déduit la salle : sol dans les murs, mur plein dehors, recadrage sur les murs, forme reconnue si la taille colle, portes converties en côté + position. Refus clair si les murs ne ferment rien, si une porte n'est pas sur le bord ou si deux portes sont du même côté ; Ctrl+Z ramène au canevas.
- Éditeur complété (18/11) : la sélection (M) copie, coupe, supprime, tourne et retourne aussi les objets entièrement dedans (rotation des objets recalculée, lumières gardées) ; outil Objet : clic = choisir un objet posé (glisser pour le déplacer, R tourner sur place, Suppr effacer), Ctrl + clic = poser par-dessus. Onglet Salles : « Couverture » (rôle × forme demandés par le générateur d'étage, d'après `AEONFloorManager` : nombre de salles qui servent, repli Trésor/Irruption → Calme, Cartographe → Événement, cases générées par le jeu faute de salle) et « Tout vérifier » (règles de la Vérification sur toutes les salles). Rôles TRESOR, IRRUPTION, CARTO choisissables comme rôle principal ; une salle Départ ou de forme jamais demandée est signalée. Fichiers : une porte, un objet, une rangée par ligne (47 à 102 lignes au lieu de 187 à 659), même texte écrit par l'éditeur et par `eon_salles.file_text` (vérifié sur les 18 salles) ; un fichier abîmé s'ouvre avec la liste des corrections, un JSON cassé donne un message clair. Constats : `Salle_Combat_Carree` (forme carree) n'était jamais tirée, le générateur ne demande jamais cette forme : passée en forme std 32 × 20 (choix de Vincent, 18/11) ; aucune salle dessinée pour les couloirs corH / corV de Combat, Tension et Calme (le jeu les génère).
- Éditeur en ligne (17/11) : appli web Google Apps Script (`Tools/EditeurSalles/AppsScript/Code.gs`, `appsscript.json`, aussi copiés dans `<Drive>/AppsScript`) qui sert `EditeurSalles.html` depuis le Drive et y lit la bibliothèque et lit/écrit les salles, sans dossier sur l'ordinateur. Le projet Apps Script est créé et déployé par Vincent sur son compte (accès « moi uniquement »). Une salle modifiée ailleurs depuis son ouverture déclenche une demande avant d'écraser. Textures publiées en WebP 1024 px (`texture_web.py`, Pillow du Python système : 0,4 Mo au lieu de 46) ; la bibliothèque est gardée dans le navigateur (IndexedDB), seuls les fichiers changés sont relus. Non vérifié sur le vrai Google : testé avec Code.gs exécuté dans node sur une copie du Drive. Les salles écrites en ligne n'arrivent dans Unreal qu'une fois synchronisées par Google Drive pour ordinateur sur le PC du projet.
