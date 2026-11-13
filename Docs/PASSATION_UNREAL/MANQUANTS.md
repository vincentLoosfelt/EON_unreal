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

## 2. Personnages et créatures à produire

| Asset | Détail | Placeholder | État |
|---|---|---|---|
| Ikiryo : animations | Marche, marche arrière, visée, tir, mort | Modèle FBX sans anim | ❌ |
| Ubume : rig et animations | Marche, ponte, mort | `invocateur.png` | ❌ |
| **Méduse** | Design 2D, modèle, rig, anims (flottement, tir, mort) | `meduse.png` | ❌ |
| **Brainless** | Design 2D, modèle, rig, anims (marche, verrouillage, charge, étourdi, mort) | `brainless.png` | ❌ |
| **Le Tentaculaire** | Corps en 3 phases, 4 tentacules au repos, tentacules d'attaque, œil, sortie de cuve | `boss.png` | ❌ |
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
| Cible d'entraînement | Cube | ❌ |
| Sols des 6 ambiances (étage 1 Coursive) | 🔲 `M_TileFloor` : tuiles 1 m tirées au hasard parmi les 3 couleurs de sol de la palette, joints en couleur `g` (J0) | 🔲 |
| Murs (étage 1 Coursive) | 🔲 Cubes 1 × 1 × 2,5 m, `MI_Mur_Coursive` = couleur `w[1]` (J0) | 🔲 |
| Cuve brisée, cuve à spécimen (4 spécimens ratés) | `cuve_specimen.png` | ❌ |

## 5. Ramassables et icônes

| Asset | Placeholder | État |
|---|---|---|
| Gemme d'ADN | Sphère verte | ❌ |
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
| (à remplir au fil des jalons) | |

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
| | | | | |

## 10. Remarques de design

Ce que le Claude de Vincent a remarqué en portant, à soumettre à Sabri. Ne rien changer sans son accord.

- (J0, technique, pas de design) Les touches clavier sont liées par caractère et non par position physique : ZQSD et WASD sont liés tous les deux. Effet de bord : sur AZERTY, A va aussi à gauche, et sur QWERTY, Q aussi. Aucune action n'utilise A ni Q, donc sans conséquence en jeu.
