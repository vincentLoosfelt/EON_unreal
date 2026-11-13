# 05 — Progression et méta

Données : `DONNEES/items.json`, `DONNEES/deblocages.json`, `DONNEES/dialogues.json` (clé `cassettes`).
Dans le code : `ITEMS`, `rollItems()`, `rollLevel()`, `takeItem()`, `gainADN()`, `killEnemy()`, `UNLOCKS`, `checkUnlocks()`, `META`, `CASS`, `crewSync()`.

## 1. Les trois ressources

| Ressource | Rôle | Durée de vie | D'où elle vient |
|---|---|---|---|
| **ADN** | Expérience | Perdue à la mort | Chaque ennemi tué |
| **Scrap** | Monnaie du marchand | Perdu à la mort | Ennemis (rare), élites, gardiens, trésors, Ulysse |
| **Données** | Monnaie permanente | **Gardées pour toujours** | Élites, gardiens, trésors, Irruption |

## 2. L'ADN et les niveaux

- Chaque mob lâche son ADN en petites gemmes (par paquets de 3 au plus). Valeurs : Nuke 1, Onibi 1, Méduse 2, Ikiryo 2, Brainless 2, Kama 2, Ubume 3. Élite +6. Gardien 25.
- Les gemmes sont **aspirées** vers Jira après 0,35 s, où qu'elle soit dans la salle.
- Passer du niveau N au suivant coûte `40 + 14 × N` ADN. Niveau 1 → 2 : 54. Niveau 2 → 3 : 68. Ça donne environ 3 niveaux par étage.
- **Le choix ne s'ouvre jamais en combat** : il attend que la salle soit nettoyée et que tout l'ADN soit ramassé, puis 0,6 s.
- **Choix 1 parmi 3** : trois cartes tirées dans les Perks et les Disquettes (pas de greffe, de cassette ni d'écho ici). Le tirage est pondéré par la rareté.

## 3. Les objets

36 objets, 5 catégories. Table complète avec l'effet exact de chacun : `DONNEES/items.json`.

| Catégorie | Nombre | Ce que c'est |
|---|---|---|
| **Disquette** | 11 | Un bonus de stat ou un soin : +1 PV max, +8 % vitesse, +15 énergie, +15 % dégâts, −12 % recharge, +30 % Données, kit de soin, aimant, plaque de blindage, catalyseur ADN, récupérateur de scrap |
| **Perk** | 16 | Un effet sur l'arme ou le dash : cadence, épaisseur, portée, charge, dash élargi, charge de dash, Renaissance ; 3 perks réservés aux greffes à projectiles ; 6 perks réservés à la Griffe |
| **Greffe** | 5 | Change la greffe active : Standard, Rafale, Laser, Lame, Shotgun |
| **Cassette** | 1 | Shield |
| **Écho** | 3 | Change une règle de la mutation : Berserk, Stabilisateur, Régulateur |

**Rareté et tirage** : Commun (poids 60), Rare (30), Épique (10). On tire sans remise dans le lot.

**Filtres du tirage** (`rollItems`) : un objet ne sort pas si
- il est verrouillé (section 6) ;
- il est lié à une greffe (`gr`) qui n'est pas la greffe active ;
- il est unique (`once`) et déjà possédé ;
- c'est la greffe qu'on a déjà en main ;
- c'est la cassette Shield et on l'a déjà débloquée.
Au plus une greffe par tirage.

**Perks liés à une greffe**
- Standard, Rafale, Shotgun : Balles perforantes (+1 ennemi traversé), Ricochet (+1 rebond sur les murs), Double canon (2 puis 3 projectiles côte à côte).
- Griffe : Bras long, Prise, Fauche (un grand arc toutes les 3 frappes), Rayon latent (un rayon toutes les 4 frappes), Vorace, Double griffe (frappe deux fois, à 0,13 s d'écart).

**Où on trouve des objets**
| Source | Combien | Prix |
|---|---|---|
| Niveau d'ADN | 3 cartes, on en prend 1 | Gratuit |
| Salle Opportunité | 3 socles, on en prend 1 | Gratuit |
| Marchand | 3 socles, on achète ce qu'on veut | Commun 8, Rare 16, Épique 28 scrap. +6 pour une greffe. |
| Gardien vaincu | 2 socles, on en prend 1 | Gratuit |
| Trésor (stock) | 1 disquette | Gratuit (piège possible) |
| Irruption, Ulysse, la gamine | 1 objet rare ou épique | Gratuit |

**Ramasser** : on marche sur le socle (à moins de 9 px). Une fiche s'affiche quand on s'approche. Chez le marchand, sans assez de scrap : « Pas assez ».

## 4. Le scrap

- Un mob lâche 1 scrap avec `12 % × sa valeur d'ADN` de chances (Ubume 36 %, Nuke 12 %).
- Élite : +3. Gardien : 12. Ennemi tombé dans le vide : +1 garanti. Trésor (stock) : 10. Insigne rendu à Ulysse : 10.
- Ordre de grandeur : 26 par étage, soit à peu près un achat au premier marchand.

## 5. Les ramassables

| Ramassable | Effet | Aimant |
|---|---|---|
| ADN | Expérience | Toute la salle |
| Scrap | +1 | 40 px |
| Énergie | +14 (rien si Jira est en mutation) | 40 px |
| Cœur | +1 PV. **Reste au sol si les PV sont pleins.** | 40 px |
| Données | +2 ou +3 | 40 px |
| Insigne, log, peluche | Quête | 40 px |
| Carte rouge | Ouvre la porte du gardien | Non (on marche dessus) |

Un ramassable ordinaire immobile au-dessus du vide finit par tomber. L'ADN et les objets de quête ne tombent jamais. L'Aimant ajoute 30 px au rayon.

## 6. Les déblocages entre les runs

Au tout début, il manque des choses au jeu. Elles se débloquent par des objectifs **cumulés sur toutes les runs**. Tant qu'un élément est verrouillé, il n'apparaît nulle part : ni au tirage, ni chez le marchand, ni dans la liste des greffes.

| Déblocage | Type | Objectif |
|---|---|---|
| Le Cobaye | Perso | Vaincre un gardien |
| Laser | Greffe | Atteindre l'étage 2 |
| Double canon | Objet | Faire 3 runs |
| Shield | Cassette | Atteindre l'étage 3 |
| Lame | Greffe | Éliminer 150 ennemis au total |
| Catalyseur Berserk | Objet | Atteindre le stade 3 de mutation |
| Shotgun | Greffe | Vaincre Mechakuve |
| Plaque de blindage | Objet | Rendre 3 insignes à Ulysse |
| Renaissance | Objet | Mourir 5 fois |
| Régulateur | Objet | Vaincre le Chercheur |
| Charge de dash + | Objet | Atteindre les serres |
| Double griffe | Objet | Vaincre un gardien avec le Cobaye |

- **Stats suivies** : nombre de runs, de morts, d'ennemis tués, étage le plus lointain atteint, stade de mutation le plus haut, gardiens vaincus par étage, gardiens vaincus avec le Cobaye.
- Vérification toutes les 0,5 s. Un déblocage s'affiche dans un encart « DÉBLOQUÉ » pendant 3,2 s. Ils sont récapitulés sur l'écran de mort.
- Jira démarre donc avec **Standard et Rafale**.
- Pour tester : prévois une commande « tout débloquer ».

## 7. La cassette Shield

Un bouclier qui absorbe un coup.

| Niveau | Comment | Effet |
|---|---|---|
| NV1 | Trouvée en run (une fois débloquée) | Absorbe 1 coup, puis 0,6 s d'invulnérabilité. Revient à l'étage suivant. **Une fois trouvée, elle est acquise pour toutes les runs suivantes** et ne sort plus dans le butin. |
| NV2 | 30 Données | Se recharge après 10 s sans prendre de dégâts |
| NV3 A « nettoyant » | 60 Données | Quand le bouclier casse : une onde de rayon 80 qui fait 5 dégâts (8 sur un boss) et efface les tirs ennemis à moins de 110 px |
| NV3 B « miroir » | 60 Données | Quand le bouclier casse : les tirs ennemis à moins de 120 px repartent vers l'envoyeur, 1,6× plus vite, 2,5 dégâts |

- NV3 est un **choix définitif** entre A et B.
- L'achat se fait dans l'onglet **Cassettes** du terminal, **seulement au C.O.M à quai** (au Kibo). En run, l'onglet se consulte mais n'achète pas.
- Deux lignes grisées sont affichées, « à définir » : Cassette Mobilité et Cassette Cobra.

## 8. Les quêtes

**Ulysse et son équipage**
- 6 insignes à retrouver (voir `04`). On les rend à Ulysse au Kibo en lui parlant.
- Par insigne rendu : **+10 scrap**.
- Tous les **3 insignes rendus : +1 PV max permanent**. Deux au maximum, avec les 6.
- Le terminal suit la quête membre par membre : insigne rendu, insigne en poche, ou « ??? quelque part dans les étages ».

**La gamine** : une issue sur deux (calmée ou perdue), définitive.

**Les logs** : 8, dans l'ordre. Relisibles dans le terminal.

**La vieille dame** : une fois rencontrée, sa quête apparaît (« Quelqu'un lui a dit d'attendre. Qui ? »).

## 9. La mort

- Avec une **Renaissance** : Jira revient à la moitié de ses PV, 2 s d'invulnérabilité.
- Sinon : écran « Signal perdu ». Il affiche l'étage atteint, le niveau, les salles traversées, les ennemis éliminés, les chutes, le scrap, le nombre d'objets, et les déblocages obtenus pendant la run.
- Relancer : nouvelle run depuis le Kibo. La cinématique de réveil et le dialogue `wake` rejouent à chaque run : Jira est un nouveau clone.

## 10. La sauvegarde

Le proto garde un seul objet de sauvegarde (`META`). À reprendre dans un SaveGame :

| Champ | Contenu |
|---|---|
| `chara` | Le perso choisi au Kibo |
| `st` | Les stats cumulées : `runs`, `morts`, `kills`, `maxFloor`, `maxMut`, `boss[étage]`, `bossCob` |
| `unl` | Les déblocages obtenus |
| `donnees` | Le compteur de Données |
| `cass` | Par cassette : niveau et branche (`A` ou `B`) |
| `ulysse` | Ulysse a été sauvé |
| `crew` | Par membre d'équipage : rien, `poche` ou `rendu` |
| `hpBonus` | Les PV max gagnés avec les insignes (se recalcule depuis `crew`) |
| `logN` | Le nombre de logs lus |
| `newtDone`, `newtEnd` | La gamine : rencontrée, et l'issue |
| `vieille` | La vieille dame a été rencontrée |
| `cobUnlock`, `metCobaye` | Le Cobaye : débloqué, déjà rencontré |
| `dockSeen`, `donHint` | Dialogues d'aide déjà vus |

**Écran titre** : s'il existe une progression, le jeu propose **Continuer** ou **Nouvelle partie**. Nouvelle partie efface tout, après une double confirmation. Sans progression, le jeu démarre directement sur le réveil.
