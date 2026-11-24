# EON (portage Unreal 5.8) — Fiches de mécaniques par personnage

Relevé en lecture seule du code de `D:/_PROJETS/briscard/EON_UE` le 24/11/2026.
Chemins de code abrégés : `Source/EON/Private/X.cpp` → `X.cpp`, `Source/EON/Public/X.h` → `X.h`.
Données : `Docs/PASSATION_UNREAL/DONNEES/*.json` (source), recopiées dans les DataTables `/Game/EON/Data/DT_Ennemis`, `DT_Greffes`, `DT_GameFeel` (`EONTuningSubsystem.cpp:12-14`).

## Mode d'emploi (pour Vincent)

- **Unité** : tout le gameplay compte en « px du proto ». **1 px = 6,25 cm**, 1 tuile = 16 px = 1 m (`EONTypes.h:13-15`).
- **Trois façons de régler** :
  1. **Menu dev en jeu** : F6 = « TWEAK », F7 = « TRICHE » (`EONDevMenu.cpp:53-124`). Seuls les réglages listés là y sont.
  2. **Console** : `eon.feel <Clé> <valeur>` change **n'importe quel** champ de `DT_GameFeel` en jeu, même absent du menu (`EONTuningSubsystem.cpp:31-43`). `eon.feel.list` les liste.
  3. **Données / code** : `DT_Ennemis` (= `ennemis.json`), `DT_Greffes` (= `greffes.json`), ou constantes C++ (il faut recompiler).
- Colonne « Menu dev » des tableaux : **F6** = réglable dans le menu Tweak ; **console** = seulement via `eon.feel` ; **DT** = DataTable / JSON ; **code** = constante C++.
- Les valeurs `DT_GameFeel` citées sont les **valeurs par défaut du code** (`EONTypes.h`). Le jeu charge la ligne « studio » de `DT_GameFeel` au démarrage (`EONTuningSubsystem.cpp:78`) ; elle peut différer (ex. le commentaire de `EONTypes.h:68` dit `PPPaint` = True dans la DT depuis le 23/11). La valeur réelle se lit en jeu dans F6 (valeur studio affichée en ambre si on l'a changée) ou par `eon.feel <Clé>`.
- « Debug » = élément dessiné avec `DrawDebug*` : provisoire, **invisible dans une build Shipping**.

Personnages couverts : **Jira** + **8 types d'ennemis** (Nuke, petite Nuke, Ikiryo, Onibi, Kama, Ubume, Brainless, Méduse) + variantes (élites, « La chose », ennemis d'event). Hors périmètre : le gardien `AEONBoss` (Tentaculaire, `EONBoss.cpp`), la cible d'entraînement `AEONTrainingDummy`.

---

## Systèmes partagés

### Réglages communs aux ennemis (`DT_GameFeel`, rubrique « Ennemis »)

| Réglage | Champ | Défaut | Effet | Menu dev |
|---|---|---|---|---|
| PV des ennemis | `EnemyHp` | 1 | × sur les PV de base (`EONEnemy.cpp:134`) | F6 « Points de vie (x) » |
| Vitesse des ennemis | `EnemySpeed` | 1 | × vitesse de croisière (`EONEnemy.cpp:441`) | F6 « Vitesse (x) » |
| Écart 3D de vitesse | `SpeedMul3D` | 0,55 | appliqué aussi aux ennemis (`EONEnemy.cpp:441`) ; **pas** aux charges/dash (235 px/s) ni aux tirs | F6 « Vitesse de marche (x) » (agit aussi sur Jira) |
| Dégâts sur Jira | `EnemyDmg` | 1 | cœurs par coup, arrondi ; 0 = inoffensifs (`EONCombatDirector.cpp:546`) | F6 « Dégâts sur Jira (coeurs) » |
| Taille des vagues | `WaveMul` | 1 | × budget de vague (`EONCombatDirector.cpp:299`) | F6 « Taille des vagues (x) » |
| Recul des ennemis | `Knock` | 1 | × recul reçu (`EONEnemy.cpp:360`) | F6 « Recul des ennemis (x) » |
| Flash blanc au coup | `HitFlash` | vrai | flash 0,07 s (`EONEnemy.cpp:358`) | F6 « Flash blanc au coup » |
| Chiffres de dégâts | `DmgNums` | vrai | (`EONEnemy.cpp:377`) | F6 « Chiffres de dégâts » |
| Arrêt sur image | `Hitstop` | 55 ms | à la mort : 100 % ; coup ≥ 3 dégâts : 30 % ; explosion : 60 % (`EONCombatDirector.cpp:415,464`, `EONEnemy.cpp:386`) | F6 « Arrêt sur image (ms) » |
| Tremblement | `Shake` | 1 | × toutes les secousses caméra (`EONPlayerCameraManager.cpp:34`) | F6 « Tremblement (x) » |

### Montée en difficulté et règles communes (code)

- **PV** = `ceil(PV de base × (1 + 0,07 × combatN) × EnemyHp)` (`EONEnemy.cpp:133-134`). `combatN` = salles hostiles entamées dans la run ; console `eon.combatn <n>` (`EONCombatDirector.cpp:70`).
- **Vitesse réelle** = `Spd × SpeedMul3D × EnemySpeed` ; lissage `min(1, dt×8)` ; séparation entre ennemis à moins de 10 px (30 px/s) (`EONEnemy.cpp:448-458, 680-683`).
- **Recul reçu** = `95 × Knock × facteur ÷ (Masse × MassM)` px/s, amorti `dt×9` (`EONEnemy.cpp:359-361, 684-686`). Facteur : balle 1, rayon 1,6, lame 2, onde courte 3,4, explosion 3.
- **Chemin** : champ de distances sur la grille (`ChaseDir`, `EONEnemy.cpp:390-418`).
- **Vide** : un ennemi au sol le traite comme un mur, sauf s'il est poussé (recul > 45 px/s) ou en dash/charge (`EONEnemy.cpp:685-688`). Chute : 0,45 s, rétrécit (0,2 + 0,8k) et s'enfonce de 200 cm (`EONEnemy.cpp:696-701, 714-720`) ; crédit : +1 scrap, ADN, texte « Chute +1 » ambre (`EONCombatDirector.cpp:495-508`). Pas de cadavre.
- **Hitbox** : capsule couchée nord-sud à hauteur du bras de Jira (17 px = 106 cm), rayon `Ehr`, demi-longueur `Ehr + Ehh/2` (`EONEnemy.cpp:242-254`). Depuis le 23/11, HH/HR sont remis à l'échelle de la **hauteur imposée du modèle** (`ModelHCm`) (`EONEnemy.cpp:125-129`).
- **Contact** (Nuke, petite Nuke, Kama) : 1 cœur si distance < R + 4 + 1 px (`EONEnemy.cpp:702`). Jira a ensuite 1 s d'invulnérabilité.
- **Apparition** : marqueur au sol 0,7 s (+0,05 s par ennemi) (`EONCombatDirector.cpp:384`), carré **magenta** (255,61,139) qui se resserre et clignote — **debug** (`EONCombatDirector.cpp:690-701`).

### Mort et butin (`AEONCombatDirector::KillEnemy`, `EONCombatDirector.cpp:408-440`)

- Hitstop `Hitstop` (55 ms), secousse 0,18.
- **Gerbe de mort** (Niagara `NS_EON_GerbeDir`) : 18 éclats dans le sens du coup, 1 sur 3 blanc, 170 px/s, 0,6 s, taille 1,3 px, frein 5, à 4 px du sol (`:423`). Couleur par type (`:420-422`) : Nuke `#e8a24a`, petite Nuke `#6fbf4a`, Onibi `#ff7a1a`, Méduse `#8f7be0`, Ikiryo `#e8923a`, Kama `#e0405a`, **Brainless et Ubume : blanc `#ffffff`** (absents de cette table, alors qu'ils ont une couleur à l'impact : incohérence à signaler).
- Butin : ADN du type (+6 si élite) ; scrap avec `12 % × ADN × ScrapM` de chances (+3 si élite) ; élite : 1 ramassable « Donnée » de valeur 2 ; énergie 25 % ; cœur 4 % (`:427-438`).
- **Aucun son** de coup ni de mort ennemie (les clés `hit`, `kill` du proto ne sont pas portées, MANQUANTS §8).

### Impact sur un ennemi (`AEONEnemy::HitPx`, `EONEnemy.cpp:352-388`)

| Élément | Détail | Réglage |
|---|---|---|
| Flash blanc | tous les matériaux remplacés 0,07 s par une MID de `/Game/EON/Materials/BG/MI_BG_pipe` avec `BC1` = blanc et `EmisAdd` = (6,6,6) (`EONEnemy.cpp:223-228, 231-240, 743-744`) | `HitFlash` (F6) ; intensité = `EmisAdd` (code) |
| Éclaboussure « chair » | 4 éclats vers le tireur (3 couleur du mob + 1 blanc), 105 px/s, 0,3 s, frein 5, à mi-hauteur (`:367-371`) | code |
| Éclats blancs | 4 éclats blancs dans le sens du coup, 100 px/s, 0,25 s (`:373`) | code |
| Chiffre de dégât | texte **debug** (`DrawDebugString`), blanc, **ambre** (255,194,61) si ≥ 3 ; au-dessus de la tête (+8 px) ; vie 0,6 s, monte à 38 px/s, gravité 60 (`EONEnemy.cpp:377-385`, `EONCombatDirector.h:72`, `EONCombatDirector.cpp:673-688`) | `DmgNums` (F6) |
| Couleurs d'impact par type | Nuke `#e8a24a`, petite Nuke `#6fbf4a`, Onibi `#ff7a1a`, Méduse `#8f7be0`, Ikiryo `#e8923a`, Kama `#e0405a`, Brainless `#c86a6a`, Ubume `#8a6aaa`, autre `#c84a7a` (`EONEnemy.cpp:367-369`) | code |
| Recharge de l'ultime de Jira | +dégâts × 0,8 (`EONEnemy.cpp:375`, `EONCharacter.h:161`) | code |
| Trous dans le sprite (punchHole du proto) | **non porté** (`EONEnemy.cpp:362`) | — |

### Barre de vie et nom (ennemis)

- Barre **debug** au-dessus de la tête (hauteur `Ehh + 4` px), visible si blessé ou élite ; demi-largeur 6 px (élite 10 px) ; fond (42,16,32), remplissage rouge (255,61,85), épaisseur 3 (`EONEnemy.cpp:780-788`).
- **Nom affiché seulement pour les élites** : `DisplayName` (« Kama élite »…) en rose (255,176,192), échelle 1,1, **debug** (`EONEnemy.cpp:789-792`). Le nom vient de `OfficialName` de `DT_Ennemis`, parenthèse coupée (`EONEnemy.cpp:31-36,130`) : « Meduse », « Brainless » sans accent.
- Le HUD (`EONHUD.cpp`) n'affiche aucune barre d'ennemi ordinaire (seulement gardien et Cartographe).

### Élites (`AEONEnemy::MakeElite`, `EONEnemy.cpp:150-159` ; tirage `EONCombatDirector.cpp:364-378`)

- Seulement 1re vague d'une salle, hors Corridor et Faille, `combatN ≥ 3`, 40 % ; parmi Kama, Ikiryo, Brainless, Ubume ; puis 2 salles sans élite (`EliteCool`).
- PV × 2,2 (arrondi), rayon R × 1,3, masse × 2 (`MassM`), modèle × 1,45 (`ModelRoot`), hitbox `Ehh` × 1,45 et `Ehr` × 1,3 (`EONEnemy.h:64-65`).
- Visuel : aucune teinte ni aura, juste plus gros + nom et barre debug.

### Cadavres (`BeginCorpse` / `Tick`, `EONEnemy.cpp:307-344`)

- Une anim de mort au hasard (ou l'explosion qui continue pour l'Onibi), puis **0,6 s** au sol, puis s'enfonce de sa hauteur en **0,5 s** et disparaît. Filet : durée de vie = total + 2 s.
- Pas d'anim de mort (Ikiryo, Ubume, Brainless, Méduse) ou chute dans le vide : disparition immédiate (`EONCombatDirector.cpp:811-816`).
- Durées en dur dans le code (0,6 s / 0,5 s).

### Rendu celluloïd, trait et 12 i/s

| Élément | Où | Réglage |
|---|---|---|
| Marquage « celluloïd » | `UEONStatics::MarkCelComponent` : CustomDepth + stencil **1** (trait + étalonnage cel) ou **2** (cel sans trait) (`EONStatics.cpp:64-79`) | code |
| Persos et ennemis | stencil 1 (avec trait) (`EONEnemy.cpp:204`, `EONCharacter.cpp:126`) | — |
| Projectiles et œufs | stencil 2, **sans trait** (Vincent 24/11) (`EONProjectile.cpp:33`, `EONEnemyProjectile.cpp:44,92`) | — |
| Animation en paliers | squelettes évalués `CelAnimFps` fois/s (tick du composant = 1/12 s) ; le déplacement reste fluide (`EONStatics.cpp:69-78`) | `CelAnimFps` = 12 (0 = fluide) — **console seulement** (pas dans F6), et lu à la création du composant |
| Trait (line art) | post-process `M_PP_EON_Trait`, instance `MI_PP_EON_Trait` (`SilWidth` × 0,5, `SilMin` 2 px) dans les volumes de L_HUB et L_Etage (`Saved/Scripts/eon_trait_mi.py`). Autres paramètres du matériau : `InkHue`, `InkValue`, `InnerRatio`, `InnerAlpha`, `ShadowOffX/Y`, `ShadowStrength`, `ShadowSoft`, `Jitter`… | paramètres matériau (éditeur) |
| Post-process anime (trait + film) | `PPPaint` met le poids des blendables à 1/0 (`EONPlayerController.cpp` `ApplyPostProcess`) | F6 « Post-process anime (trait + film) » |
| Post-process complet | `PPAll` active/désactive les volumes | F6 « Post-process complet » |
| Matériau des ennemis du studio | `M_EON_ToonEnnemi` (Substrate Toon opaque, sans normal map) + profil `TP_EON_Anime`, instances `MI_<Nom>` (paramètre texture `BaseTex`) (`Saved/Scripts/eon_ennemis_import.py:2,65`, `eon_ennemis_toon.py`) | éditeur |

### Effets Niagara (`EONFx.cpp`)

Trois systèmes, pilotés par des paramètres utilisateur :
- `/Game/EON/VFX/NS_EON_Gerbe` (éclats en boule) : `User.Count`, `User.Color`, `User.Life`, `User.Size`, `User.Drag`, `User.Speed`, `User.Radius`.
- `/Game/EON/VFX/NS_EON_GerbeDir` (éclats en cône) : idem + `User.Dir` (relevé de 0,35 vers le haut).
- `/Game/EON/VFX/NS_EON_Anneau` (anneau au sol) : `User.Color`, `User.Life`, `User.Size` (diamètre).
- Matériaux : `M_EON_Particule`, `M_EON_Anneau`. `Gerbe2`/`GerbeDir2` = deux couleurs (1 éclat sur N de la 2e couleur).
- Test : console `eon.fx anneau|gerbe|dir` (`EONFx.cpp:102-112`).
- Chaque appel donne ses chiffres en dur dans le code (nombre, couleur, vitesse px/s, vie s, taille px, frein).

### Caméra et impacts (`EONPlayerCameraManager.cpp`)

- Secousse (« trauma ») : `AddTrauma(v × Shake)`, plafond 1, baisse 1,8/s, amplitude 8 × trauma² px (`:31-35, 103-106`).
- Kick : `AddKick(dir, px × CamKick)`, amorti `dt×14` (`:23-28,104`). `CamKick` : F6 « Kick au tir (x) ».
- Hitstop : dilatation du temps global à 0,0001 pendant X ms réelles ; fige tout, effets compris (`EONTuningSubsystem.cpp:93-114`).

### Tirs ennemis (`AEONEnemyProjectile`, `EONEnemyProjectile.cpp`)

| | Petit tir (Méduse) | Gros tir (Ikiryo) |
|---|---|---|
| Classe | `AEONEnemyProjectile` | idem, `bBig = true` |
| Maillage | `/Engine/BasicShapes/Sphere` (`:45`) | idem |
| Matériau | MID de `/Game/EON/Materials/M_Flat`, paramètre `Color` (`:22-29`). `M_Flat` = Base Color + Roughness 0,8, **sans émission** (`Saved/Scripts/eon_setup_j0.py:230-236`) | idem |
| Couleur | magenta `#ff3d8b` × 4 (`:58-59`) | orange `#e8923a` × 4 |
| Taille | rayon 2 px → sphère de 25 cm (`:60`) | rayon 3 px → 37,5 cm |
| Hauteur de vol | 9 px (56 cm) | 13 px (81 cm) |
| Vitesse | 82 px/s (5,1 m/s) | 170 px/s (10,6 m/s) |
| Vie | 3 s, meurt au mur (`:73-82`) | 3 s |
| Touche Jira | à moins de 4 + 1,5 px de sa capsule (`EONCombatDirector.cpp:36-41, 636-640`) : 1 cœur | idem |
| Traînée, halo, lumière | **aucun** (MANQUANTS §7) | aucun |
| Ombre / trait | pas d'ombre, pas de trait (stencil 2) | idem |
| Son au départ | `SFX_Ennemi_Tir_01/02`, vol 0,32, hauteur ±8 %, 1 par volée (écart 80 ms) (`EONCombatDirector.cpp:478`) | vol 0,5, hauteur ±3 % |
| Réglages | tout en code (`EONEnemy.cpp:548`, `:58-60`) | (`EONEnemy.cpp:604-605`) |

Annulés par : Lame/Fauche dans l'arc, onde de Jira dans son rayon, bouclier NV3 (nettoyant 110 px ou miroir 120 px), hologramme (absorbe, −1 PV leurre) (`EONCharacter.cpp:1196-1205, 1263-1266`, `EONCombatDirector.cpp:630-635, 824-864`).

### Sons (`EONSfx.cpp`)

Dossier `/Game/EON/Audio/Placeholders/`. Sons présents : `SFX_Ennemi_Tir_01/02`, `SFX_Jira_Tir_01/02/03`, `SFX_Jira_Laser`, `SFX_Jira_Ultime_Rayon`, `SFX_Proto_alarm/card/door/explode/hiss/hurt/item/lift/roar`. Réglages `Sound`/`Vol` de `DT_GameFeel` **non branchés** (MANQUANTS l. 233). Côté combat, seuls 4 sons sont joués : tir de Jira, laser, rayon d'ultime, tir ennemi (+ rugissement de « La chose »).

### Vagues (`SpawnWave`, `EONCombatDirector.cpp:295-390`)

Budget = `min(13, (5 + 0,75 × combatN) × surface × mul × WaveMul)`, ÷2 à l'étage 4. Coût / combatN mini : Nuke (meute 3–4) 3,2 / 1 ; Onibi 1,1 / 1 ; Méduse 1,7 / 2 ; Ikiryo 1,9 / 2 ; Brainless 1,8 / 2 ; Kama 2,1 / 3 ; Ubume 2,6 / 3 (`:325-327`). Étage 2+ : Brainless, Méduse, Ubume ×2 de chances. Corridor : Nuke, Onibi, Kama, Brainless, du fond du couloir. Vague suivante quand il reste 1 ennemi (`:578-582`). Console : `eon.wave [mul]`, `eon.combat [vagues]`, `eon.kill` ; F7 « Lancer une vague », « Tuer les ennemis ».

---

## Jira (joueuse)

Classe `AEONCharacter` (`EONCharacter.h/.cpp`), Blueprint `/Game/EON/Blueprints/BP_Jira`, contrôleur `AEONPlayerController`, HUD `AEONHUD`.

### Modèle, matériaux, animations

| Élément | Nom / chemin | Détail |
|---|---|---|
| Maillage | `/Game/EON/Characters/Jira/SK_Jira_UEFN` (squelette du mannequin UEFN) | 198 cm casque compris, mis à 181 cm (29 px) (`EONCharacter.cpp:73-81`) ; lacet −90° |
| Matériaux | `MI_Jira_Corps`, `MI_Jira_Cheveux` ; textures `T_Cyborg_BaseColor`, `T_Cyborg_BaseColor_2` (`Content/EON/Characters/Jira/`) | |
| Locomotion | ABP `/Game/Blueprints/SandboxCharacter_CMC_ABP` (Game Animation Sample 5.8), via `BPI_SandboxCharacter_Pawn` → `UEONStatics::FillAnimProps` (`EONCharacter.cpp:69-72, 113-118`) | |
| Couche de tir | `A_Jira_shoot_front` sur le haut du corps (depuis `spine_01`), poids `EONTirAlpha` lissé à vitesse 14 (`EONCharacter.cpp:592-602`, `Saved/Scripts/eon_jira_jeu.py`) | |
| Anims présentes mais **non utilisées** par le code | `A_Jira_charged_attack`, `A_Jira_dash`, `A_Jira_death`, `A_Jira_hacking`, `A_Jira_heavy_hit`, `A_Jira_knockback`, `A_Jira_light_attack`, `A_Jira_light_hit`, `A_Jira_shoot_back/left/right` (`/Game/EON/Characters/Jira/Anims/UEFN/`) | aucune référence dans `Source` |
| Rendu | celluloïd avec trait, squelette à 12 i/s (`EONCharacter.cpp:126`) | `CelAnimFps` |
| Orientation | regarde toujours vers la visée (`EONCharacter.cpp:591`) | |

### Valeurs de base (`EONCharacter.h:180-205`, propriétés éditables du Blueprint)

| Élément | Champ | Valeur | Réglage |
|---|---|---|---|
| Marche | `WalkSpeedPx` × `SpeedMul3D` | 82 × 0,55 = 45 px/s (2,8 m/s) (`EONCharacter.cpp:554`) | `SpeedMul3D` F6 « Vitesse de marche (x) » ; `WalkSpeedPx` dans BP_Jira |
| Course | × `RunMul` | 1,7 (4,8 m/s) | F6 « Vitesse de course (x) » |
| Déclenchement course | garder le dash > `RunHoldTime` (0,2 s) **ou** Maj maintenue (`EONCharacter.cpp:505-506`) | | BP_Jira |
| Lissage de l'entrée | `min(1, dt×16)` (`:549-550`) | | code |
| PV | `StartHP` 6 (+ bonus méta des insignes) (`:153-156`) | | BP_Jira |
| Énergie | `EnergyMax` 100, regen `EnergyRegen` 16/s après `EnergyRegenDelay` 0,7 s (`:1054-1057`) | | BP_Jira ; triche F7 « Énergie infinie » ; le champ `Regen` de DT_GameFeel n'est **pas** branché |
| Capsule | rayon 4 px (25 cm), hauteur 29 px | | BP_Jira |
| Bout du bras (départ des tirs) | 17 px de haut (106 cm), 3 px de côté, 12 px devant (`EONTypes.h:22`, `EONCharacter.cpp:681-693`) | | code |

### Dash (`PressDash`, `EONCharacter.cpp:610-632` ; mouvement `:541-546`)

| Élément | Champ | Valeur | Réglage |
|---|---|---|---|
| Vitesse | `DashSpeedPx` × `DashMul` × `DashDistM` (objet) | 345 px/s (21,6 m/s), **pas** de `SpeedMul3D` | F6 « Distance du dash (x) » (`DashMul`) |
| Durée | `DashDuration` | 0,15 s (≈ 52 px = 3,2 m) | BP_Jira |
| Invulnérabilité | `DashInvulnerability` | 0,2 s | BP_Jira |
| Charges | `DashMax` | 2 (+1 par objet `p_dashc`) | BP_Jira ; F7 « Dash infini » |
| Recharge | `DashCooldown` × `CdM` | 1,1 s par charge | BP_Jira |
| Direction | déplacement en cours, sinon visée | | code |
| Secousse | 0,06 | | code |
| Poussière | 8 grains gris-violet `#8a84b8` vers l'arrière, 80 px/s, 0,3 s (Niagara GerbeDir) (`:631`) | | code |
| Poussière de course | 1 grain `#8a84b8` toutes les 0,07 s, 20 px/s, 0,35 s (`:507-514`) | | code |
| Traînée de fantômes, son, anim dédiée | **non faits** (`A_Jira_dash` inutilisée ; MANQUANTS §7) | | |

### Visée (`ApplyAimAssist`, `EONCharacter.cpp:516-530, 636-679` ; contrôleur `UpdateMouseAim` / `UpdateStickAim`)

- Souris : vers le curseur ; manette : stick droit (garde l'angle 0,45 s ; stick à fond > 0,75 = tir).
- Aide à la visée : cibles à moins de 220 px, cône `0,12 + 0,4 × force` rad, tire l'angle de `force × 0,8`. Force = `AimAssist` (0,15) × facteur d'armure 0,4 / 0,6 / 0,8 / 1. Réglage F6 « Aide à la visée ».
- En course : regarde où elle court, pas de tir. Mutation stade 3 : la visée tremble ±0,175 rad.
- Réticule : **aucun** (MANQUANTS §7 : « réticule qui s'écarte » non fait).

### Greffes (armes du bras) — données `DT_Greffes` (= `greffes.json`)

Sélection : `IA_Greffe1..5`, `IA_GreffeNext/Prev` (`EONPlayerController.cpp:94-98, 324-326`). Ordre de Jira (`JiraOrder`) : Standard, Rafale, Laser, Lame, Shotgun. Les verrous méta (`GreffeOk`) retirent les greffes non débloquées (Jira démarre avec Standard et Rafale ; F7 « Tout débloquer »). Changer de greffe annule charge et salve (`EONCharacter.cpp:298-306`). HUD : « Cobra : <nom> » en bas à gauche, couleur de la greffe (`EONHUD.cpp:141-158`).

| Greffe (clé DT) | Type | Cadence / charge | Dégâts | Vitesse | Dispersion | Vie | Coût | Kick | Couleur `Col` |
|---|---|---|---|---|---|---|---|---|---|
| Standard (`standard`) | auto | 9 tirs/s | 1 | 250 px/s | 3° | 0,9 s | 0,7 | 1 | `#3ef2e0` cyan |
| Rafale (`rafale`) | burst | charge 0,7 s → 2 à 8 balles, 1 / 0,045 s | 1 / balle | 235 | 14° | 0,8 | 0,9 / balle | 1,3 | `#ffc23d` ambre |
| Laser (`laser`) | beam | charge 0,6 s (part si > 20 %) | 6 × charge (min 1) | instantané, 300 px | — | — | 10 × (0,4 + 0,6 × charge) | 3,2 × (0,5 + 0,5 × charge) | `#ff6fd8` rose |
| Lame (`lame`) | melee | 3,2 coups/s | 3 | — | arc 2 rad, portée 22 px | — | 0 (ne nourrit pas la mutation) | 2,2 (non utilisé par `DoSlash`) | `#b8f07a` vert |
| Shotgun (`shotgun`) | spread | 1,8 tirs/s, 7 plombs | 1 / plomb | 240 ± 20 % | 40° | 0,32 | 3 | 3,6 | `#ff9a3d` orange |
| Griffe (`griffe`) | lash | 2,4 /s | 2 | — | ligne 78 px, largeur 5 px, ramène × 1,3 | — | 1,6 | 2 | `#e0503a` |
| Rayon (`rayon`) | beam | charge 0,5 s | 5 × charge | 220 px | — | — | 9 × (…) | 3 | `#ffa040` |
| Fauche (`fauche`) | melee | 1,6 /s | 4 | — | arc 3,4 rad, portée 32 px | — | 2 | 3 | `#ff6a5a` |

Griffe, Rayon et Fauche ont `JiraOrder` 0 : greffes du **Cobaye** (perso non jouable), absentes du cycle de Jira (`EONCharacter.cpp:957-972`) ; les objets `k_*` qui s'y rattachent n'ont pas d'effet (`:1472`).

Multiplicateurs communs (DT_GameFeel, F6 rubrique « Jira : tir ») :

| Réglage | Champ | Défaut | Agit sur | Menu |
|---|---|---|---|---|
| Vitesse des balles | `BulletSpeed` | 1 | vitesse des projectiles (`EONCharacter.cpp:734`) | F6 |
| Portée | `RangeMul3D` | 1 | durée de vie des projectiles (`:750`) | F6 |
| Cadence | `RateMul` | 1 | cadence et vitesse de charge (`:718, 1086, 1113`) | F6 |
| Dispersion | `Spread` | 1 | cône (`:732`) | F6 |
| Recul de Jira | `Recoil` | 1 | recul au tir (`:767`) | F6 |
| Coût en énergie | `EnergyCost` | 1 | coût de chaque tir | F6 |
| Flash de bouche | `Flash` | vrai | lumière de bouche | F6 |
| Kick caméra | `CamKick` | 1 | recul caméra | F6 |
| Douilles | `Shells` | vrai | **non branché** (pas de douilles) | retiré de F6 |
| Mutation | — | — | cadence × 0,8 dès le stade 1 | code |

#### Le projectile de Jira (Standard, Rafale, Shotgun, et tirs renvoyés par le bouclier miroir)

| Élément | Nom / valeur | Source | Réglage |
|---|---|---|---|
| Classe | `AEONProjectile` | `EONProjectile.h/.cpp` | — |
| Collision | `USphereComponent` « Sphere », rayon (1 + `Size`) px = 6,25 cm (+6,25 cm par objet « Balles épaisses » `p_thick`) ; bloque murs et créatures, ignore les autres projectiles | `EONProjectile.cpp:21-27, 62`, `EONCharacter.cpp:750` | code / objet |
| Maillage visible | « Visual » = `/Engine/BasicShapes/Sphere` étirée en trait **50 cm × 10 cm × 10 cm** (échelle 0,5 / 0,1 / 0,1), sans ombre | `EONProjectile.cpp:29-37` | **code** (`SetRelativeScale3D`) |
| Matériau | MID de `/Game/EON/Materials/M_Flat`, paramètre `Color` = `Col` de la greffe ; **non émissif** (Base Color + Roughness 0,8) | `EONProjectile.cpp:78-83` | couleur : `DT_Greffes.Col` ; luminosité : il faudrait un matériau émissif |
| Trait / cel | celluloïd **sans trait** (stencil 2) | `:33` | — |
| Mouvement | `UProjectileMovementComponent` « Movement », sans gravité, oriente le trait selon la vitesse | `:39-51` | — |
| Vitesse | `Spd` × `BulletSpeed` × (0,94 à 1,06) ; Standard 250 px/s = 15,6 m/s | `EONCharacter.cpp:734` | DT_Greffes `Spd`, F6 |
| Durée de vie | `Life` × `RangeM` (objet) × `RangeMul3D` | `:750` | DT_Greffes `Life`, F6 « Portée » |
| Départ | bout du bras ; multi-canon (`j_multi`) : 2 balles à ±3 px, puis 3 à −5/0/+5 px | `:737-753` | objet |
| Perforation / ricochet | objets `j_pierce` (traverse N ennemis), `j_bounce` (N rebonds, point blanc 0,15 s au rebond) | `EONProjectile.cpp:95-116` | objet |
| Impact mur | 4 éclats couleur du tir, vers l'arrière, `0,2 × vitesse + 35` px/s, 0,2 s | `EONProjectile.cpp:127-131` | code |
| Impact ennemi | voir « Impact sur un ennemi » | | |
| Traînée | **aucune** | | |
| Son | `SFX_Jira_Tir_01..03` au hasard, vol 0,45, hauteur ±7 %, écart mini 0,03 s (un par appel de `SpawnBullet` : le Shotgun n'en joue qu'un pour 7 plombs) | `EONCharacter.cpp:753` | code |
| Champ de force du C.O.M | une balle qui traverse le dôme du HUB y lance une onde orange (`AEONChampForce::Impact`, `MPC_EON_Champ`) | `EONChampForce.cpp:17-25` | — |

À noter : l'objet « Balles épaisses » (`Size`) grossit la **collision**, pas le trait visible.

#### Mise en scène d'un tir (`OnShot`, `EONCharacter.cpp:756-777`)

| Élément | Valeur | Réglage |
|---|---|---|
| Flash de bouche | `UPointLightComponent` « MuzzleLight » : intensité 6000, rayon 66 px (4,1 m), sans ombre, couleur = `Col`, 0,06 s, au bout du bras + 20 cm (`EONCharacter.cpp:84-90, 769-775`) | `Flash` (F6) ; intensité/rayon/durée : code |
| Sprite de flash | **non fait** (MANQUANTS §7) | — |
| Kick caméra | `kick × 1,6` px vers l'arrière | `CamKick` (F6) |
| Secousse | `0,04 × kick` | `Shake` (F6) |
| Recul de Jira | `kick × 14` px/s | `Recoil` (F6) |
| Douille | **non faite** | (`Shells` non branché) |
| Shotgun : arrêt sur image | `Hitstop × 0,15` (`:1100`) | `Hitstop` |

#### Rayons Laser / Rayon (`DoBeam`, `EONCharacter.cpp:1145-1178`)

- Trait **debug** (`DrawDebugLine`), couleur `Col`, épaisseur `2 + 1,5 × Size + 2 × charge` px, 0,14 s ; s'arrête au mur, traverse tous les ennemis (largeur de touche `Ehr + 0,35 × Ehh + W`).
- Bout : 10 éclats couleur `Col`, 85 px/s, 0,3 s.
- Son `SFX_Jira_Laser`, volume `0,5 + 0,4 × charge`, hauteur ±4 %.
- Secousse `0,15 × charge` ; charge > 95 % : hitstop × 0,5. Recul ennemi × 1,6.
- Barre de charge HUD : 40 × 1,5 (unités écran), couleur `Col`, blanche à 100 % (`EONHUD.cpp:149-155`).

#### Lame / Fauche (`DoSlash`, `EONCharacter.cpp:1180-1219`)

- Zone : rayon `Range × (1 + 0,15 × Size) × √RangeM`, arc `Arc + 0,15 × Size` ; recul ennemi × 2 ; **détruit les tirs ennemis** dans l'arc.
- Jira avance de 40 px/s ; kick caméra 2 px vers l'avant.
- Visuel : arc **debug** de 8 segments, couleur `Col`, épaisseur 6, 0,12 s, rayon R + 4 px. Pas de son, pas d'anim (MANQUANTS §3 : anim de mêlée à faire).

#### Griffe (`DoLash`, `EONCharacter.cpp:1221-1248`) — Cobaye seulement

Ligne **debug** épaisseur 8, 0,18 s ; 6 éclats `Col` + lilas `#d8d4f0`, 64 px/s, 0,25 s ; ramène les ennemis (`Pull` 1,3) ; hitstop × 0,35 si touche.

### Sorts (`UpdateSpells`, `EONCharacter.cpp:1292-1363`)

| Sort (touche HUD) | Mécanique | Coût / recharge | Visuel | Son |
|---|---|---|---|---|
| **Onde courte** (E, `IA_Spell`) | appui < 0,25 s : rayon 56 px, 1 × `DmgMult`, recul × 3,4, efface les tirs ennemis du rayon (`:1309`, `Shockwave :1250-1277`) | 18 ; 0,9 s × `CdM` | anneau Niagara magenta `#ff3d8b` du rayon, 0,3 s + 24 éclats magenta/blanc 180 px/s 0,25 s ; secousse 0,25 ; hitstop × 0,5 | aucun |
| **Onde continue** | maintenu > 0,25 s : pulsation toutes les 0,22 s, rayon 44 px, 1,2 dégât, recul × 1,7 ; Jira à mi-vitesse (`:1313-1319, 553`) | 30 / s | anneau magenta 0,18 s | aucun |
| **Hologramme** (F, `IA_Holo`) | leurre 5 s / 8 PV ; ennemis à < 240 px le visent ; au contact il perd 5 PV/s (mobs de contact) ou 2 ; un tir ennemi lui ôte 1 (`CastHolo :1279-1290`, `EONCombatDirector.cpp:800-806, 630-635`) | 28 ; 7 s × `CdM` | apparition : 16 éclats cyan `#3ef2e0` ; corps : **capsule debug cyan clignotante** (rayon 6 px) ; fin : 22 éclats cyan/blanc | aucun |
| **Ultime** (R, `IA_Ultimate`) | jauge 0–100 (+0,8 × dégâts infligés) ; QTE « Clavier thoracique » : 5 flèches en 3,2 s (`StartQTE`, `UpdateQTE :1365-1410`) ; raté → jauge à 50 (« Sequence ratee » / « Trop lent ») ; réussi → « Protocole thoracique » : un rayon par ennemi, l'un après l'autre (0,15 + 0,09 × k s), 14 dégâts (30 sur un gardien), recul × 2,5 (`UltFire :1412-1420`) | jauge pleine | cut-in HUD 1,1 s (bande sombre, titre blanc, sous-titre cyan « Frappe de saturation… ») ; hitstop 1 s ; chaque frappe : **ligne debug cyan** épaisseur 40 tombant de 30 m, 0,25 s + 26 éclats cyan/blanc ; secousse 0,35 | `SFX_Jira_Ultime_Rayon` vol 0,7 par frappe |

Sorts bloqués dès la mutation stade 2 (bannière « Spells bloques »). Valeurs de sorts : toutes en **code** (aucune dans DT/F6). HUD : 3 emplacements E/F/R avec barre de recharge, cyan prêt / gris en recharge / rouge sombre bloqué ; « PRET » qui clignote (`EONHUD.cpp:160-180`).

### PV, coups reçus, bouclier, armure (`HurtPlayer`, `EONCharacter.cpp:203-247`)

| Élément | Valeur | Visuel / son | Réglage |
|---|---|---|---|
| Coup reçu | 1 cœur × `EnemyDmg` ; × 2 en mutation (stade ≥ 1) | flash rouge plein écran 0,25 s (`EONHUD.cpp:91-92`) ; secousse 0,55 ; hitstop `Hitstop × 1,2 + 30` ms ; recul 120 px/s ; **pas de son, pas d'anim** (`A_Jira_light_hit`/`heavy_hit`/`knockback` inutilisées) | `EnemyDmg`, `Hitstop` (F6) ; F7 « Invincible » |
| Invulnérabilité après coup | 1 s, sans clignotement visible | — | code |
| Bouclier (cassette Shield / objet `c_shield`) | absorbe 1 coup, 0,6 s d'invulnérabilité ; NV2 : recharge après 10 s sans dégât ; NV3 : onde nettoyante (110 px, 5 dégâts aux ennemis à < 80 px) ou miroir (tirs renvoyés × 1,6, 2,5 dégâts, couleur cyan) | texte « Bouclier » cyan ; 20 éclats cyan ; anneau cyan 80 px ou blanc-bleu 46 px ; carré cyan après les cœurs au HUD | code / méta |
| Blocage d'armure | chance `ArmBlock × niveau` (0,2 par niveau) | texte « Bloque » rose `(224,90,138)` ; 12 éclats rose/blanc | `ArmBlock` (F6) |
| Renaissance (objet `p_revive`) | revient à la moitié des PV, 2 s d'invulnérabilité, bannière « Renaissance » (`:249-259`) | — | objet |
| Mort | « Signal perdu » + bilan (debug `GEngine`) ; relance R / clic / A (`EONCombatDirector.cpp:724-754`) | anim `A_Jira_death` **non utilisée** | — |
| Chute dans le vide | vrai trou, kill zone 10 m plus bas, retour au sol sûr le plus proche, −`FallDmg` cœur, 1,2 s d'invulnérabilité, 14 éclats cyan, secousse 0,4 (`VoidRespawn :344-384`) | pas d'anim de chute | `FallDmg` (F6 « Dégâts de chute (x) ») |

### Énergie, mutation, armure (`Spend :697-713`, `UpdateMutation :1015-1081`)

| Élément | Valeur | Visuel | Réglage |
|---|---|---|---|
| Énergie à 0 | mutation stade 1 (le tir ne coûte plus rien) | bannière rouge « Mutation - stade 1 » ; secousse 0,35 | — |
| Montée de stade | toutes les `MutDur` (4 s) si Jira dépense encore (× 2 avec `e_stab`, × (1 − `ArmMut` × armure)) | bannière « Mutation - stade N » | `MutDur`, `ArmMut` (F6) |
| Retour | −1 stade toutes les 1,2 s sans dépense ; état sain → 30 % d'énergie, +1 niveau d'armure (max 3) | « Etat sain » / « Etat sain - l'armure reste » | code |
| Effets | stade 1 : coups × 2, cadence × 0,8 ; stade 2 : vitesse × 0,8, sorts bloqués ; stade 3 : visée qui tremble, dérapage ±20 px/s | HUD : voile rouge qui pulse, stade 2 aberration, stade 3 bandes et neige (`EONHUD.cpp:68-89`) ; barre d'énergie rouge clignotante « Mutation I/II/III » | code |
| Armure permanente | énergie max − `ArmCost` (12 %) par niveau ; blocage `ArmBlock` ; aide à la visée renforcée | montée : 34 éclats prune `#a03a64`/rose `#e05a8a` ; descente : 16 grains sombres `#3a3850` ; bannières « Le bras se transforme », « L'armure pousse », « Casque : ciblage assiste en ligne » ; HUD « Armure N » | `ArmCost`, `ArmBlock` (F6) |
| Corps qui change (bras, armure, casque) | **non fait** : le maillage ne change pas (MANQUANTS §3) | | |

### Interface de Jira (`EONHUD.cpp:94-139`)

Cœurs = carrés rouges (vides : rouge sombre) ; bouclier = carrés cyan ; barre d'énergie 60 unités cyan (blanche clignotante sous 25 %), part perdue par l'armure hachurée rose ; 2 barrettes de dash violettes `(138,132,184)` ; barre d'ADN verte « Niv N », « Scrap N » ambre, « Donnees N » bleu clair. Dessinés en `Canvas` (pas de widget UMG), échelle = hauteur d'écran / 270.

### Objets qui changent Jira (`TakeItem`, `EONCharacter.cpp:1441-1478`)

Vitesse (`d_spd` × 1,08, `n_plaque` × 0,94), dégâts (`d_dmg` × 1,15, `e_berserk` +35 %/stade), cadence (`p_rate` × 1,12), épaisseur (`p_thick`), portée (`p_range` × 1,2), charge (`p_charge` × 0,8), dash (`p_dashd` × 1,25, `p_dashc` +1), recharges (`d_cd` × 0,88), perforation, ricochet, multi-canon, bouclier, Renaissance… Prise d'objet : bannière couleur de rareté + 30 éclats de cette couleur, 113 px/s, 0,6 s.

### Triches (F7, `EONDevMenu.cpp:104-124`)

Invincible (`God`), Énergie infinie (`InfEnergy`), Dash infini (`InfDash`), Tue en un coup (`OneShot`, dégâts × 1000), Soigner, +100 ADN, +100 scrap, +50 Données, Tout débloquer, Pilote automatique (`eon.auto`).

### Manquant / provisoire (Jira)

- Rayons, arcs de lame, fouet, frappes d'ultime, hologramme : **lignes/capsules debug**, invisibles en Shipping.
- Projectile : trait gris-coloré non émissif, sans traînée ni halo ; taille visuelle fixe.
- Pas de réticule, de douille, de sprite de flash, de traînée de dash.
- Sons absents : dash, chute, mutation, armure, onde, hologramme, coup reçu, lame, shotgun distinct (le Shotgun et la Rafale jouent `SFX_Jira_Tir`).
- Anims non branchées : dash, mort, coups reçus, mêlée, charge (MANQUANTS §3 liste aussi : chute, Griffe, onde, sortie de cuve, cut-in illustré).
- Corps d'armure/mutation non fait.
- MANQUANTS §3 et §7 décrivent encore Jira et les ennemis en « sprite billboard » : c'est périmé, le code utilise `SK_Jira_UEFN` et les modèles du studio.

---

## Nuke (type interne `goule`)

**Rôle** : mêlée en meute (« nuée »), arrive par 3 ou 4.

### Stats (`DT_Ennemis`, ligne `goule`)

| PV | R (déplacement) | Vitesse | Masse | Contact | Vole | Hitbox HH × HR (proto) | ADN | Coût vague |
|---|---|---|---|---|---|---|---|---|
| 3 | 5 px | 42 px/s (× 0,55 = 1,4 m/s) | 1,3 | oui | non | 16 × 5 → remis à l'échelle 6,4 × 2 px (modèle 40 cm) | 1 | 3,2 (meute) |

### Comportement (`EONEnemy.cpp:461-506`)

- Compte les Nuke vivantes (`Total`) et celles à < 70 px (`Grp`). Besoin = min(3, Total).
- **Seule** depuis < 3,5 s : recule si Jira à < 120 px (× 1,1).
- **Pas assez groupée** : rejoint la Nuke la plus proche à × 1,1 pendant 3 s au plus.
- **En meute** : suit le champ vers Jira + 35 % de vitesse latérale (côté `Sd` tiré au hasard) ; seule (enragée) × 1,3.
- Se sépare des autres Nuke à < 10 px.
- Dégât : contact, 1 cœur à < R + 5 px.
- Meute : 3 + 45 % de chances d'une 4e, dispersées de ±10 px (`EONCombatDirector.cpp:351-361`).

### Éléments visibles

| Élément | Nom / chemin | Détail | Réglage |
|---|---|---|---|
| Maillage | `/Game/EON/Ennemis/Nuke/SK_Nuke` (squelette `SKEL_Nuke`, studio « Ghoul ») | hauteur imposée **40 cm** (`ModelHCm`, `EONEnemy.cpp:62-63`) | code (table `Looks`) |
| Matériau | `MI_Nuke` (`M_EON_ToonEnnemi`), texture `T_Nuke_BaseColor` | | éditeur |
| Marche | `A_Nuke_Walk`, vitesse de lecture = vitesse / Spd, bornée 0,6–2 ; à l'arrêt : marche à 0,25 (`EONEnemy.cpp:301-304`) | | code |
| Attaque | `A_Nuke_Attack` × 1,5, quand Jira est à < R + 8 px (`:292`) | | `AttackRate` dans `Looks` |
| Morts | `A_Nuke_Death_01/02/03` au hasard | | `Looks` |
| Impact / mort | couleur `#e8a24a` (ocre) | | code |
| Son | **aucun** | | |
| Nom / barre | barre debug si blessée ; nom non affiché (pas d'élite Nuke) | | |

### Manquant / provisoire

`A_Nuke_Death_V0` sert à la petite Nuke ; 4e mort annoncée par MANQUANTS §1 (« death ×4 ») : seules 3 + V0 importées. Pas de son d'attaque.

---

## Petite Nuke (type interne `gouleP`) — pondue par Ubume

**Rôle** : sort d'un œuf d'Ubume et fonce sur Jira.

| PV | R | Vitesse | Masse | Contact | Hitbox (proto → modèle) | ADN | Coût |
|---|---|---|---|---|---|---|---|
| 3 | 5 | 48 px/s | 1 | oui | 10 × 5 → 4 × 2 px (modèle 25 cm) | **0** | — (jamais dans les vagues) |

- Comportement : suit le champ vers Jira à pleine vitesse, se sépare de **tous** les ennemis (`EONEnemy.cpp:507-512`).
- Naissance : à l'éclosion d'un œuf, apparition en 0,05 s (`EONCombatDirector.cpp:659-666`).

| Élément | Nom | Détail |
|---|---|---|
| Maillage | `SK_Nuke` (même modèle), hauteur **25 cm** (`EONEnemy.cpp:64-65`) | aucune teinte propre |
| Anims | `A_Nuke_Walk`, `A_Nuke_Attack_V0` × 1,5, morts `A_Nuke_Death_V0` / `A_Nuke_Death_01` | |
| Impact / mort | vert `#6fbf4a` | |
| Son | aucun | |

Manquant : pas de modèle « plus petit » dédié (MANQUANTS §1 : Ghoul plus petite) ; teinte verte seulement sur les éclats.

---

## Ikiryo (type interne `sniper`)

**Rôle** : tireur à distance, un seul gros tir rapide après une longue visée.

| PV | R | Vitesse | Masse | Contact | Hitbox | ADN | Coût | Dispo |
|---|---|---|---|---|---|---|---|---|
| 4 | 6 | 34 px/s | 1,3 | non | 22 × 5 → 28,8 × 6,5 px (modèle 180 cm) | 2 | 1,9 | combatN ≥ 2 |

### Comportement (`EONEnemy.cpp:581-617`)

| État | Durée | Ce qu'il fait |
|---|---|---|
| Move | — | recule sous 170 px, avance au-delà de 280 px, se décale (45 % latéral, change de côté au hasard) ; passe en visée si recharge finie et Jira à > 110 px |
| Aim | 1,1 s | suit Jira ; **ligne de visée debug rouge** (255,61,85) épaisseur 1, de 14 à 240 px, à 13 px de haut (`:751-755`) |
| Fire | 0,12 s | ligne devient **blanche** épaisseur 3, puis tire |
| Rec | 0,5 s | recule si Jira < 150 px |
| Recharge | 2,6 à 3,5 s | (initiale 0,6 s) |

- Le tir : `FireEnemyBullet` depuis 12 px devant lui, hauteur 13 px, 170 px/s, vie 3 s, **gros** (orange `#e8923a`, 37,5 cm) ; secousse 0,14 ; son `SFX_Ennemi_Tir` vol 0,5 (voir « Tirs ennemis »).
- Peut être poussé dans le vide.

| Élément | Nom | Détail |
|---|---|---|
| Maillage | `/Game/EON/Ennemis/Ikiryo/SK_Ikiryo` (`SKEL_Ikiryo`, studio « Shooter ») | 180 cm (`EONEnemy.cpp:66`) |
| Matériau | `MI_Ikiryo`, `T_Ikiryo_BaseColor` (+ `T_Ikiryo_Displacement`) | UV recalées (MANQUANTS l. 42-45) |
| Anims | **aucune** : modèle figé (le code fait varier un play rate sans anim) | |
| Impact / mort | `#e8923a` | |
| Cadavre | aucun (disparaît) | |

Manquant : toutes les anims (marche, recul, visée, tir, mort — MANQUANTS §2) ; ligne de visée en debug ; pas de flash de bouche ni de son de charge.

---

## Onibi (type interne `kamikaze`)

**Rôle** : fonce et explose.

| PV | R | Vitesse | Masse | Contact | Hitbox | ADN | Coût |
|---|---|---|---|---|---|---|---|
| 2 | 6 | 68 px/s (le plus rapide) | 0,8 | non | 20 × 7 → 24 × 8,4 px (modèle 150 cm) | 1 | 1,1 |

### Comportement (`EONEnemy.cpp:513-534`)

- **Move** : suit le champ en ondulant, vitesse × (1 + 0,3 × sin(8t)).
- À < 18 px de Jira : **Arm** 0,45 s. Pendant l'armement : clignote blanc à 10 Hz (`:743`), tremble ±1 px (`:712`), **cercle debug orange** (255,154,61) de rayon 10 ± 2 px au sol (`:772-775`), anim `A_Onibi_Explosion` lancée.
- **Explose** : meurt **sans crédit** (pas d'ADN ni butin), `Aoe(rayon 28 px, 3 dégâts, blesse Jira, #ff7a1a)` (`:527-531`).
- **Tué avant** : explose quand même, plus petit : rayon 26 px, 2 dégâts, ne blesse pas Jira, couleur `#ff9a3d` (`EONCombatDirector.cpp:425`).

### L'explosion (`AEONCombatDirector::Aoe`, `EONCombatDirector.cpp:442-465`)

| Élément | Valeur | Réglage |
|---|---|---|
| Dégâts aux ennemis | tous ceux à < rayon + `Ehr`, recul × 3 (réaction en chaîne possible entre Onibi) | code |
| Dégât à Jira | 1 cœur si à < 80 % du rayon (22,4 px = 1,4 m) | `EnemyDmg` |
| Éclats | 34, 1 sur 3 blanc, 180 px/s, 0,5 s, taille 1,25 px, frein 5 (Niagara Gerbe) | code |
| Anneau | `NS_EON_Anneau`, diamètre = 2 × rayon, 0,25 s, couleur de l'explosion | code |
| Secousse / hitstop | 0,45 / `Hitstop × 0,6` | `Shake`, `Hitstop` |
| Son | **aucun** (`SFX_Proto_explode` existe mais n'est pas joué ici) | — |
| Lumière | aucune | — |

| Élément | Nom | Détail |
|---|---|---|
| Maillage | `/Game/EON/Ennemis/Onibi/SK_Onibi` (`SKEL_Onibi`, studio « Kamikaze ») | 150 cm |
| Matériau | `MI_Onibi`, `T_Onibi_BaseColor` | |
| Course | `A_Onibi_Run` ; à l'arrêt `A_Onibi_Walk` | |
| Armement / explosion | `A_Onibi_Explosion` × 1, continue sur le cadavre (`EONEnemy.cpp:289, 312-318`) | |
| Morts (tué) | `A_Onibi_Death_01/02` | |
| Non utilisées | `A_Onibi_Attack`, `A_Onibi_Run_Anticipation`, `A_Onibi_Run_Variation` | |
| Impact / mort | `#ff7a1a` | |

Manquant : signal sonore d'armement (le doc 03 en parle, le code n'en joue pas) ; son d'explosion ; cercle d'armement en debug ; la perte de 6 PV au leurre du proto n'est pas portée (MANQUANTS l. 190).

---

## Kama (type interne `slasher`)

**Rôle** : élan puis charge-lame en ligne droite.

| PV | R | Vitesse | Masse | Contact | Hitbox | ADN | Coût | Dispo |
|---|---|---|---|---|---|---|---|---|
| 6 | 6 | 38 px/s | 1,7 | oui | 22 × 5 → 28,8 × 6,5 px (180 cm) | 2 | 2,1 | combatN ≥ 3 |

### Comportement (`EONEnemy.cpp:553-580`)

| État | Durée | Ce qu'il fait | Visuel |
|---|---|---|---|
| Move | — | marche vers Jira ; à < 85 px et recharge finie → Wind | `A_Kama_Run` |
| Wind (élan) | 0,55 s | fige sa direction | tremble ±1 px ; **ligne debug rouge** épaisseur 1,5 sur 70 px à 6 px de haut (`:768-771`) ; `A_Kama_Attack` lancée (`:290`) |
| Dash | 0,32 s | 235 px/s tout droit (non affecté par `SpeedMul3D`) | traînée : points rouges `#e0405a` immobiles 2 px, 0,2 s, ~48 /s (`:572`) |
| Rec | 0,6 s (0,7 s s'il heurte un mur + secousse 0,12) | | |
| Recharge | 1,2 s (initiale 0,6 s) | | |

Dégât : **contact** (à tout moment, pas seulement pendant la charge), 1 cœur. Le dash peut l'envoyer dans le vide.

| Élément | Nom | Détail |
|---|---|---|
| Maillage | `/Game/EON/Ennemis/Kama/SK_Kama` (`SKEL_Kama`, studio « Slasher ») | 180 cm |
| Matériau | `MI_Kama`, `T_Kama_BaseColor`, `T_Kama_Height` | boucles et mâchoire sans UV (rayées, MANQUANTS l. 41) |
| Anims | `A_Kama_Run`, `A_Kama_Attack` (× 1), `A_Kama_Death` ; `A_Kama_Taunt` inutilisée | |
| Impact / mort | `#e0405a` | |
| Élite | possible (« Kama élite ») | |

### Variantes basées sur le Kama

- **La chose** (event Irruption) : Kama élite + `MakeStalker` (`EONFloorManager.cpp:2251-2252`, `EONEnemy.cpp:798-855`) : 1 PV, R 6, nom « La chose ». Rôde autour de Jira (ellipse 110 × 80 px, ≤ 70 px/s) 2,5–4 s, s'arrête 0,65 s avec `SFX_Proto_roar` (vol 0,9), charge 240 px/s 0,75 s (traînée lie-de-vin `#3a1020`), recommence. Contact hors rôde = PV + 5 (tue). Yeux : lumière ponctuelle « YeuxChose » rouge (255,61,85) (`EONFloorManager.cpp:275-280`). Visuel : modèle du Kama élite, pas noirci (MANQUANTS l. 65 : silhouette noire à faire).
- **Membre d'équipage muté / Ex-quartier-maître / Survivant muté** : Kama ou Brainless élite, 20 PV × (1 + 0,3 × (étage − 1)), masse × 2,2 (`EONFloorManager.cpp:2570-2575`).

Manquant : pas de son d'élan (le doc 03 cite un signal sonore) ; télégraphe en debug.

---

## Ubume (type interne `invocateur`)

**Rôle** : reste à distance et pond des œufs qui éclosent en petites Nuke.

| PV | R | Vitesse | Masse | Contact | Hitbox | ADN | Coût | Dispo |
|---|---|---|---|---|---|---|---|---|
| 7 | 6 | 34 px/s | 1,5 | non | 22 × 6 → 32 × 8,7 px (200 cm) | 3 | 2,6 | combatN ≥ 3 |

### Comportement (`EONEnemy.cpp:655-677`)

- **Move** : recule sous 120 px, avance au-delà de 200 px, décalage latéral 50 %.
- Si recharge finie et **moins de 3 œufs** en jeu : **Lay** 0,75 s (cercle debug jaune-vert (216,240,106) de 8 px au sol, `:776-779` ; anim `A_Ubume_Attack` × 2).
- Ponte : un œuf part de 10 px de haut vers Jira (±0,3 rad), à 45–100 px. Recharge 3,4–4,6 s.

### L'œuf (`AEONEnemyEgg`, `EONEnemyProjectile.cpp:86-133`, `.h:43-66`)

| Élément | Valeur | Réglage |
|---|---|---|
| Maillage | `/Engine/BasicShapes/Sphere`, rayon 3 px, ovale × 1,3 en hauteur (≈ 37,5 × 49 cm) | code |
| Matériau | `M_Flat`, `Color` = vert pâle `#d8f06a` × 2, non émissif, sans trait | code |
| Vol | en cloche, 0,55 s (`FlyTime`), sommet +14 px | code |
| Au sol | tremble de plus en plus, éclot à 2 s (`HatchTime`) | code |
| Éclosion | 16 éclats vert `#8aa82a` / jaune-vert `#d8f06a`, 160 px/s, 0,35 s ; une petite Nuke apparaît ; rien si l'œuf tombe dans le vide (`EONCombatDirector.cpp:645-671`) | code |
| Son | aucun | |

| Élément | Nom | Détail |
|---|---|---|
| Maillage | `/Game/EON/Ennemis/Ubume/SK_Ubume` (`SKEL_Ubume`, studio « Invocateur ») | 200 cm (le plus grand) |
| Matériaux | `MI_Ubume_Body`, `MI_Ubume_Glasses`, `MI_Ubume_Tentacles` (+ textures BaseColor/Height de chaque partie) | |
| Anims | `A_Ubume_Walk`, `A_Ubume_Idle` (arrêt), `A_Ubume_Attack` × 2 (ponte) | |
| Mort | **pas d'anim** : disparaît | |
| Impact | `#8a6aaa` ; mort : **blanc** (absent de la table de `KillEnemy`) | |
| Élite | possible | |

Manquant : anims de ponte dédiée et de mort (MANQUANTS §2) ; modèle d'œuf (sphère, MANQUANTS l. 67) ; télégraphe de ponte en debug.

---

## Brainless (type interne `brainless`)

**Rôle** : verrouille, charge tête baissée ; sonné contre un mur.

| PV | R | Vitesse | Masse | Contact | Hitbox | ADN | Coût | Dispo |
|---|---|---|---|---|---|---|---|---|
| 8 | 7 | 30 px/s | 2,6 (le plus lourd) | non | 22 × 6 px (pas de hauteur imposée : 137,5 cm) | 2 | 1,8 | combatN ≥ 2 |

### Comportement (`EONEnemy.cpp:618-654, 689-695`)

| État | Durée | Ce qu'il fait | Visuel |
|---|---|---|---|
| Move | — | vers Jira ; à < 150 px et recharge finie → Lock | marche |
| Lock | 0,65 s | fige la direction, recule à 18 px/s × multiplicateurs | **ligne debug rouge clignotante** épaisseur 2, de 10 à 90 px, à 7 px de haut (`:756-759`) |
| Chg (charge) | 1,7 s max | 235 px/s tout droit | poussière beige `#b0a488`, ~36 grains/s, ±35 px/s, 0,25 s (`:640`) |
| Touche Jira | — | 1 cœur + poussée 170 px/s ; sonné 0,9 s ; secousse 0,45 (`:641-646`) | |
| Percute un mur | — | **étourdi 1,6 s**, secousse 0,3 | 12 éclats crème `#e8e0c8`/blanc, 110 px/s, 0,35 s (`:690-695`) ; **3 points ambre debug** qui tournent au-dessus de la tête (`:760-767`) |
| Recharge | 2 s | | |

Pendant la charge il peut tomber dans le vide.

| Élément | Nom | Détail |
|---|---|---|
| Maillage | **placeholder Unity** `/Game/EON/Unity/import/fbx/chara/Brain_Crawler` | hauteur = HH 22 px (137,5 cm) |
| Anim | `Brain_CrawlerArmature_Armature_Armature_Armature_Unreal_Take_baselayer` (boucle unique, au rythme de la vitesse ; 0,25 à l'arrêt) | |
| Matériau | celui du FBX Unity | |
| Impact | `#c86a6a` ; mort : **blanc** (absent de `KillEnemy`) | |
| Mort | pas d'anim : disparaît | |
| Élite | possible | |

Variantes : **Dormeur des pieces** (Trésor piégé, 18 PV × (1 + 0,3 × (étage − 1)), masse × 2, `EONFloorManager.cpp:1422-1427`) ; **La gamine** (mutée, 22 PV × idem, `:2542-2547`) ; équipage muté (voir Kama).

Manquant : tout le personnage (design, modèle, rig, anims marche/verrouillage/charge/étourdi/mort — MANQUANTS §2) ; télégraphes en debug ; aucun son.

---

## Méduse (type interne `meduse`)

**Rôle** : volante, tourne autour de Jira, salve de 3 tirs en éventail.

| PV | R | Vitesse | Masse | Contact | Vole | Hitbox | ADN | Coût | Dispo |
|---|---|---|---|---|---|---|---|---|---|
| 4 | 6 | 36 px/s | 1,3 | non | **oui** (ne tombe jamais, ignore le vide) | 18 × 6 px (112,5 cm) | 2 | 1,7 | combatN ≥ 2 |

### Comportement (`EONEnemy.cpp:535-552`)

- Reste entre 70 et 115 px de Jira, tourne autour (60 % latéral, change de sens au hasard ~0,4 fois/s).
- Toutes les 1,7 à 2,4 s (premier tir après 1 à 2,2 s) : **3 tirs** en éventail (écart 0,22 rad), départ à 9 px de haut, 82 px/s, vie 3 s, petits magenta `#ff3d8b` (voir « Tirs ennemis »).
- **Flottement** : décalage du modèle `HoverPx` 6 px (37,5 cm) + oscillation `5 − round(sin 4t)` px (4 à 6 px, soit 25–37,5 cm) (`EONEnemy.cpp:74, 221, 710`).

| Élément | Nom | Détail |
|---|---|---|
| Maillage | **placeholder Unity** `/Game/EON/Unity/import/fbx/xeno/Meshy_AI_Unyielding_Chimera_0111100029_generate` | « Chimera Meshy », hauteur 18 px |
| Anims | aucune | |
| Impact / mort | violet `#8f7be0` | |
| Mort | pas de cadavre | |
| Élite | jamais | |

Manquant : tout (design, modèle, rig, anims flottement/tir/mort — MANQUANTS §2) ; nom provisoire « Meduse (hors doc) ».

---

## Récapitulatif des tailles en jeu

| Personnage | Hauteur du modèle | Source |
|---|---|---|
| Jira | 181 cm | `EONCharacter.cpp:81` |
| Nuke | 40 cm | `ModelHCm`, `EONEnemy.cpp:63` |
| Petite Nuke | 25 cm | `:65` |
| Ikiryo | 180 cm | `:66` |
| Kama | 180 cm | `:69` |
| Onibi | 150 cm | `:71` |
| Ubume | 200 cm | `:73` |
| Brainless | 137,5 cm (HH 22 px) | `:67` |
| Méduse | 112,5 cm (HH 18 px) + flottement | `:74` |
| Élite | × 1,45 | `EONEnemy.cpp:157` |

Ces hauteurs se changent dans le dernier champ (`ModelHCm`) de la table `Looks()` (`EONEnemy.cpp:59-79`) : la hitbox suit.

## Récapitulatif « ce qui est en debug » (à remplacer pour une build Shipping)

Chiffres de dégâts ; barres de vie et noms d'élite ; marqueurs d'apparition ; télégraphes (visée Ikiryo, élan Kama, verrouillage Brainless, étourdissement, armement Onibi, ponte Ubume) ; rayons Laser/Rayon ; arcs Lame/Fauche ; fouet Griffe ; hologramme ; frappes de l'ultime ; écran « Signal perdu » (bilan).
