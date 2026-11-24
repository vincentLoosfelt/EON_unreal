# E.O.N — Tous les effets spéciaux (relevé du code au 24/11/2026, mis à jour le soir)

Projet lu : `D:/_PROJETS/briscard/EON_UE`. Les chemins `fichier:ligne` partent de `Source/EON/Private/`, sauf mention contraire.

**Lecture des chiffres.** 1 px du proto = 6,25 cm. Les vitesses d'éclats sont des maximums : le système Niagara tire au hasard entre 25 et 100 % de cette valeur. Les éclats sont des carrés sans gravité, freinés. Ils partent en demi-sphère vers le haut. Taille par défaut : 1 px, frein 4.

**Les trois systèmes Niagara** (`EONFx.cpp`, construits par `Saved/Scripts/eon_vfx_build.py`) :
- `NS_EON_Gerbe` : éclats dans toutes les directions.
- `NS_EON_GerbeDir` : cône de ±63° autour d'une direction, relevé de 0,35 vers le haut.
- `NS_EON_Anneau` : cercle à plat au sol, qui grandit de 0 au rayon donné en s'effaçant.

`Gerbe2` et `GerbeDir2` font deux couleurs : un éclat sur N prend la 2e couleur. La console `eon.fx anneau|gerbe|dir` lance un effet de test (`EONFx.cpp:128-142`). Ce test n'est pas compté dans les totaux. Depuis le 24/11 : `eon.fx eclair|lumiere|flaque|organique|cercle|cone|ligne` en plus.

**États**
- **OK** : réel et visible en jeu, même s'il s'agit d'un placeholder.
- **DEBUG** : dessiné avec `DrawDebug*` ou `GEngine->AddOnScreenDebugMessage`, donc invisible dans une build Shipping.
- **PRÊT** : codé et testable par la console `eon.fx`, mais pas encore branché sur un déclencheur du jeu (son propriétaire n’est pas porté).
- **ABS** : prévu mais pas fait, aucun code ne le produit. Le déclencheur commence par « Prévu : » et la dernière colonne, « Demandé par : », nomme le document qui le demande au lieu d’un endroit dans le code. Sources : la passation de Sabri du 13/11 (`Docs/PASSATION_UNREAL`, proto pixel v0.60.1), `MANQUANTS.md`, des notes « pas encore » dans le code, et deux propositions du relevé des fiches. Le projet n’a pas de GDD.

**Codes.** La colonne « code » reprend les codes de `Docs/Fiches_Persos/_src/fiches_data.py`. Les effets qui n’avaient pas de code en ont reçu un ici (ex. JIR-V13, COM-V8, préfixes GAR, DEC, INT). Les sous-parties d’un même effet sont numérotées .1, .2… (ex. JIR-V10.3, le cut-in de l’ultime).

**Arrêt sur image.** L'hitstop fige tout le jeu, éclats compris. Le proto, lui, laissait tourner ses effets à 15 %.

**Changé en version 5 (24/11 au soir, demandes de Vincent).** JIR-T1 balle de Jira à 20 % et émissive. COM-V10 balles ennemies à 10 % et émissives, traînées 3 fois plus longues (aussi IKI-T1, GAR-V9, Méduse). JIR-V14 douille en celluloïd. COM-V9 trous à 45 % de leur taille, avec du sang qui gicle dans le sens du tir.

---

## 1. Jira

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| JIR-T1 | Balle | Sphère moteur étirée en trait de 10 × 2 × 2 cm (20 % de l'ancien 50 × 10 × 10, Vincent 24/11). Matériau émissif `M_EON_Glow` (intensité 8, lumineux sans éclairage), couleur de la greffe : Standard #3ef2e0, Rafale #ffc23d, Shotgun #ff9a3d. Pas d'ombre ni de trait. Taille visuelle fixe : « Balles épaisses » ne grossit que la collision. 15,6 m/s en Standard. | Chaque tir : auto ; rafale chargée de 2 à 8 balles, une toutes les 0,045 s ; éventail de 7 plombs sur 40°. | EONProjectile.cpp:37-38, 80-82 ; EONCharacter.cpp:726-758 | OK |
| JIR-V13 | Traînée de la balle | Aucune traînée ni halo derrière la balle. | Derrière chaque balle de Jira, pendant tout son vol. | **Codé le 24/11 :** EONProjectile.cpp:90 et :104 (traînée Glow Trainee + halo Rond, M_EON_Glow) ; **Demandé par :** Passation Sabri 13/11, 07_ASSETS_3D.md §5 (« Traînée du projectile… lecture du tir ») | OK |
| JIR-V1 | Flash de bouche | Lumière ponctuelle couleur de la greffe, au bout du bras + 20 cm. Intensité 6000, rayon 66 px (4,1 m), sans ombre, 0,06 s. | Chaque tir (réglage F6 « Flash »). | EONCharacter.cpp:84-90, 583-588, 769-775 | OK |
| JIR-V2 | Sprite de flash | Image de flamme au canon. | À chaque tir, au bout du canon, avec la lumière de JIR-V1 (0,06 s). | **Codé le 24/11 :** EONCharacter.cpp:800 → EONFx::Eclair (halo, trait blanc, cœur, 0,06 s) ; **Demandé par :** MANQUANTS.md §7 (« le sprite du flash du proto manque ») ; Passation Sabri 13/11, 07_ASSETS_3D.md §5 | OK |
| JIR-V14 | Douille éjectée | Douille ambre #ffc23d en celluloïd (Vincent 24/11) : même matériau toon que le décor (`MI_BG_pipe`, teinte BC1) et trait du post-process. Part sur le côté et rebondit au sol. | À chaque tir, éjectée sur le côté, rebondit au sol. | **Codé le 24/11 :** EONCharacter.cpp:811 → EONFx::Douille (EONFx.cpp:173-190, celluloïd) ; rebond EONFxPiece.cpp:69 (260 px/s², ×0,4, 4 s au sol, 80 max) ; **Demandé par :** Passation Sabri 13/11, 02_JOUEUR_ET_COMBAT.md:86 (mise en scène du tir) et 07_ASSETS_3D.md §5 | OK |
| JIR-U2 | Réticule qui s'écarte | Viseur qui s'ouvre à chaque tir. | À chaque tir, le réticule s'ouvre puis se referme. | **Codé le 24/11 :** EONCharacter.cpp:803 (CrossT +0,35, −4/s) ; dessin EONHUD.cpp:109 (4 traits, rouge en mutation) ; **Demandé par :** Passation Sabri 13/11, 02_JOUEUR_ET_COMBAT.md:86 ; noté « pas encore » dans EONCharacter.cpp:776 | OK |
| JIR-V15 | Kick caméra au tir | La caméra recule de kick × 1,6 px (× CamKick) à l'opposé du tir, amortie en dt × 14. Secousse de 0,04 × kick (kick : Standard 1, Shotgun 3,6). Jira recule de kick × 14 px/s. | Chaque tir. | EONCharacter.cpp:760-767 ; EONPlayerCameraManager.cpp:23-28, 103-107 | OK |
| JIR-V16 | Arrêt sur image du Shotgun | Hitstop × 0,15 (≈ 8 ms). | Chaque tir de Shotgun. | EONCharacter.cpp:1100 | OK |
| JIR-V3 | Éclats de mur | 4 éclats couleur du tir en cône vers l'arrière, à 0,2 × vitesse + 35 px/s, 0,2 s. | Balle arrêtée par un mur. | EONProjectile.cpp:118, 127-131, 149 | OK |
| JIR-V17 | Point de ricochet | 1 point blanc immobile de 1 px, 0,15 s. | Balle qui ricoche (objet Ricochet). | EONProjectile.cpp:110-111 | OK |
| JIR-T3 | Rayon (Laser ; aussi Rayon du Cobaye) | Trait `DrawDebugLine` couleur de la greffe (Laser #ff6fd8, Rayon #ffa040), du bras jusqu'au mur. Épaisseur 2 + 1,5 × Size + 2 × charge px, 0,14 s. | Relâcher le tir chargé à plus de 20 %. | EONCharacter.cpp:1167-1170 | DEBUG |
| JIR-V18 | Bout du rayon | 10 éclats couleur de la greffe, toutes directions, 85 px/s, 0,3 s. | Même déclencheur. | EONCharacter.cpp:1171-1172 | OK |
| JIR-V19 | Secousse du rayon | Secousse 0,15 × charge, plus le kick du tir (× 0,5 à 1). Charge au-delà de 95 % : hitstop × 0,5. | Même déclencheur. | EONCharacter.cpp:1173-1176 | OK |
| JIR-T4 | Arc de lame (Lame ; aussi Fauche) | Arc de 8 segments `DrawDebugLine`, couleur de la greffe (Lame #b8f07a, Fauche #ff6a5a). Épaisseur 6, rayon R + 4 px, à hauteur de hanche, 0,12 s. | Coup de Lame. | EONCharacter.cpp:1211-1218 | DEBUG |
| JIR-V20 | Poussée de la lame | Caméra poussée de 2 px vers l'avant ; Jira avance de 40 px/s. | Coup de Lame. | EONCharacter.cpp:1206-1210 | OK |
| JIR-V21 | Fouet (greffe Griffe, du Cobaye) | Ligne `DrawDebugLine` #e0503a, épaisseur 8, 0,18 s, jusqu'au mur. | Tir de Griffe (hors du cycle de Jira : `JiraOrder` 0). | EONCharacter.cpp:1241-1242 | DEBUG |
| JIR-V22 | Bout du fouet | 6 éclats, moitié couleur de la greffe, moitié lilas #d8d4f0, 64 px/s, 0,25 s. Hitstop × 0,35 si le fouet touche. | Tir de Griffe. | EONCharacter.cpp:1243-1247 | OK |
| JIR-V4 | Poussière de dash | 8 grains gris-violet #8a84b8 en cône vers l'arrière, 80 px/s, 0,3 s. Secousse 0,06. | Appui sur dash. | EONCharacter.cpp:629-631 | OK |
| JIR-V6 | Traînée de fantômes du dash | Silhouettes fantômes derrière Jira. | Pendant chaque dash (0,15 s), silhouettes laissées derrière Jira. | **Codé le 24/11 :** EONCharacter.cpp:549 (dash, 90 %/image, 0,18 s) et :516 (course, 0,12 s) → EONFx::Fantome ; **Demandé par :** Passation Sabri 13/11, 02_JOUEUR_ET_COMBAT.md:52 et 07_ASSETS_3D.md §5 ; MANQUANTS.md §7 ; noté « pas encore » dans EONCharacter.cpp:630 | OK |
| JIR-V5 | Poussière de course | 1 grain #8a84b8 toutes les 0,07 s, 15 cm derrière les pieds, 20 px/s, 0,35 s, sans frein. | Course (Maj, ou dash maintenu). | EONCharacter.cpp:507-514 | OK |
| JIR-V7 | Onde courte (E) | Anneau magenta #ff3d8b au sol, de 0 à 56 px de rayon (7 m de diamètre), 0,3 s. 24 éclats en couronne, magenta et blancs (1 sur 2), 180 px/s, 0,25 s, frein 6, départ ±8 px. Secousse 0,25 ; hitstop × 0,5. | Appui court sur E. | EONCharacter.cpp:1268-1276, 1309 | OK |
| JIR-V8 | Onde continue (E maintenu) | Anneau magenta de 44 px, 0,18 s, une pulsation toutes les 0,22 s, sans éclats. | E maintenu plus de 0,25 s. | EONCharacter.cpp:1269, 1319 | OK |
| JIR-V9.1 | Hologramme : apparition | 16 éclats cyan #3ef2e0, 42 px/s, 0,4 s, frein 3. | Touche F. | EONCharacter.cpp:1287-1288 | OK |
| JIR-V9.2 | Hologramme : corps | `DrawDebugCapsule` cyan, rayon 6 px, de la hauteur de Jira. Clignote à 10 Hz (allumée 70 % du temps), 5 s. Le leurre est donc invisible en Shipping. | Leurre actif. | EONCharacter.cpp:1329-1343 | DEBUG |
| JIR-V9.3 | Hologramme : fin | 22 éclats cyan et blancs, 64 px/s, 0,4 s. | Fin des 5 s, ou leurre détruit (8 PV). | EONCharacter.cpp:1336-1338 | OK |
| JIR-V10.1 | Ultime : voile et QTE | Voile sombre #07060f à 60 % sur tout l'écran. 5 cases de flèches qui se colorent quand elles sont réussies. Chrono cyan, rouge sous 30 %. Jeu figé (hitstop de 120 ms renouvelé), secousse 0,1 au départ. | R, jauge pleine. | EONHUD.cpp:387-409 ; EONCharacter.cpp:1374, 1380 | OK |
| JIR-V10.2 | Ultime : échec | Secousse 0,3 ; bannière rouge « Sequence ratee » ou « Trop lent ». | Mauvaise flèche, ou 3,2 s écoulées. | EONCharacter.cpp:1398-1408 | OK |
| JIR-V10.3 | Ultime : cut-in | Bande sombre à 95 %, deux filets cyan, titre blanc « Protocole thoracique » qui glisse, sous-titre cyan, 1,1 s. Hitstop de 1 s. | QTE réussi. | EONHUD.cpp:410-424 ; EONCharacter.cpp:1412-1418 | OK |
| JIR-V23 | Illustration du cut-in | Dessin de Jira dans la bande, aujourd'hui remplacé par une bande de couleur. | Pendant le cut-in de l'ultime (JIR-V10.3). | **Codé le 24/11 :** EONHUD.cpp:448 ; texture /Game/EON/UI/T_Jira_CutIn (rendu provisoire, eon_cutin_jira.py + eon_cutin_png.py) ; **Demandé par :** MANQUANTS.md §3 (illustration à produire ; aujourd'hui bande de couleur) | OK |
| JIR-V10.4 | Ultime : rayon du ciel | Ligne `DrawDebugLine` cyan, épaisseur 40, de 30 m au-dessus de l'ennemi jusqu'à lui, 0,25 s. | Une frappe par ennemi, à 0,09 s d'écart. | EONCharacter.cpp:1355-1356 | DEBUG |
| JIR-V10.5 | Ultime : impact | 26 éclats cyan et blancs (1 sur 3), 190 px/s, 0,5 s, 1,25 px. Secousse 0,35. | Chaque frappe. | EONCharacter.cpp:1357-1361 | OK |
| JIR-V11 | Flash rouge de coup reçu | Voile rouge plein écran (1 ; 0,05 ; 0,1), alpha de 0,3 à 0 en 0,25 s. Secousse 0,55 ; hitstop de 55 × 1,2 + 30 ms ; recul de 120 px/s. | Coup reçu. Aussi à la chute : 0,25 s et secousse 0,4. | EONCharacter.cpp:239-241, 377-380 ; EONHUD.cpp:91-92 | OK |
| JIR-V12 | Voiles de mutation | Stade 1 : voile rouge sombre qui pulse (alpha ≈ 0,09, plus vite à chaque stade), 2 bandes claires qui défilent. Stade 2 : en plus, aberration (voiles rouge et cyan décalés de ±2 px). Stade 3 : 7 bandes de 3 px et 160 points de neige blancs. Secousse 0,35 à chaque montée de stade. | Énergie à 0, puis montée de stade. | EONHUD.cpp:68-89 ; EONCharacter.cpp:710, 1030 | OK |
| JIR-V24 | Veines de mutation sur le corps | Veines sur Jira, aujourd'hui remplacées par la teinte rouge du HUD. | Quand la mutation de Jira monte (stades 1 à 3). | **Codé le 24/11 :** EONCharacter.cpp:593 (stades 0 / 0,45 / 0,75 / 1) ; motif + lueur dans M_EON_ToonMasked (eon_fx_abs_mat.py, eon_fx_abs_veines2.py) ; **Demandé par :** MANQUANTS.md §3 (veines à produire ; aujourd'hui teinte rouge du HUD) | OK |
| JIR-V25 | Bouclier qui encaisse | 20 éclats cyan #3ef2e0, 90 px/s, 0,35 s. Secousse 0,25. | Coup absorbé par le bouclier. | EONCharacter.cpp:219-222 | OK |
| JIR-V26 | Textes « Bouclier » et « Bloque » | Texte flottant `DrawDebugString`, cyan ou rose (224,90,138). | Bouclier, blocage d'armure. | EONCharacter.cpp:220, 230 → EONCombatDirector.cpp:699-703 | DEBUG |
| JIR-V27 | Bouclier NV3 nettoyant | Anneau cyan de 80 px (10 m), 0,35 s. Secousse 0,4. | Coup absorbé, cassette Shield NV3, branche A. | EONCombatDirector.cpp:839-857 | OK |
| JIR-V28 | Bouclier NV3 miroir | Anneau blanc-bleu #e8f6ff de 46 px, 0,25 s. Les tirs proches repartent en balles cyan (JIR-T1, #3ef2e0). Bannière. | Branche B. | EONCombatDirector.cpp:858-879 | OK |
| JIR-V29 | Blocage d'armure | 12 éclats rose #e05a8a et blancs, 80 px/s, 0,3 s. Secousse 0,15. | Blocage (chance ArmBlock). | EONCharacter.cpp:229-232 | OK |
| JIR-V30 | Armure qui pousse | 34 éclats prune #a03a64 et rose #e05a8a (1 sur 3), 130 px/s, 0,5 s, 2 px, départ ±6 px. Secousse 0,3 ; flash rouge de 0,08 s. | Montée d'un niveau d'armure. | EONCharacter.cpp:1066-1072 | OK |
| JIR-V31 | Armure qui régresse | 16 grains sombres #3a3850, 60 px/s, 0,5 s. | Descente d'un niveau d'armure. | EONCharacter.cpp:1068 | OK |
| JIR-V32 | Retour au bord | 14 éclats cyan, 50 px/s, 0,4 s. | Jira traverse la kill zone sous un trou. | EONCharacter.cpp:373-374 | OK |
| JIR-V33 | Objet pris | 30 éclats couleur de la rareté, 113 px/s, 0,6 s, 2 px. | Prise d'un objet. | EONCharacter.cpp:1475-1476 | OK |

## 2. Commun aux ennemis

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| COM-V1 | Flash de coup | Tous les matériaux remplacés par `MI_BG_pipe` blanc, EmisAdd (6, 6, 6), pendant 0,07 s. | Ennemi touché (F6 « HitFlash »). | EONEnemy.cpp:223-240, 358, 743-744 | OK |
| COM-V2 | Éclaboussure | 4 éclats vers le tireur (3 couleur du monstre, 1 blanc), 105 px/s, 0,3 s, frein 5, à mi-hauteur. Couleurs : Nuke #e8a24a, Onibi #ff7a1a, Kama #e0405a, Ikiryo #e8923a, Ubume #8a6aaa ; autres #c84a7a. | Chaque coup reçu. | EONEnemy.cpp:362-371 | OK |
| COM-V3 | Éclats d'impact | 4 éclats blancs dans le sens du coup, 100 px/s, 0,25 s, à 4 px du sol. | Chaque coup reçu. | EONEnemy.cpp:372-373 | OK |
| COM-V4 | Gerbe de mort | 18 éclats en cône dans le sens du dernier coup (1 sur 3 blanc), 170 px/s, 0,6 s, 1,3 px, frein 5. Couleur de la table des morts. Secousse 0,18. | Mort. | EONCombatDirector.cpp:432-440 | OK |
| COM-V6 | Arrêt sur image | Temps global à 0,0001 pendant X ms réelles. Mort : 55 ms (100 %). Coup de 3 dégâts ou plus : 30 %. Explosion : 60 %. | Voir colonne précédente. | EONTuningSubsystem.cpp:93-106 ; EONCombatDirector.cpp:431, 480 ; EONEnemy.cpp:386 | OK |
| COM-V7 | Tremblement caméra | Décalage aléatoire de la caméra de 8 × trauma² px (50 cm au plus). Trauma plafonné à 1, baisse de 1,8 par seconde, × réglage Shake. | Tous les `Shake` et `AddTrauma`. | EONPlayerCameraManager.cpp:31-35, 101-107 ; EONCombatDirector.cpp:537-543 | OK |
| COM-V5 | Marqueur d'apparition | Carré `DrawDebugBox` magenta (255,61,139) au sol, qui se resserre de 10 à 4 px et clignote, 0,7 s. | Avant chaque apparition. | EONCombatDirector.cpp:705-718 | DEBUG |
| COM-V8 | Poussière d'apparition | Poussière au sol à l'arrivée d'un ennemi. | À l'apparition de chaque ennemi, avec le marqueur de 0,7 s (COM-V5). | **Codé le 24/11 :** EONCombatDirector.cpp:420 (gerbe de 12 grains) et :421 (anneau 12 px) ; **Demandé par :** MANQUANTS.md §7 (« pas faits ») ; Passation Sabri 13/11, 07_ASSETS_3D.md §5 | OK |
| COM-U1 | Chiffres de dégâts | `DrawDebugString` blanc, ambre (255,194,61) à partir de 3 dégâts. Monte à 38 px/s avec une gravité de 60, 0,6 s. Le même canal affiche « Chute +1 », « +N donnees », « Pas assez » et « -N ». | Coup (F6 « DmgNums ») ; chute, ramassage, achat. | EONEnemy.cpp:376-385 ; EONBoss.cpp:212-217 ; EONCombatDirector.cpp:683-703 ; EONPedestal.cpp:89-92 | DEBUG |
| COM-U2 | Barre de vie | `DrawDebugLine` : fond (42,16,32) et remplissage rouge, épaisseur 3, 12 px de large (20 px pour un élite), au-dessus de la tête. | Ennemi blessé, ou élite. | EONEnemy.cpp:781-788 | DEBUG |
| COM-U3 | Nom d'élite | `DrawDebugString` rose (255,176,192). | Élite. | EONEnemy.cpp:789-792 | DEBUG |
| COM-C2 | Élite agrandie | Modèle × 1,45, sans teinte ni aura. | Élite. | EONEnemy.cpp:150-157 | OK |
| COM-C1 | Chute dans le vide | L'ennemi rétrécit de 1 à 0,2 et s'enfonce de 2 m en 0,45 s. | Ennemi poussé dans un trou. | EONEnemy.cpp:427-432, 714-720 | OK |
| COM-C3 | Cadavre | Anim de mort, 0,6 s au sol, puis s'enfonce de sa hauteur en 0,5 s. | Mort, pour un modèle qui a une anim de mort. | EONEnemy.cpp:308-343 | OK |
| COM-V9 | Trous dans le modèle (punchHole) et sang | Trous percés dans le corps à chaque coup (8 au plus), tube dans l'axe de la caméra. Rayon de la formule du proto × 0,45 (Vincent 24/11 : « trop gros »). À chaque trou, gerbe de sang dans le sens du tir (cône ±63°) : 6 à 16 gouttes selon la taille du trou, rouge #c0142e et rouge sombre #5e0816 (1 sur 3), 170 px/s, 0,5 s, 1,3 px, frein 3. | À chaque coup reçu par un ennemi. | **Codé le 24/11 :** EONEnemy.cpp:378-379 PunchHole (rayon) et :394-397 (sang, EONFx::GerbeDir2) ; masque Trou0..7 dans M_EON_ToonEnnemi ; vidéo Saved/Shots/EON_COM-V9_Trous_Sang.mp4 ; **Demandé par :** Proto pixel (punchHole) ; Vincent 24/11 (taille et sang) | OK |
| COM-V10 | Balle, traînée et halo des tirs ennemis | Balle à 10 % de sa taille (2,5 cm ; 3,75 cm pour le gros tir), émissive `M_EON_Glow` intensité 8 (Vincent 24/11). Halo rond de 7 × S2 px, traînée de 16 × S2 px de long (1 m ; 2 m pour le gros tir) et 2,5 × S2 px de large, allongée le 24/11 (« trop petites »). Lumière couleur du tir, 14 px (22 px pour le gros tir). | Derrière chaque tir ennemi (Ikiryo, Méduse, gardien), pendant tout son vol. | **Codé le 24/11 :** EONEnemyProjectile.cpp:61-65 (balle), :84 (halo), :85-87 (traînée), :88-94 (lumière) ; **Demandé par :** MANQUANTS.md §7 (sphères « sans halo ni lumière ») ; Passation Sabri 13/11, 07_ASSETS_3D.md §5 ; Vincent 24/11 (tailles) | OK |
| COM-V11 | Butin au sol | Petites formes `M_Flat`, couleur × 3 : ADN #8af07a, scrap #ffc23d, énergie #3ef2e0, cœur #ff3d55, Données #9ad8ff. Elles flottent (±6 cm) ; l'ADN file vers Jira. | Mort d'un ennemi. | EONPickup.cpp:41-53, 88-94, 160 | OK |

## 3. Nuke

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| NUK-V1 | Couleur d'éclats | Éclaboussure (COM-V2) et gerbe de mort (COM-V4) ocre #e8a24a. | Coup, mort. | EONEnemy.cpp:368 ; EONCombatDirector.cpp:435 | OK |

## 4. Onibi

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| ONI-C2.1 | Clignotement d'armement | Clignote en blanc à 10 Hz (flash COM-V1) et tremble de ±1 px pendant 0,45 s ; anim Explosion. | Onibi à moins de 18 px de Jira. | EONEnemy.cpp:517-525, 712, 743 | OK |
| ONI-C2.2 | Cercle d'armement | `DrawDebugCircle` orange (255,154,61) au sol, rayon 10 ± 2 px qui pulse. | Armement. | EONEnemy.cpp:772-775 | DEBUG |
| ONI-V1 | Éclats d'explosion | 34 éclats orange #ff7a1a et blancs (1 sur 3), toutes directions, 180 px/s, 0,5 s, 1,25 px, frein 5. | Explosion après l'armement (rayon 28 px). | EONEnemy.cpp:526-531 ; EONCombatDirector.cpp:470-477 | OK |
| ONI-V2 | Anneau d'explosion | `NS_EON_Anneau` #ff7a1a, de 0 à 28 px de rayon (3,5 m), 0,25 s. Secousse 0,45 ; hitstop × 0,6. | Même déclencheur. | EONCombatDirector.cpp:477-480 | OK |
| ONI-C4 | Explosion à la mort | 34 éclats #ff9a3d et blancs, anneau #ff9a3d de 26 px, 0,25 s. Secousse 0,45 ; hitstop × 0,6. S'ajoute à la gerbe de mort #ff7a1a. | Onibi tué avant d'exploser. | EONCombatDirector.cpp:441 | OK |
| ONI-V3 | Lumière d'explosion | Éclair lumineux. | À l'explosion de l'Onibi, avec ONI-V1 et ONI-V2. | **Codé le 24/11 :** EONCombatDirector.cpp:485 → EONFx::Lumiere (0,25 s) ; **Demandé par :** Proposé par le relevé des fiches du 24/11 (fiche ONI-V3) ; pas demandé par la passation | OK |

## 5. Kama

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| KAM-C2.1 | Tremblement d'élan | Tremble de ±1 px pendant 0,55 s ; anim Attack. | Élan, à moins de 85 px de Jira. | EONEnemy.cpp:558, 712 | OK |
| KAM-C2.2 | Ligne d'élan | `DrawDebugLine` rouge (255,61,85), épaisseur 1,5, longue de 70 px dans l'axe de la charge, à 6 px du sol. | Élan. | EONEnemy.cpp:768-771 | DEBUG |
| KAM-V1 | Traînée de charge | Points rouges #e0405a de 2 px, immobiles, 0,2 s, environ 48 par seconde. | Charge de 0,32 s à 235 px/s. | EONEnemy.cpp:570-572 | OK |
| KAM-V3 | Choc contre un mur | Secousse 0,12. | Charge qui heurte un mur. | EONEnemy.cpp:689 | OK |
| KAM-V2 | Couleur d'éclats | Rouge #e0405a. | Coup, mort. | EONEnemy.cpp:368 ; EONCombatDirector.cpp:437 | OK |
| KAM-V4 | La chose (KAM-X1) : traînée | Points lie-de-vin #3a1020 de 2 px, 0,25 s, environ 36 par seconde. | Charge à 240 px/s pendant 0,75 s (salle Irruption). | EONEnemy.cpp:837-840 | OK |

## 6. Ubume

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| UBU-C2 | Cercle de ponte | `DrawDebugCircle` jaune-vert (216,240,106) de 8 px au sol. | Ponte (0,75 s). | EONEnemy.cpp:776-779 | DEBUG |
| UBU-T1 | Œuf | Sphère `M_Flat` #d8f06a × 2, environ 37 × 49 cm. Vole en cloche en 0,55 s (monte de 14 px), tremble de plus en plus au sol, éclot à 2 s. | Ponte. | EONEnemyProjectile.cpp:97-132 ; EONCombatDirector.cpp:496-507 | OK |
| UBU-V1 | Éclosion | 16 éclats vert #8aa82a et jaune-vert #d8f06a (1 sur 2), 160 px/s, 0,35 s. | L'œuf éclot sur du sol. | EONCombatDirector.cpp:671-674 | OK |
| UBU-V2 | Couleur d'éclats | Coup : violet #8a6aaa. Mort : blanc, car l'Ubume manque dans la table des morts. | Coup, mort. | EONEnemy.cpp:369 ; EONCombatDirector.cpp:435-437 | OK |

## 7. Ikiryo

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| IKI-C2 | Ligne de visée | `DrawDebugLine` rouge (255,61,85), épaisseur 1, de 14 à 240 px, à 13 px du sol. Devient blanche, épaisseur 3, au tir (0,12 s). | Visée (1,1 s), puis tir. | EONEnemy.cpp:750-755 | DEBUG |
| IKI-T1 | Gros tir | Sphère orange #e8923a émissive (`M_EON_Glow`), 3,75 cm (10 % de l'ancienne, Vincent 24/11), à 81 cm du sol, 10,6 m/s, 3 s. Halo, traînée et lumière : COM-V10. | Tir. | EONEnemy.cpp:605 ; EONEnemyProjectile.cpp:59-65 | OK |
| IKI-C3 | Secousse au tir | Secousse 0,14. | Tir. | EONEnemy.cpp:606 | OK |
| IKI-V2 | Flash de bouche de l'Ikiryo | Éclair au canon. | À chaque tir de l'Ikiryo, au bout de son canon. | **Codé le 24/11 :** EONEnemy.cpp:645 → EONFx::Eclair (e8923a, 0,08 s) ; **Demandé par :** Proposé par le relevé des fiches du 24/11 (mecaniques_persos.md:429) ; pas demandé par la passation | OK |
| IKI-V1 | Couleur d'éclats | Orange #e8923a. | Coup, mort. | EONEnemy.cpp:368 ; EONCombatDirector.cpp:436 | OK |

## 8. Gardien de l'étage 1 (le Tentaculaire)

Ce groupe ne fait pas partie de la liste demandée. Il est ajouté pour que l'inventaire soit complet.

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| GAR-V1 | Flash blanc du gardien | Chair en EmisAdd (6, 6, 6) pendant 0,07 s, et 0,06 s à chaque explosion de la mort. | Coup reçu. | EONBoss.cpp:210, 450-456 | OK |
| GAR-V2 | Éclats sur invulnérable | 3 éclats gris-violet #8f8abd, 64 px/s, 0,2 s. | Coup pendant l'intro, un changement de phase ou la mort. | EONBoss.cpp:200-204 | OK |
| GAR-V3 | Éclaboussure du gardien | 3 éclats rose-chair #c84a7a vers le tireur (le premier blanc), 85 px/s, 0,3 s. | Coup reçu. | EONBoss.cpp:207-209 | OK |
| GAR-V4 | Cuve qui se fissure | Vitre verte (31,90,72) qui tremble jusqu'à ±4 cm et s'éclaircit (EmisAdd de 0,15 à 2,15). Secousses au hasard de 0,05 à 0,2. | Intro (3 s). | EONBoss.cpp:294-300, 483-494 | OK |
| GAR-V5 | Vitre qui éclate | 60 éclats vert d'eau #7affd0 et blancs (1 sur 3), 250 px/s, 0,75 s, départ ±10 px. Secousse 1 ; hitstop de 140 ms ; le gardien grossit. | 0,7 s avant la fin de l'intro. | EONBoss.cpp:301-308 | OK |
| GAR-V6 | Carton-titre | Bande sombre ; le nom s'écrit lettre par lettre en rose ; sous-titre ; 2,6 s. | Vitre éclatée. | EONHUD.cpp:323-334 ; EONBoss.cpp:306 | OK |
| GAR-V7 | Corps vivant | Boule de chair #8a2a4a qui respire (±4 %) et vibre à l'élan de charge. Rouge vif en phase 3, grise quand il est invulnérable. Œil blanc dont la pupille rouge lumineuse (EmisAdd 4) suit Jira. 4 tentacules de 6 sphères qui ondulent. | Permanent. | EONBoss.cpp:107-117, 442-480 | OK |
| GAR-V8 | Changement de phase | Secousse 0,8 ; bannière « Phase II » ou « Phase III - Rupture » ; puis 1,5 s de petits tremblements (0,1). | À 66 % puis 33 % des PV. | EONBoss.cpp:186-192, 313-318 | OK |
| GAR-V9 | Tirs en éventail et en couronne | Sphères magenta #ff3d8b émissives (`M_EON_Glow`), 2,5 cm (10 % de l'ancienne, Vincent 24/11), à 56 cm du sol, de 58 à 100 px/s. Même classe que les tirs de la Méduse : halo, traînée et lumière de COM-V10. | Phases 1 à 3. | EONBoss.cpp:141-155, 351-371 ; EONEnemyProjectile.cpp:59-65 | OK |
| GAR-V10 | Annonce du tentacule | Bande rouge lumineuse (EmisAdd 3) au sol vers Jira, qui s'élargit et clignote (14 Hz), pendant 0,55 à 0,75 s. Vrai maillage, pas du debug. | Phases 2 et 3. | EONBoss.cpp:505-520 | OK |
| GAR-V11 | Tentacule qui frappe | Cylindre de chair qui jaillit en 0,16 s, reste 0,4 s, se retire en 0,3 s. Secousse 0,12. | Fin de l'annonce. | EONBoss.cpp:405-415, 521-526 | OK |
| GAR-V12 | Cercle de mortier | Anneau rouge lumineux qui clignote (8 Hz) et disque orange lumineux qui grandit jusqu'au rayon (22 px) en 1,1 s. | Phases 2 et 3. | EONBoss.cpp:529-544 | OK |
| GAR-V13 | Explosion de mortier | 26 éclats #ff9a3d et blancs, 150 px/s, 0,5 s. Anneau #ff9a3d du rayon, 0,25 s. Secousse 0,3. | Fin du cercle. | EONBoss.cpp:388-402 | OK |
| GAR-V14 | Charge contre le mur | Secousse 0,35. | Phase 3. | EONBoss.cpp:348 | OK |
| GAR-V15 | Mort : explosions en chaîne | Pendant 1,9 s, toutes les 0,13 s : 16 éclats #c84a7a et blancs à ±20 × ±17 px, 150 px/s, 0,4 s, flash blanc, secousse 0,35. Le corps rétrécit jusqu'à 25 %. Au début : hitstop de 160 ms et secousse 0,6. | PV à 0. | EONBoss.cpp:223-231, 279-291, 438 | OK |
| GAR-V16 | Mort : explosion finale | 90 éclats (60 #c84a7a, 15 #ffc23d, 15 blancs), 280 px/s, 0,9 s, frein 3. Anneau blanc de 70 px (8,75 m), 0,4 s. Secousse 1 ; hitstop de 220 ms. | Fin des 1,9 s. | EONBoss.cpp:236-249 | OK |
| GAR-V17 | Barre de vie du gardien | Barre rouge en bas au centre, grise quand il est invulnérable, avec des encoches à 66 et 33 %. | Combat. | EONHUD.cpp:335-343 | OK |
| GAR-V18 | Flaque d'acide, matière organique au sol | Zones dangereuses au sol. Elles appartiennent à la Mechakuve et au Chercheur, deux gardiens non portés. | Gardien de l'étage 2 (Mechakuve) : flaque après chaque jet d'acide (rayon 15, 4,5 s) ; matière organique sous Jira toutes les 2,6 s en phase 4. Étage 3 (Chercheur) : chair sous Jira toutes les 3 s. | **Codé le 24/11 :** Prêt, non branché : EONFx::Sol forme Flaque ; essai eon.fx flaque / organique (EONFx.cpp:227). À brancher avec la Mechakuve et le Chercheur ; **Demandé par :** Passation Sabri 13/11, 03_ENNEMIS_ET_BOSS.md:151-171 et 07_ASSETS_3D.md §5 ; ces gardiens ne sont pas encore portés | PRÊT |
| GAR-V19 | Télégraphes pieux, écrasement, balayage en cône | Cercles au sol et cône d'annonce des autres gardiens. | Avant chaque attaque des gardiens des étages 2 et 3 : cercle au sol (pieux, écrasement, 0,65 à 0,8 s), cône rouge (balayage, 0,55 s), ligne (bras étiré). | **Codé le 24/11 :** Prêt, non branché : EONFx::Sol formes Disque / Cone / Ligne ; essai eon.fx cercle / cone / ligne (EONFx.cpp:229). À brancher avec les gardiens des étages 2 et 3 ; **Demandé par :** Passation Sabri 13/11, 03_ENNEMIS_ET_BOSS.md:151-166 et 07_ASSETS_3D.md §5 (« toute attaque dangereuse s'annonce ») | PRÊT |

## 9. Décor et salles

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| DEC-V1 | Arcs électriques du HUB | 8 éclairs bleu électrique (0 ; 0,63 ; 4 en HDR), rubans face caméra en `MI_EON_Arc` additif. Halo × 3 à 40 %, 12 segments, 1 branche, épaisseur 16 cm. Scintillement 20 fois par seconde (de 20 à 100 %), forme refaite toutes les 0,06 s, extrémités qui sautent toutes les 0,4 à 1,2 s. Ils suivent la descente du C.O.M. | Permanent au HUB. | EONElectricArc.cpp:116-169 ; Public/EONElectricArc.h:41-55 | OK |
| DEC-V2 | Champ de force du dôme | Ondes orangées lumineuses sur la sphère du dôme (`M_EON_ChampForce` via `MPC_EON_Champ`), 4 à la fois au plus. Les hexagones s'allument près de Jira. | Balle qui traverse le dôme ; impulsion au hasard toutes les 3 à 6 s. | EONChampForce.cpp:56-100 | OK |
| DEC-V3 | Lampes qui clignotent ou vacillent | Lumière ponctuelle et néons. Mode « clignote » : 10 % un tiers du temps. Mode « vacille » : 12 % par à-coups. | Lampes posées dans ces modes. | EONLampe.cpp:85-107 | OK |
| DEC-V4 | Irruption : lumières qui vacillent | Exposition à −1,5 EV une fois sur trois, toutes les 0,05 s ; lampes éteintes ; 1,8 s. | Entrée dans la salle Irruption. | EONFloorManager.cpp:2231-2233 ; EONPlayerCameraManager.cpp:125-130 ; EONLampe.cpp:103-106 | OK |
| DEC-V5 | Irruption : le noir | Exposition à −5 EV. Seule la lampe de Jira éclaire : blanche, 60000, rayon 58 px (3,6 m), 80 cm au-dessus d'elle. Secousse 0,3 et bannière. | Après 1,8 s, jusqu'à la mort de la chose. | EONFloorManager.cpp:267-274, 2234-2263 | OK |
| DEC-V6 | Yeux de la chose | Lumière rouge #ff3d55 à 70 cm du sol : rayon 10 px et intensité 15000 quand elle rôde ; 22 px et 60000 quand elle charge. | Même salle. | EONFloorManager.cpp:275-280, 2264-2271 | OK |
| DEC-V7 | Cuves de réveil : lueurs | Lumière verte sous la cuve, rayon 4,5 m. Scellée #3a8a70 (5400) ; brisée, la même à 0,18 ; Jira #7affd0 ; ouverte #3ef2e0, faible. | Permanent au Kibo. | EONWakePod.cpp:69-74, 105-110, 152-159 | OK |
| DEC-V8 | Voyant des cuves scellées | Lumière #ff3d55 (2500, 1,5 m) et matériau `Light_red` qui clignotent ensemble (environ 0,75 Hz). Spécimen sombre qui flotte de ±3 cm. | Cuves scellées. | EONWakePod.cpp:75-80, 209-223 | OK |
| DEC-V9 | Cuves brisées | 16 éclats de verre fixes (dents sur les bords, débris au sol) et flaque verte émissive (opacité 0,22, émission 1,2). | Cuves brisées. | EONWakePod.cpp:85-89, 121-150 | OK |
| DEC-V10 | Réveil : vidange et ouverture | Le liquide vert baisse en 1 s (de 0,6 à 1,6 s). Le tube de verre descend en 0,45 s à partir de 1,7 s. Secousses 0,3 (à 1,7 s) et 0,25 (à 3,7 s). | Cinématique de réveil. | EONWakePod.cpp:225-247 ; EONFloorManager.cpp:596, 609 | OK |
| DEC-V11 | Réveil : vapeur | Bouffées (sphères `MI_EON_Vapeur`, opacité 14 % au plus) qui montent et grossissent de 4 à 6 fois, 0,7 à 1,1 s. 45 par seconde pendant l'ouverture, puis 20 d'un coup à 2,6 s (48 en tout au plus). | Même cinématique. | EONWakePod.cpp:93-102, 186-201, 235-256 | OK |
| DEC-V12 | Carte rouge | Pavé rouge lumineux (EmisAdd 3 ; 0,2 ; 0,35). | Salle Sécurité. | EONFloorManager.cpp:292-298 | OK |
| DEC-V13 | Carte prise | 30 éclats rouges #ff3d55 et blancs, 120 px/s, 0,5 s, 2 px. | Ramassage de la carte. | EONFloorManager.cpp:1440-1445 | OK |
| DEC-V14 | Embuscade, Trésor piégé | Secousse 0,4 (embuscade) ; secousse 0,6 (le Dormeur se réveille). | Carte prise ; disquette du Trésor prise. | EONFloorManager.cpp:1421, 1458 | OK |
| DEC-V15 | Porte rouge | Panneau rouge lumineux (EmisAdd 0,9 ; 0,05 ; 0,12). | Porte verrouillée par la carte. | EONTileRoom.cpp:111-122 | OK |
| DEC-V16 | Porte intérieure | Panneau orangé, faiblement lumineux. | Porte intérieure fermée. | EONTileRoom.cpp:435-442 | OK |
| DEC-V17 | Chair d'infestation | Boules rose-violet #8a2a4a à lueur faible, une par tuile, de tailles variées. Se rétracte en 1,6 s. Une variante bouche la passerelle du HUB. | Permanent ; rétraction quand le gardien est neutralisé. | EONTileRoom.cpp:70-106 ; EONFloorManager.cpp:1461-1468, 1868-1885 | OK |
| DEC-V18 | Socles d'objets | Gemme couleur de la rareté × 4 qui flotte (±25 cm) et tourne à 60°/s. | Socles. | EONPedestal.cpp:47-55, 95-96 | OK |
| DEC-V19 | Objet acheté ou pris | Achat : 10 éclats ambre #ffc23d. Objet gratuit : 10 éclats gris-violet #8f8abd sur chaque socle. 57 px/s, 0,4 s. | Prise d'un objet sur un socle. | EONFloorManager.cpp:1703-1705 | OK |
| DEC-V20 | Faille : brèche | 40 éclats cyan et blancs, départ ±30 px, 120 px/s, 0,5 s. Secousse 0,6. | Ouverture de la brèche, à 1,2 s puis à 6 s. | EONFloorManager.cpp:2036-2039 | OK |
| DEC-V21 | Console de confinement | Cube violet sombre, éteint, qui devient cyan lumineux (EmisAdd 0,4 ; 3 ; 2,8). | Salle vidée. | EONFloorManager.cpp:1251, 2097 | OK |
| DEC-V22 | Paroi confinée | 30 éclats cyan, 100 px/s, 0,5 s. Secousse 0,3. | Console activée. | EONFloorManager.cpp:2115-2117 | OK |
| DEC-V23 | Terminal du C.O.M | Cube cyan lumineux (EmisAdd 0,4 ; 3 ; 2,7) qui tourne à 45°/s, puis à 540°/s pendant la descente. | Permanent au HUB. | EONFloorManager.cpp:1306-1308, 1813-1822 | OK |
| DEC-V24 | Descente du C.O.M : secousses | Levier : 0,5. Puis 3 à-coups de plus en plus forts, une vibration de 0,06 toutes les 0,35 s, et un choc de 0,3 à l'arrivée. | Levier tiré. | EONFloorManager.cpp:1360-1365, 1397, 1762 | OK |
| DEC-V25 | La gamine et Ulysse tremblent | Le PNJ tremble de ±0,6 px (gamine) ou ±0,5 px (Ulysse). | Salle Événement. | EONFloorManager.cpp:1166-1167 ; EONNpc.cpp:75 | OK |
| DEC-V26 | Flash blanc du PNJ | PNJ blanc, EmisAdd 2, pendant 0,15 s. | Gamine touchée par un ennemi. | EONFloorManager.cpp:2347 ; EONNpc.cpp:62-73 | OK |
| DEC-V27 | La gamine mute | 30 éclats rose pâle #ffb0c0 et brun #7a3a30, 180 px/s, 0,5 s, 2 px. Secousse 0,6. | Arrivée sans la peluche. | EONFloorManager.cpp:2537-2540 | OK |
| DEC-V28 | Cible d'entraînement : flash | Le cube lilas devient blanc pendant 0,07 s et recule (6 px). | Cible touchée. | EONTrainingDummy.cpp:75-78, 105-113 | OK |
| DEC-V29 | Cible d'entraînement : chiffres et DPS | `DrawDebugString` des dégâts et du « DPS ». | Cible touchée. | EONTrainingDummy.cpp:118-131 | DEBUG |

## 10. Interface

| code | nom | ce qu'on voit | déclencheur | fichier:ligne ou source | état |
|---|---|---|---|---|---|
| INT-V1 | Fondu de changement de salle | Fondu vers #07060f en 0,18 s, puis réouverture en 0,18 s. | Porte franchie. | EONFloorManager.cpp:1284-1290, 1317-1330 | OK |
| INT-V2 | Fondu de la descente du C.O.M | Fondu au noir lissé de 2,7 à 3,7 s, 0,25 s de noir, réouverture en 0,9 s. | Descente du C.O.M. | EONFloorManager.cpp:1341-1388 | OK |
| INT-V3 | Bannière centrale | Bande sombre à 60 % sur toute la largeur, à 30 % de la hauteur de l'écran, texte coloré, fondu de 0,3 s, 1,8 s. | Événements. | EONHUD.cpp:377-384 ; EONFloorManager.cpp:1293-1298 | OK |
| INT-V4 | Encart « DÉBLOQUÉ » | Encart en haut au centre avec fondu d'entrée et de sortie, 3,2 s. | Déblocage méta. | EONHUD.cpp:535 | OK |
| INT-V5 | Voiles des écrans | Voiles #07060f : dialogue 55 %, terminal du C.O.M 80 %, titre 86 %, choix de niveau 78 %. | Ouverture de ces écrans. | EONHUD.cpp:451, 558, 594, 761 | OK |
| INT-V6 | Écran de mort « SIGNAL PERDU » | Voile très sombre à 75 %, titre rouge. | Mort de Jira. | EONHUD.cpp:477-480 | OK |
| INT-V7 | Bilan de mort, bannière de secours | Messages `GEngine->AddOnScreenDebugMessage` (bilan, et bannière quand le FloorManager manque). | Mort ; bannière sans FloorManager. | EONCombatDirector.cpp:241, 755-760 | DEBUG |
| INT-V8 | Clignotements du HUD | Barre d'énergie blanche qui clignote sous 25 % ; rouge qui clignote en mutation ; « PRET » de l'ultime ; jauge de charge blanche quand elle est pleine. | Voir colonne précédente. | EONHUD.cpp:112, 120, 154, 175 | OK |
| INT-V9 | Texte de debug F6 | Bloc de texte `GEngine` (entrée, énergie, vitesse, Shake…). | Réglage `DebugText`. | EONPlayerController.cpp:375, 439-464 | DEBUG |

---

## Isolés (hors vertical slice)

- Petite Nuke (gouleP) : éclaboussure et gerbe de mort vert #6fbf4a (EONEnemy.cpp:367 ; EONCombatDirector.cpp:435). OK.
- Brainless : ligne de verrouillage `DrawDebugLine` rouge clignotante, épaisseur 2, de 10 à 90 px (EONEnemy.cpp:756-759). DEBUG.
- Brainless : poussière de charge beige #b0a488, environ 36 grains par seconde, 35 px/s, 0,25 s (EONEnemy.cpp:640). OK.
- Brainless : secousse 0,45 quand il touche Jira (EONEnemy.cpp:645). OK.
- Brainless : choc contre un mur, 12 éclats crème #e8e0c8 et blancs, 110 px/s, 0,35 s, secousse 0,3 (EONEnemy.cpp:690-695). OK.
- Brainless : étourdi, 3 points ambre `DrawDebugPoint` qui tournent au-dessus de la tête (EONEnemy.cpp:760-767). DEBUG.
- Brainless : éclaboussure #c86a6a ; gerbe de mort blanche, car il manque dans la table des morts (EONEnemy.cpp:370). OK.
- Méduse : 3 petits tirs magenta #ff3d8b émissifs de 2,5 cm (COM-V10) en éventail de ±0,22 rad, 82 px/s (EONEnemy.cpp:548). OK.
- Méduse : flotte à 5 ± 1 px du sol (EONEnemy.cpp:710). OK.
- Méduse : éclaboussure et gerbe de mort #8f7be0 (EONEnemy.cpp:368 ; EONCombatDirector.cpp:436). OK.

---

## Totaux par état (sections 1 à 10, hors isolés)

| propriétaire | OK | DEBUG | ABS | total |
|---|---|---|---|---|
| Jira | 30 | 6 | 7 | 43 |
| Commun ennemis | 10 | 4 | 3 | 17 |
| Nuke | 1 | 0 | 0 | 1 |
| Onibi | 4 | 1 | 1 | 6 |
| Kama | 5 | 1 | 0 | 6 |
| Ubume | 3 | 1 | 0 | 4 |
| Ikiryo | 3 | 1 | 1 | 5 |
| Gardien (Tentaculaire) | 17 | 0 | 2 | 19 |
| Décor et salles | 28 | 1 | 0 | 29 |
| Interface | 7 | 2 | 0 | 9 |
| **Total** | **108** | **17** | **14** | **139** |

Isolés (hors vertical slice) : 10 lignes, dont 8 OK et 2 DEBUG.
