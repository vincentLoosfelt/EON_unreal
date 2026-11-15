# EON — Recette jalon 1 : « Jira qui tire »

Écrit le 14/11/2026. **Compilé : non. Joué : non.** Aucun éditeur n'était connecté, donc ce code n'a jamais été compilé ni lancé. Premier test : ouvrir `EON.uproject` (ou lancer un build C++), corriger les éventuelles erreurs de compilation et me les envoyer.

Les captures et la comparaison se font avec `Docs/PASSATION_UNREAL/REFERENCE_PROTO/EON_v0.60.1_DEV.html`, dans la salle de réveil du Kibo.

## Ce qui a changé

| Élément | Où | Détail |
|---|---|---|
| Dash | `AEONCharacter::PressDash` | 2 charges, 345 px/s pendant 0,15 s, invulnérable 0,2 s, une charge rendue toutes les 1,1 s. Direction : le déplacement, ou la visée si Jira est immobile. |
| Course | `AEONCharacter::Tick` | Bouton de dash maintenu plus de 0,2 s en se déplaçant : ×`RunMul` (1,7). Pas de tir ni de visée en course. |
| Énergie | `AEONCharacter` | 100, remonte de 16/s après 0,7 s sans dépense. À 0 : tir gratuit (la mutation arrive au jalon 4). |
| Greffe Standard | `FireAuto`, `SpawnBullet` | 9 tirs/s, dispersion 3°, vitesse 250 px/s ± 6 %, vie 0,9 s, coût 0,7, tout lu dans `DT_Greffes`. |
| Projectile | `AEONProjectile` | Part du bout du bras (12 px devant, 17 px de haut). Meurt au premier mur ou à la première cible. |
| Cibles | `AEONTrainingDummy` | Vacille, clignote en blanc, chiffres de dégâts, « DPS » sur 1,2 s. 4 cibles posées par `AEONGameMode` si la carte n'en a pas. |
| Game feel | `OnShot` | Flash 0,06 s, kick caméra ×1,6, recul de Jira ×14 px/s, secousse 0,04 × kick. |
| Visée | `ApplyAimAssist`, `UpdateStickAim` | Aide à la visée (cône, 220 px). Au stick : angle « franc » gardé 0,45 s, RT ou stick à fond tire. |
| Hitstop | `UEONTuningSubsystem::RequestHitstop` | Prêt, mais seul un coup à 3 dégâts ou plus le déclenche. |

## À faire dans l'éditeur

1. Ouvrir le projet : Unreal propose de recompiler le module `EON`. Accepter.
2. **Rien à créer à la main** : `BP_Jira`, `BP_EONGameMode` et les entrées de J0 suffisent. Les cibles sont posées par le code au lancement.
3. Si le matériau `M_Flat` manque, relancer `Saved/Scripts/eon_setup_j0.py` éditeur fermé. Sans lui, balles et cibles apparaissent en gris.

## Recette

| # | Étape | Attendu |
|---|---|---|
| 1 | Lancer Play. | 4 cubes sur poteau dans la salle. La ligne cyan affiche `EON J1`, l'énergie et les charges de dash. |
| 2 | Clic gauche maintenu vers une cible. | Les balles partent du bout du bras, un flash éclaire le bras. Un chiffre `1` s'envole à chaque touche, `DPS` s'affiche, le cube vacille et clignote. |
| 3 | Tirer en continu. | L'énergie descend (0,7 par tir), la caméra recule un peu à chaque tir, Jira recule légèrement. À 0, le tir continue. Après 0,7 s sans tirer, l'énergie remonte. |
| 4 | Appuyer sur Espace ou Maj. | Jira file dans le sens de la marche (ou de la visée si elle est immobile). Charges `2/2` → `1/2`, `DASH` affiché. Deux dashs d'affilée, le troisième ne part pas avant la recharge (1,1 s). |
| 5 | Garder Espace enfoncé en marchant. | Après 0,2 s : `COURSE`, vitesse d'environ 139 px/s. Impossible de tirer en courant. |
| 6 | `eon.feel.preset sec` puis tirer. | Plus de flash, de kick, de secousse ni de recul, et plus de chiffres ni de clignotement. La différence doit sauter aux yeux. `eon.feel.preset studio` pour revenir. |
| 7 | `eon.feel RateMul 2`, puis `eon.feel Spread 3`. | Cadence doublée ; gerbe plus ouverte. |
| 8 | Manette : stick droit à fond, ou RT. | Jira tire. En relâchant le stick, elle continue de viser dans la même direction pendant 0,45 s. |
| 9 | Comparer avec le proto, côté salle de réveil. | Le tir « pèse » pareil : cadence, recul, secousse. Noter tout écart de rythme ou de force. |

## Ce que je n'ai pas pu vérifier

- La compilation (aucune erreur vue, mais aucun compilateur lancé).
- Que les balles touchent vraiment les cibles (collision `Pawn` côté cible, `BlockAllDynamic` côté balle).
- Le ressenti du recul de Jira, du kick et de la secousse.
- Les chiffres de dégâts : `DrawDebugString` ne marche pas en build Shipping.
- Le hitstop : jamais déclenché avec la Standard.

## Pas encore fait (notés dans MANQUANTS.md)

Douille éjectée, réticule qui s'écarte, impact des balles, traînée du dash, poussière de course, sons, modèle 3D de Jira (`Cyborg_mode.fbx` et ses 16 anims, à brancher dans `BP_Jira`).
