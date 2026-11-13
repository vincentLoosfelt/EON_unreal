# Décor et éléments de salle en PNG — spécification (v0.45.0)

Statut : 🔄 système en place, aucune tuile remplacée pour l'instant (tout vient encore du code).

## Principe
Chaque tuile du décor peut être remplacée par un PNG. Pour toute tuile absente, le jeu garde celle dessinée par le code : on peut donc remplacer **une seule tuile** sans rien casser, et avancer au fil de l'eau.

## Format
- **Hauteur du PNG = 16 px exactement.** Une tuile = 16 px de large. Une feuille = les tuiles côte à côte, de gauche à droite.
- 1 pixel du PNG = 1 pixel interne du jeu (affiché ×3), comme les sprites de personnages.
- Un PNG de mauvaise hauteur est refusé avec un message (« hauteur 16 px attendue »).
- Transparence autorisée. Elle compte pour les props et le décor solide, qui se posent **par-dessus le sol**. Le sol, les murs et la cuve du boss doivent être opaques.

## Fichiers (12 feuilles = 4 types × 3 ambiances)
Ambiances : `coursive`, `machines`, `labo`.

| Fichier | Tuiles (de gauche à droite) |
|---|---|
| `sol_<ambiance>.png` (10) | 0 uni A · 1 uni B · 2 uni C · 3 plaque/grille · 4 néon · 5 flaque · 6 rayures · 7 tache sombre · 8 rivets · 9 tuyau |
| `mur_<ambiance>.png` (6) | 0 dessus de mur (sans façade) · 1 façade unie · 2 façade néon · 3 façade conduit · 4 façade conduit + vanne · 5 façade voyants |
| `prop_<ambiance>.png` (5) | 0 terminal cathodique · 1 faisceau de câbles · 2 tuyauterie · 3 lecteur à bandes · 4 plaque gravée |
| `deco_<ambiance>.png` (4) | 0 terminal (bloc solide) · 1 caisse métal · 2 tonneau · 3 cuve du boss |

Exemple : `sol_labo.png` fait 160 × 16 px. Si tu ne dessines que les 3 premières tuiles, il peut faire 48 × 16 px : les 7 autres restent celles du code.

## Utilisation
1. **Dossier `sprites/`** à côté du fichier HTML : `sprites/sol_labo.png`, chargé au démarrage.
2. **Glisser-déposer** du PNG sur la page (build dev), ou bouton « Charger des PNG ».
3. Bouton « Revenir au code » : tout revient au dessin d'origine.

## Ce qui reste dessiné par le code (volontairement, pour l'instant)
- Bords sombres et liseré du haut des murs : **pas d'autotuile**. Les murs en PNG n'ont donc que leur propre dessin. Si l'effet manque, on ajoutera des tuiles de coin.
- Bandes de danger au bord du vide, falaises du vide, câbles au sol, chevrons devant les portes.
- Lumières et néons (halos) : elles restent liées au type de tuile (néon du sol, façade néon, terminal). Si tu remplaces ces tuiles par autre chose, le halo restera au même endroit.
- Passe peinture (taches et usure par-dessus la salle) : s'applique encore sur tes tuiles tant que l'option « peinture » est active.
- Encore dessinés par le code uniquement : la cuve du boss et son animation de rupture, l'alcôve du marchand (le mur qui pivote), les œufs d'Ubume, les zones de danger du boss, les projectiles et les effets.
- La couleur d'écran des terminaux (prop 0) est ambre dans le PNG ; le halo, lui, garde la couleur tirée au hasard par le code.


---

# Éléments de salle (v0.45.0)

Même mécanisme que les tuiles : un PNG dans `sprites/`, par glisser-déposer ou via « Charger des PNG ». Pas de PNG = le dessin du code. **Ici une frame = un état** (ouvert, verrouillé, etc.), les frames sont côte à côte. La hauteur et la largeur d'une frame sont imposées : un PNG de mauvaise taille est refusé avec un message qui donne la taille attendue.

| Fichier | Frame | Frames | Ancrage | États (de gauche à droite) |
|---|---|---|---|---|
| `porte_h.png` | 32 × 16 | 4 | coin haut-gauche de la porte | 0 ouverte · 1 verrouillée · 2 carte rouge voyant allumé · 3 carte rouge voyant éteint |
| `porte_v.png` | 16 × 32 | 4 | idem | idem |
| `cuve_reveil.png` | 30 × 46 | 5 | coin haut-gauche | 0 scellée (silhouette) · 1 Jira dans le liquide · 2 Jira, liquide vidangé · 3 brisée · 4 vide |
| `com.png` | 34 × 34 | 1 | centré sur le C.O.M | socle seul |
| `console_faille.png` | 14 × 14 | 3 | coin haut-gauche | 0 éteinte · 1 allumée · 2 allumée (pulse) |
| `ascenseur.png` | 28 × 18 | 2 | centré sur la plateforme | 0 verrouillé · 1 actif |
| `carte_rouge.png` | 11 × 8 | 1 | centré (la carte oscille toute seule) | carte |
| `piedestal.png` | 16 × 6 | 1 | sous l'objet | socle |
| `item_icones.png` | 12 × 12 | 5 | **centré** sur l'objet | 0 disquette · 1 cassette · 2 greffe · 3 écho · 4 perk |
| `ramassables.png` | 8 × 8 | 3 | **centré** | 0 scrap · 1 énergie · 2 cœur |

**Portes** : dessine la porte du **Nord** (`porte_h`) et celle de l'**Ouest** (`porte_v`), le jeu fait le miroir pour le Sud et l'Est. Le petit point coloré qui annonce la salle suivante (opportunité, ascenseur) reste dessiné par le code par-dessus la porte ouverte. Le liseré cyan qui pulse sur la porte ouverte est dans le code : si tu veux le garder, dessine-le dans ta frame.

**Cuve de réveil** : le jeu choisit la frame selon l'état et l'avancement de la vidange (frame 1 tant que la cuve est pleine, frame 2 ensuite, frame 4 quand le verre est ouvert). La baisse continue du niveau de liquide, les bulles et le contour sombre d'un pixel ne sont pas repris en mode PNG.

**Ce qui reste du code par-dessus** : l'hologramme de l'étage et le texte « C.O.M », les textes « CONFINEMENT / HORS LIGNE », « Ascenseur », « Verrouillé », le prix en scrap sous un objet, le halo de rareté derrière un objet, l'ombre de la carte rouge, le point d'info des portes.

**Couleurs d'objets** : en mode PNG, la teinte de chaque catégorie vient de ton dessin, plus du code.

## Fichiers fournis
- 12 feuilles de tuiles (`sprites/v0.44.0/decor/`) et 10 feuilles d'éléments (`sprites/v0.45.0/elements/`) : le rendu actuel du code, exporté tel quel, avec une planche-contact étiquetée chacune.
- Le bouton « Exporter les placeholders » du panneau dev les génère tous (en local ; sur le lien en ligne, le téléchargement est bloqué).

## v0.49.1 — nouveaux éléments remplaçables (25/10/2026)
Règle de Sabri : **tout nouvel élément visuel arrive avec son placeholder PNG**, sans qu'il ait à le demander. Placeholders : `sprites/v0.49.1/` (planches `_planche_nouveautes_v0.49.1.png` et `_planche_contact.png`).

| Fichier | Frame | Frames | Contenu |
|---|---|---|---|
| `cobaye.png` | 18×28 | 3 | Le Cobaye (garde, foulée, passage), tourné vers la droite. `cobaye_v.png` optionnel = variante mutée |
| `gamine_mutee.png` | 14×25 | 2 | La gamine mutée (placeholder = Brainless teinté rose) |
| `cuve_specimen.png` | 32×56 | 5 | Cuves de l'étage 2 : 4 spécimens ratés, puis la cuve brisée. Pod de 2×3 tuiles, posé 4 px plus bas que le haut de la frame |
| `generateur.png` | 16×30 | 1 | Générateur du bouclier de Mechakuve. Ancrage bas-centre. Barre de PV et flash dessinés par le code |
| `gamine.png` | 24×28 | 2 | La gamine devant sa grille d'aération : 0 = recroquevillée, 1 = calmée avec Lapin. Le « ? » reste en code |
| `ramassables_quete.png` | 12×12 | 3 | 0 = insigne d'équipage, 1 = log, 2 = peluche Lapin. Centrés |
| `boutons_manette.png` | 12×9 | 11 | Icônes Xbox des invites manette (v0.59.3) : 0 A, 1 B, 2 X, 3 Y, 4 LB, 5 RB, 6 LT, 7 RT, 8 Menu, 9 Vue, 10 Croix. Affichées ×3 dans les dialogues |
| `mechakuve.png` | 48×92 | 4 | Corps de Mechakuve, une frame par phase (1 cuve fermée, 2 zones sensibles vertes, 3 ouverte sur le spécimen, 4 spécimen sorti). Pieds au centre, 2 px au-dessus du bas. Restent en code : pinces, bouclier, câbles, queue, vapeur, flash |
| `sol_cuves.png`, `mur_cuves.png`, `prop_cuves.png`, `deco_cuves.png` | 16×16 | 10 / 6 / 5 / 4 | Tuiles de l'ambiance « Cuves » (étage 2), même format que les 3 autres ambiances |

## v0.50.1 — course (30/10/2026)

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `jira_course_a0.png` … `jira_course_a3.png` | 19×29 | 2 | Jira qui court, une feuille par niveau d'armure (0 = combinaison, 3 = casque fermé). 0 = grande foulée, 1 = genou levé. Buste penché d'1 px vers l'avant. Ancrage bas-centre comme `jira_aX.png`. Le bras-arme reste dessiné par le code (rabattu vers l'arrière, il balance avec la foulée) |
| `cobaye_course.png` | 19×28 | 2 | Le Cobaye qui court, cape au vent. Mêmes frames et même ancrage |

Les veines de mutation sont ajoutées par le code par-dessus, comme pour la marche.

## v0.51.0 — terminal du C.O.M (30/10/2026)

| Fichier | Taille | Frames | Contenu |
|---|---|---|---|
| `com_terminal.png` | 424×236 | 1 | Cadre du terminal du C.O.M (fond, bordure, scanlines). Posé en x=28, y=18 de l'écran 480×270. Onglets, listes, icônes d'items et texte restent dessinés par le code par-dessus : garder la zone intérieure sombre et lisible |

## v0.52.0 — étage 3 « Les labos » et le Chercheur (30/10/2026)

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `sol_blocs.png`, `mur_blocs.png`, `prop_blocs.png`, `deco_blocs.png` | 16×16 | 10 / 6 / 5 / 4 | Tuiles de l'ambiance « Labos » (étage 3) : carrelage clinique froid, acier, néon rouge d'urgence. Même format que les autres ambiances |
| `silo_test.png` | 48×60 | 3 | Silo de test (capsule cryo scellée) : 0 occupé (silhouette), 1 vide allumé, 2 brisé. Bloc de 3×3 tuiles, l'image déborde de 12 px vers le haut. Lumière froide et givre animé restent en code |
| `table_operation.png` | 32×22 | 3 | Table d'opération : 0 nue, 1 drap sur un corps, 2 sangles et sang. Bloc 2×1 tuiles, déborde de 6 px vers le haut |
| `chercheur.png` | 112×112 | 5 | Le Chercheur : 0 humain (accueil), 1/2/3 bras mutant par phase, 4 bras levé (écrasement). Tourné vers la droite, pieds en x=40, y=108 (le jeu retourne l'image quand il regarde à gauche). Restent en code : télégraphes (cône, cercle), bras étiré, débris, chair au sol, flash |

## v0.53.0 — étage 3 en labo de recherche (30/10/2026)

L'ambiance « Labos » devient la seule ambiance claire du jeu : carrelage blanc cassé à joints de 8 px, murs gris clair, écrans bleus, obscurité réduite de moitié. Les tuiles `*_blocs.png` ont donc changé : reprendre celles de la v0.53.0.

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `poste_recherche.png` | 32×32 | 5 | Bureau blanc, 2 moniteurs, clavier. Bloc 2×1 tuiles, déborde de 16 px vers le haut. Les 5 frames = 5 combinaisons d'écrans |
| `analyseur.png` | 32×42 | 2 | Machine d'analyse : centrifugeuse vitrée, écran de scan, bande jaune/noir. Bloc 2×2, déborde de 10 px vers le haut |
| `etagere_bocaux.png` | 32×34 | 2 | Étagère de bocaux de spécimens (vert, ambre, rose, bleu). Bloc 2×1, souvent contre un mur, déborde de 18 px |
| `ecran_mural.png` | 32×16 | 5 | Grand écran fixé au mur nord, ne bloque rien. Contenu : 0 hélice d'ADN, 1 électrocardiogramme, 2 histogramme, 3 lignes de données, 4 scan de corps |

En PNG, les écrans sont figés ; en code, ils sont animés.

## v0.54.0 — étage 4 « Les serres » (30/10/2026)

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `sol_serres.png`, `mur_serres.png`, `prop_serres.png`, `deco_serres.png` | 16×16 | 10 / 6 / 5 / 4 | Tuiles de l'ambiance « Serres » : herbe, terre, armatures vert-de-gris, lampes de culture chaudes |
| `arbre_serre.png` | 48×64 | 3 | Arbre complet (tronc + feuillage) : 0 vert, 1 fruitier, 2 muté (violet, veines). Bloc 2×2 tuiles ; posé en x−8, y−28 du bloc. Devient transparent quand Jira passe derrière |
| `bac_plantes.png` | 32×22 | 3 | Bac de culture : 0 salades, 1 fleurs, 2 vrilles mutantes. Bloc 2×1, déborde de 8 px |
| `hydroponie.png` | 32×36 | 2 | Rayonnage hydroponique : 0 LED roses, 1 LED chaudes. Bloc 2×1, déborde de 20 px |
| `saule.png` | 112×112 | 1 | Le saule pleureur (tronc + rideau de branches). Pied du tronc en x=56, y=104. Transparent quand Jira est dessous |
| `vieille_dame.png` | 24×32 | 2 | La vieille dame assise : 0 avant la discussion, 1 après (tête relevée). Pieds en x=12, y=32. Le banc reste en code |

Restent en code : herbe et fleurs au sol (touffes, fougères, fleurs, champignons), balancement des branches, le « ? » et « [C] Parler ».

## v0.55.0 — le C.O.M (30/10/2026)

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `com.png` | 80×84 | 2 | **Remplace l'ancien socle 34×34.** La sphère du C.O.M façon gacha ball cassette futurism : coque basse en métal usé et plastique jauni, dôme de verre fumé ambre, noyau vert CRT, capsules, anneau équatorial à LED. 0 = en attente, 1 = prêt à descendre. Centre de la sphère en x=40, y=50 (le pied est vers y=68) |

Restent en code : la salle (plateforme circulaire, 4 passerelles, garde-corps, balises des portes, rouge vers le gardien), les entrailles de la station qui défilent dans le vide, l'hologramme de l'étage, le levier de l'onglet Descente.

## v0.56.0 — l'infestation (01/11/2026)

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `infestation.png` | 32×32 | 3 | Masse de chair qui bloque la passerelle et la porte du gardien. Une frame par case bloquée (0 avec œil, 1 veines, 2 vrilles), centrée sur la case 16×16 (déborde de 8 px de chaque côté). Le jeu la rétrécit et l'efface quand le gardien meurt |

## v0.58.0 / v0.59.0 — Données et events (01/11/2026)

| Fichier | Taille frame | Frames | Contenu |
|---|---|---|---|
| `donnee.png` | 8×8 | 1 | Puce de Données au sol (monnaie permanente). Centrée |
| `tresor.png` | 64×36 | 2 | Salle Trésor : 0 stock de pièces (caisses, engrenages), 1 carnage (corps, sang). Centre de la salle en x=32, y=12. Les ramassables et le piédestal restent en code |
| `cartographe.png` | 16×22 | 2 | Le gamin des conduits : 0 debout, 1 à terre. Pieds en x=8, y=20. Barre de PV et « ! » en code |

Irruption : pas de nouveau PNG. Elle réutilise `cuve_specimen.png` (une cuve brisée et vide), et le noir est géré par la lumière du jeu.
