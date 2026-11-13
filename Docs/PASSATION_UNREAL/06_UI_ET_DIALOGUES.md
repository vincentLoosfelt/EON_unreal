# 06 — Interface et dialogues

Positions données sur l'écran interne du proto, 480 × 270. En UMG, garde les mêmes proportions : multiplie par 4 pour du 1920 × 1080.
Données : `DONNEES/dialogues.json`. Captures : `CAPTURES_PROTO/`.
Dans le code : `drawHUD()`, `drawMinimap()`, `drawBigMap()`, `drawCOMUI()`, `drawLevelUp()`, `drawDialog()`, `drawDead()`, `drawTitle()`, `drawTargeting()`.

Le proto dessine toute son interface avec une police bitmap 3 × 5 en majuscules. Ne la reproduis pas : prends une police lisible. La charte UI du studio prévoit Kode Mono et Oxanium, avec la palette `#E9A100` / `#EF1400` / `#01060C`. Elle n'a pas été appliquée au proto. Demande à Sabri s'il veut l'appliquer dès maintenant ou garder les couleurs du proto pour la comparaison.

## 1. Le HUD en jeu

**En haut à gauche**, du haut vers le bas :
1. **PV** : une rangée de carrés rouges (un par PV max), les PV perdus en sombre. Un carré cyan à la suite si une cassette Shield est équipée (éteint si elle est consommée).
2. **Énergie** : une barre cyan, avec le mot « Énergie ». Elle clignote en blanc sous 25 %. La part d'énergie max perdue à cause de l'armure est hachurée en rose au bout de la barre. **En mutation**, la barre devient rouge et clignote (de plus en plus vite avec le stade), le mot devient « Mutation I / II / III », et trois petits carrés indiquent le stade.
3. **Dash** : un petit trait par charge. Celui qui se recharge se remplit.
4. **ADN** : une barre verte fine et « ADN niv N ». Elle clignote quand un niveau attend.
5. Les objets de quête en main : carte rouge, insignes, peluche.

**En bas à gauche** :
- Le nom de la greffe active, dans sa couleur (« Cobra : Standard »).
- Onde, Holo, Ultime : chacun avec sa touche (ou son icône manette) et une petite barre de recharge. Grisés pendant la recharge, rouge sombre quand la mutation les bloque. L'ultime a une jauge, et clignote « PRÊT » à 100 %.

**En haut à droite** :
- La **mini-carte** (section 2).
- Le nom du rôle de la salle, dans sa couleur.
- « Étage N » et le scrap. Les Données en dessous, dès qu'on en a.
- « Vague 1/2 » pendant un combat.

**Au centre**, selon le moment :
- Une **bannière** : une bande horizontale avec un texte court (1,8 s). Elle sert à tout : nom de la salle à l'entrée, « Salle nettoyée », « Mutation - stade 2 », nom d'un objet ramassé, « Niveau 3 ».
- L'encart **DÉBLOQUÉ** (3,2 s).
- Le **carton-titre** d'un gardien : son nom écrit lettre par lettre en grand, un sous-titre, 2,6 s.
- Un **log** : un encart en haut au centre, « TRANSMISSION - LOG 3/8 », le texte s'écrit à 40 caractères par seconde.
- Pendant la Faille : la jauge **OXYGÈNE** en bas au centre, puis « CONFINEMENT PRÊT » ou « ASPHYXIE ».

**En bas au centre**, contre un gardien : sa barre de vie avec son nom et des encoches aux seuils de phase. Grise quand il est invulnérable, cyan sous bouclier. Si le boss est hors champ, une flèche au bord de l'écran l'indique.

**Dans le monde** :
- Les **chiffres de dégâts** qui s'envolent (blanc, jaune à partir de 3).
- Le **réticule de ciblage** sur l'ennemi le plus proche de la ligne de visée (moins de 280 px, à moins de 0,6 rad). Son dessin évolue avec le niveau d'armure : deux crochets discrets (niveau 0), un anneau de chair qui bat (1), un viseur orange segmenté (2), un viseur complet avec un identifiant de cible (3).
- Les ennemis qui viennent d'apparaître sont encadrés brièvement (à partir du niveau 1).
- La **fiche d'objet** quand on s'approche d'un socle : catégorie, nom, rareté, description, prix.
- L'invite d'interaction : « Consulter le C.O.M », « Parler », « Incarner ».

**Effets plein écran** : flash rouge quand Jira est touchée. En mutation : teinte rouge qui pulse, aberration chromatique à partir du stade 2, bandes de parasites et neige au stade 3.

## 2. La carte

- **Mini-carte** : une case par salle (8 × 6 px). Seules les salles vues sont affichées : la salle où on est, et ses voisines directes. Les portes sont des traits entre les cases. La salle courante clignote.
- Des points de couleur marquent ce qui compte : Opportunité (ambre), gardien (rouge, cyan une fois la carte en main), carte rouge (rouge), Événement (violet), salle hostile entamée et pas finie (magenta). Le marchand déjà visité a sa propre icône, pour savoir où retourner.
- **Grande carte** (Tab maintenu) : la même en grand, avec la couleur du rôle de chaque salle, une légende, et « Salles visitées 6/15 ». Le jeu continue mais Jira ne bouge plus.
- Avec la carte du Cartographe, toutes les salles et leur rôle sont révélés.

## 3. Le terminal du C.O.M

Un grand cadre (424 × 236) sur fond assombri. Titre « C.O.M // TERMINAL », l'étage et le perso en haut à droite. Le jeu est en pause. Cinq onglets :

| Onglet | Contenu |
|---|---|
| **Améliorations** | À gauche, la liste des objets de la run, regroupés (×n). À droite, la fiche de l'objet choisi : rareté, catégorie, description. En tête : le perso, la greffe active, le niveau d'ADN, les PV (dont le bonus des insignes), le dash, le scrap. |
| **Logs** | Les 8 logs. Ceux qu'on a trouvés se relisent en entier. Les autres : « signal non capté ». |
| **Quêtes** | Une entrée par quête découverte : l'objectif de l'étage (où est le gardien, carte en main ou non), Ulysse et son équipage, la gamine, le Cobaye, la vieille dame, les déblocages (les objectifs restants s'affichent « ??? : condition »), les archives. |
| **Descente** | Le levier. Verrouillé ou prêt. |
| **Cassettes** | La liste des niveaux de cassette, le solde de Données, un bouton Installer. |

- Le terminal s'ouvre sur l'onglet Descente si le levier est prêt, sinon sur Améliorations.
- Navigation : gauche / droite pour les onglets (ou 1 à 5), haut / bas dans une liste, Entrée ou A pour agir, C, Échap ou B pour fermer.

Captures : `03_com_terminal_descente.png`, `04_com_terminal_cassettes.png`.

## 4. L'écran de niveau

Fond assombri. Titre « NIVEAU N - ADN ASSIMILÉ ». Trois cartes côte à côte (132 × 74 chacune) : catégorie, icône, nom, rareté, description. Si le perk est lié à une greffe, la carte le dit.

- Choix : 1, 2, 3, ou gauche / droite puis Entrée, ou A à la manette, ou un clic.
- Les 0,35 premières secondes, aucun choix n'est accepté, pour éviter une validation accidentelle en sortant d'un combat.
- Le jeu est figé. Les effets visuels continuent au ralenti (10 %).

Capture : `50_level_up_choix_1_parmi_3.png`.

## 5. Les dialogues

**La boîte** : en bas de l'écran, sur toute la largeur. Un fond assombri sur le jeu. Un **portrait** carré à gauche, encadré à la couleur du personnage. Le nom, puis le texte qui s'écrit lettre par lettre avec un petit son de voix propre à chaque personnage.

- Le jeu est en pause pendant un dialogue.
- Suite : Espace, E, Entrée ou A. Un premier appui affiche toute la ligne, le suivant passe à la ligne d'après.
- Passer tout le dialogue : Échap ou B.
- Chaque ligne a une **expression** pour le portrait : neutre, sourire, inquiet, colère, triste.
- Les portraits du proto sont des placeholders dessinés par le code. Les vrais portraits sont à produire (`MANQUANTS.md`).

**Les personnages** (`personnages` dans le JSON) : l'IA de la station (cyan), Jira (vert pâle), le Cobaye (rouge), la gamine (rose), Ulysse (ambre), le marchand (ambre), le Chercheur (blanc bleuté), le cartographe (vert), la vieille dame (vert pâle).

**Les 26 dialogues**, par déclencheur :

| Clé | Quand |
|---|---|
| `wake` (`wakeCob` pour le Cobaye) | À la sortie de la cuve |
| `dock` | Première entrée dans le C.O.M à quai |
| `iaIntro` | Première arrivée à l'étage 1 |
| `cuves`, `labos`, `serres` | Arrivée aux étages 2, 3, 4 |
| `redDoor` | Première fois devant la porte rouge sans la carte |
| `card` | En ramassant la carte rouge (l'embuscade suit) |
| `infestCom`, `infestBoss` | Premier contact avec l'infestation, côté C.O.M ou côté gardien |
| `rift` | La paroi cède |
| `marchand` | Première approche du marchand |
| `donnees` | Premières Données ramassées |
| `ulysse1`, `ulysse2` | Ulysse sauvé (première fois, fois suivantes) |
| `cobMeet`, `jiraMeet` | Première rencontre de l'autre perso au Kibo |
| `newtGood`, `newtBad` | La gamine, avec ou sans la peluche |
| `cartoStart`, `cartoWin`, `cartoLost` | Le cartographe |
| `chercheur`, `chercheurFin` | Avant et après le gardien de l'étage 3 |
| `vieille` | La vieille dame |

Les répliques d'Ulysse au Kibo sont construites par le code selon le nombre d'insignes (`ulysseHubLines`). Si on joue le Cobaye, les répliques de Jira lui sont attribuées et l'IA dit « sujet C » au lieu de « unité Jira ».

**Règle de rythme** (retour de playtest) : le narratif ne doit jamais casser le tir. Aucune scène ne bloque plus de quelques secondes, tout se passe, et les logs se lisent sans pause.

## 6. Les autres écrans

- **Écran titre** : seulement s'il existe une progression. Deux boutons, « Continuer » et « Nouvelle partie » (double confirmation).
- **Pause** : le jeu se fige, un voile sombre.
- **Mort** : « Signal perdu », les stats de la run, les déblocages de la run, l'invite pour relancer.
- **QTE de l'ultime** : fond assombri, les 5 flèches à saisir, une barre de temps. Chaque direction a sa couleur (haut cyan, bas magenta, gauche vert, droite ambre).
- **Cut-in de l'ultime** : une bande illustrée qui traverse l'écran pendant 1,1 s. C'est l'endroit prévu pour une illustration 2D façon anime des années 90.

## 7. Le son

Le proto synthétise tous ses sons par code (une trentaine, dans `sfx()`). Rien à récupérer comme fichier. La liste des sons à produire est dans `MANQUANTS.md`. En attendant, branche chaque événement sur un point d'appel sonore nommé, même vide, pour que le sound designer n'ait qu'à remplir.
