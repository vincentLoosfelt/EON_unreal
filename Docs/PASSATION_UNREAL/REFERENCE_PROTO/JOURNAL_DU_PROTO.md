# TODO du proto — état au 18/10/2026 (v0.45.0)

Légende : ✅ Validé / 🔄 En cours / ⚠️ À clarifier

## 1. À récupérer plus tard (décidé par Sabri)
| Sujet | Détail | Statut |
|---|---|---|
| Fluidité du liquide des cuves | En mode PNG, les cuves de réveil passent en 5 frames : on perd la baisse continue du niveau de liquide et les bulles. À récupérer un jour (animation en frames ou liquide dessiné par le code derrière un verre en PNG). | ⚠️ À faire |
| Liseré pulsant des portes ouvertes | Dans le code seulement. En mode PNG il faut le dessiner dans la frame, ou le remettre par code. | ⚠️ À faire |
| Flèches animées de l'ascenseur actif | Idem : en mode PNG, dans la frame. | ⚠️ À faire |

## 2. Pas encore remplaçables en PNG (dessinés uniquement par le code)
- La cuve du boss et son animation de rupture.
- L'alcôve du marchand (le mur qui pivote).
- Les œufs d'Ubume, les zones de danger du boss, les projectiles, les effets (explosions, tirs, particules).
- Le **Tentaculaire** (corps et tentacules). L'export de la feuille « boss » ne donne qu'un placeholder : à capturer ou à découper si on veut l'archiver.
- L'interface (HUD, cartes d'objets, carte de l'étage).
Règle : tout ce qui peut passer en sprite y passe. Le reste reste ici, en attente.

## 3. Élites
| Sujet | Décision | Statut |
|---|---|---|
| Garder les élites | Oui (18/10). Le GDD Entités les documente : version renforcée, barre de vie ou **bouclier à casser**, dégâts en plus, apparition en zone extérieure, X % de chance de lâcher une **greffe que Jira n'a pas**. | ✅ Validé |
| Limite de spawn | Les élites **ne s'enchaînent pas**. Règle validée le 18/10 : **1 élite max par salle, puis 2 salles de combat sans élite**. | ✅ Validé |
| Implémentation | **v0.46.0** : dès le 3e combat, 40 % de chance par salle (1re vague seulement, jamais corridor ni faille). Un mob Kama / Ikiryo / Brainless / Ubume devient élite : PV ×2,2, taille ×1,3, nom « <mob> élite ». Testé : 480 salles, aucun enchaînement, jamais 2 dans une salle. | 🔄 À valider en jeu |
| Reste | Bouclier à casser, dégâts en plus, vraie zone extérieure (le proto n'a pas de zones : substitut = 3 combats passés), drop de greffe que Jira n'a pas. | ⚠️ À faire |

## 4. Comportements documentés mais pas dans le proto
Source : GDD « Architecture des entités » (Nuke, Onibi, Ikiryo, Kama, élites). Pour l'Ubume mob, seul le fichier de présentation le décrit (position fixe, engendre des créatures). Plan (19/10) : les développer et les régler dans le **Banc de mobs** (artifact séparé), puis les reporter dans le proto.
- **Kama** : charge imparable puis déplacement par dash, immunisé au recul, enchaînement de coups à une lame puis deux lames.
- **Onibi** : résistance élevée aux dégâts, explosion à dégâts de zone dégressifs (maximum au centre).
- **Ikiryo** : grenades semi-organiques (dégâts de zone) et laser canalisé ; comportement **Appui** (garde ses distances, recule si on s'approche).
- **Ubume (mob)** : comportement **Appui**, invoque de petites unités.
- Zones **intérieure / intermédiaire / extérieure** : conditionnent quels mobs apparaissent (le proto n'a pas de zones).
- Priorité de cible : notée dans les docs, pas branchée dans le HUD de ciblage.
- Boss étage 1 : le GDD décrit 4 attaques (bombardement, contre-attaque, balayage, liane tentaculaire) et une transition de phase qui brise les cuves et réveille des entités. Le Tentaculaire du proto est une base différente : à comparer, attaque par attaque.

## 5. Contenu à produire
- **Mechakuve** (boss de l'étage 2) : 4 phases, bouclier avec générateur déporté, jets d'acide, pinces. Rien dans le proto.
- Rôle de l'**Agglomérat** (tableur) : mob, mini-boss ou abandon ? ⚠️
- Noms définitifs de **Brainless** et **Méduse** (hors documents). ⚠️

## 6. Décisions de fond encore ouvertes
- Monnaies : tranché le 18/10 (v0.47.0). **ADN = XP** (run, choix 1 parmi 3 à chaque niveau) · **Scrap** = monnaie de run (marchand) · **Données** = seule monnaie permanente (hors proto). 🔄 À valider en jeu.
  - Réglages actuels : ADN Nuke/Onibi 1, Méduse/Ikiryo/Brainless/Kama 2, Ubume 3, élite +6, boss 25 ; niveau N coûte 40 + 14×N (≈3 niveaux par étage). Scrap : 12 % × valeur du mob, élite +3, boss 12, chute +1 (≈26 par étage, ≈1 achat au 1er marchand).
  - À trancher plus tard : le Scrap reste-t-il en partie à la mort (convertie en Données) ? Le doc doit-il coûter du Scrap ou autre chose ?
- Le lien partagé en ligne est la build **dev** (avec panneau de réglages). Faut-il le passer en build joueur ? ⚠️
- **Git** : pas maintenant. Déclencheur : Vincent dans le code, ou passage à Unity. Les builds versionnées et le changelog font office d'historique d'ici là.

## 7. À valider en jeu par Sabri
- **Niveaux ADN** (v0.47.0) : rythme (~3 par étage), choix ouvert seulement en salle nettoyée, pool = perks + disquettes.
- **Scrap** (v0.47.0) : est-ce qu'on hésite assez chez le marchand ?
- **Élites** (v0.46.0) : fréquence 40 % par salle.
- Sprite de la **Nuke** (v0.42.0) : taille, lisibilité du visage, teinte verte ou plus pâle.
- Comportement **Nuée** (v0.43.0) : taille de meute (3), durée de fuite seule (3,5 s), intensité du débordement.
- Crash après le boss de l'étage 1 : non revu depuis la v0.41.2. Si ça se reproduit, copier le texte du cadre rouge.
- Réglages ouverts de la passation (Q1 à Q8) : cohérence des deux Goules, masse de la Goule, design de la Méduse, pose de Jira, durée d'oxygène de la faille (32 s), soin du boss (55 %).

## 8. Premier retour de playtest (20/10/2026)
- ✅ Le gunfeel donne envie de revenir : « satisfaction de tirer sur des monstres », c'est la base du jeu.
- ✅ Demande de **plus de lore / d'histoire / de narratif**.
- ⚠️ Inquiétude : mêler narratif et gunfeel dynamique sans casser le rythme.
- Pistes (lore actuel : `DOC\SCÉNARIO\LORE_ACTUEL.md`) : événements de salle (Ulysse, doc, gamine/Newt, cartographe, droïde), dialogues courts Hub/COM, logs à ramasser, Codex, clone de la run précédente qui revient muté.
- Plan de test proposé : 1) une ligne de dialogue ou un log par étage, sans pause forcée ; 2) un vrai event narratif (gamine ou doc) pour voir si c'est vécu comme récompense ou coupure ; 3) fragments de lore au Hub après la mort.
- Règles : aucune scène ne bloque plus de quelques secondes, le contenu narratif reste optionnel.

## 9. En attente (20/10/2026)
- Docs UI (`DOC\UI`) lus : charte, moodboard, UX/UI du 17/09. Écarts proto : polices (Kode Mono / Oxanium) et palette (#E9A100 / #EF1400 / #01060C) non appliquées au HUD ; arbre de compétences absent du proto ; cassette « s'use » (lore) vs « se stocke » (maquettes). Décision : on traite plus tard.
- Banc de mobs : choisir le premier mob à développer (Ikiryo, Kama ou Onibi).

## 10. Pistes pour plus tard (20/10/2026)
- **Satisfaction des récompenses façon Vampire Survivors** (peut-être, à voir) : le retour joueur parle de la satisfaction de tirer sur des monstres. Idée : renforcer le plaisir de récolter et de monter en puissance (gemmes ADN qui s'aspirent, pluie de loot, level-up très lisible et gratifiant, effets sonores et visuels sur les paliers). Base déjà en place : ADN aimanté, choix 1 parmi 3 à chaque niveau (v0.47.0). À définir quand on y reviendra : ce qu'on veut en prendre sans diluer le gunfeel ni l'ambiance.

## 11. v0.48.0 (25/10/2026) — Ulysse au Kibo, le Cobaye jouable, étage 2 « Les cuves »
**Fait**
- **Ulysse** : sauvé à l'étage 1, il ne revient plus dans les étages. Il se réfugie au **Kibo** (salle de réveil = hub) et y reste d'une run à l'autre.
  - Ensuite, la salle Événement garde un **membre d'équipage muté** (élite : Kama ou Brainless, « Ex-mécanicienne », « Ex-pilote »…). Il lâche son **insigne**.
  - Les insignes ramassés sont gardés même si on meurt (objets de quête). On les rend à Ulysse au Kibo, au début de la run suivante.
  - Récompenses (provisoires) : **+10 scrap par insigne**, et **+1 PV max permanent toutes les 2 insignes rendues** (3 max). Équipage : 6 membres.
- **Le Cobaye**, second perso jouable (d'après la planche « Cobaye sheet ») : cheveux rouges, bandage, cape rouge, bras muté à griffes.
  - **v0.48.1 : il n'apparaît au Kibo qu'après le premier boss vaincu** (bannière « Une cuve s'est ouverte au Kibo »).
  - Il attend au Kibo à côté de sa cuve brisée. **[C] / Entrée / Select** pour l'incarner (au Kibo seulement, avant de partir en run). Le perso choisi est retenu.
  - Kit : **1 Griffe** (le bras se détend en ligne, blesse tout ce qu'il traverse et ramène les cibles) · **2 Rayon** (tir chargé orange) · **3 Fauche** (grand arc au corps à corps). Pas de greffes de loot.
  - Stats : 7 PV, un peu plus lent, 1 seul dash mais recharge rapide.
  - **La mutation est sa force** : +20 % de dégâts par stade, pas de dégâts doublés ni de ralentissement, sorts bloqués seulement au stade 3, 12 % de chances de regagner 1 PV par kill en état muté.
  - Première rencontre au Kibo : petit dialogue. Ses répliques remplacent celles de Jira, et l'IA l'appelle « sujet C ».
- **Étage 2 « Les cuves »** (ambiance Alien 4, programme de clonage) : nouvelle palette ocre sale + néons ambre. Cuves de spécimens (4 silhouettes d'expériences ratées, cuves brisées et vidées) contre les murs nord et sud, posées sans jamais couper un passage. Biomasse au pied des murs, flaques de liquide amniotique.
  - Plus de mobs organiques (Brainless, Méduse, Ubume).
  - Dialogue d'arrivée de l'IA.
  - Testé : 887 salles d'étage 2 générées, 320 avec cuves, aucune erreur.
- Panneau dev : bouton **Réinitialiser la méta** (perso, Ulysse, insignes). La méta est gardée dans le navigateur.

**À valider en jeu**
- Ressenti du Cobaye (portée et attraction de la griffe, rayon, fauche, 1 seul dash), lisibilité du sprite (placeholder).
- Rythme de la quête d'Ulysse et valeur des récompenses.
- Ambiance de l'étage 2 (assez Alien 4 ? trop sombre ?).

**Reste ouvert**
- Boss de l'étage 2 : c'est encore **le Tentaculaire** (renforcé). **Mechakuve** reste à faire.
- Le Cobaye dans le lore : personnage à part (homme, planche dédiée) ou variante de Jira (clones de la scientifique, cf. `LORE_ACTUEL.md`) ? ⚠️
- Le Banc de mobs n'a pas été republié. Son générateur fonctionne avec la v0.48.0, vérifié.
- Feuilles PNG du décor « cuves » (`sol_cuves`, `mur_cuves`…) : elles existent, mais pas encore de placeholders exportés.

## 12. Positionnement (formule de Sabri, 25/10/2026)
**« Un perso qui se joue comme un vaisseau dans FTL, des évolutions comme Vampire Survivors, un mapping comme Binding of Isaac. »**
- ➜ 03/11/2026 : Hades ajouté pour le lore (l'histoire avance run après run). Doc complet : `DOC\GDD\EON_Positionnement_References_v1.md` (+ version Claude Docs).
- **FTL : le perso comme un vaisseau.** Le corps de Jira est un ensemble de systèmes à gérer. Dans le proto : les greffes (armes), l'énergie partagée entre tir, sorts et armure, la mutation qui prend le relais quand l'énergie est vide, l'armure qui coûte du plafond d'énergie. À creuser : arbitrer l'énergie entre systèmes, systèmes endommagés et réparables ?
- **Vampire Survivors : les évolutions.** L'ADN fait monter de niveau, avec un choix 1 parmi 3 (perks, disquettes). À creuser : des synergies et évolutions de combinaisons (2 objets qui fusionnent), et la satisfaction des récompenses (section 10).
- **Binding of Isaac : le mapping.** Une grille de salles, une carte, des salles spéciales (opportunité, marchand, sécurité/carte rouge, événement, ascenseur du boss). À creuser : salles secrètes, étages à thèmes.
- **Pixel art vs 3D : ce que change la DA (Sabri, 25/10/2026).**
  - En pixel art, on perd clairement l'identité « animation 2D ». La version 3D se différenciait par son anime 2D rendu en 3D, face aux jeux qui font du 3D classique.
  - En pixel art, on joue plutôt sur la DA et la nostalgie des jeux de l'époque.
  - Piste pour garder une part d'anime en pixel : les cut-ins (déjà là pour l'ultime), les portraits de dialogue et les rencontres animées « façon 90's » (l'idée de Newt), en illustration 2D par-dessus le jeu en pixel.

## 13. Décisions du 25/10/2026 et v0.49.0
**Décisions**
- ✅ Le Cobaye est **une variante de Jira** (« tous ne font qu'un »).
- ✅ Les testeurs reçoivent la **build joueur** (sans panneau). Elle a son propre lien, séparé du lien dev.
  - Lien testeurs : https://claude.ai/artifact/6hrw2qoS7NxXUnw8UNvk8X (à partager depuis le menu Partager). Lien dev : https://claude.ai/artifact/SKayxPsyUBwQKcyXZ788wt. On publie les deux à chaque version.
- ✅ Les noms Brainless / Méduse et l'Agglomérat sont gardés tels quels pour le moment.
- ✅ Les élites restent telles quelles. Les comportements de mobs du GDD sont en pause.

**Fait en v0.49.0**
- **Mechakuve**, boss de l'étage 2, d'après les planches de la bible (mis-en-page_3_boss) : une machine-cuve mécano-organique ancrée, 240 PV, 4 phases.
  1. Cuve fermée : jets d'acide qui laissent des flaques, tentacules qui jaillissent sur des points fixes (annoncés par un cercle).
  2. (75 %) **Bouclier** : invulnérable tant que les **2 générateurs** placés loin du boss tiennent (câbles verts visibles). Coup de bouclier si on colle. Bouclier détruit = boss étourdi 2,5 s.
  3. (50 %) La cuve s'ouvre sur le spécimen. Les **pinces attrapent** si on est trop près ou immobile (annonce de 0,55 s, un dash l'évite) : 1 PV et on est tiré vers le boss.
  4. (25 %) Le spécimen sort, une queue apparaît. La **matière organique** au sol ralentit et blesse, et la machine avance.
- **Un log par étage**, à ramasser, lu en surimpression sans pause (8 logs tirés de la trame du 21/08, dans l'ordre, d'une run à l'autre).
- **La gamine** (event optionnel, salle Événement de l'étage 2, une fois) :
  - Sa peluche « Lapin » est cachée à l'étage 1.
  - Avec la peluche, elle se calme et donne un objet rare ou épique.
  - Sans, elle mute en élite (« La gamine ») ; une fois tuée, elle lâche un objet.

**À valider en jeu**
- Difficulté et lisibilité de Mechakuve (pinces, acide, générateurs bien visibles ?).
- Les logs : bon rythme, bonne longueur ?
- La gamine : trouve-t-on la peluche, le choix est-il compris ?

**Reste ouvert**
- La gamine : la branche « on l'ignore, elle se souvient de toi et revient mutée » (doc Events et PNJ) n'est pas faite. ⚠️
- Le sprite de la gamine mutée est un Brainless élite rose (placeholder).

## 14. Direction « une greffe = un perso » (Sabri, 25/10/2026) — à suivre
- **Chaque greffe est liée à un perso jouable** : un perso = une greffe.
- **Les perks sont liés à leur greffe** : chaque perso a ses perks propres.
- Un perso pourra prendre la greffe d'un autre, mais seulement via une **mécanique rare, encore à inventer**. ⚠️
- **Items neutres** (pour tous les persos) à ajouter par la suite.

**Écarts avec le proto actuel (v0.49.0)**
- Jira a les 5 greffes (touches 1 à 5) et le Cobaye en a 3 (griffe, rayon, fauche). Cette règle remplace ce système.
- Les 7 perks actuels sont communs, et la plupart sont en fait **neutres** : dash élargi, charge de dash, renaissance. Seuls « cadence du mode actif », « portée », « épaisseur » et « charge rafale/laser » touchent l'arme.
- Les 5 objets « GREFFE » du loot (changer d'arme) n'ont plus de sens tels quels. Ils sont la base naturelle de la future mécanique rare.
- Les disquettes (stats), cassettes et échos sont neutres par nature : ils forment la base des items neutres.

**À trancher avant de coder** ⚠️
- La greffe de Jira : Standard ? Et les 4 autres (rafale, laser, lame, shotgun) : de futures variantes de Jira, ou on les retire ?
- Le Cobaye garde-t-il un seul mode (la griffe), le rayon et la fauche devenant ses perks ?

**Branche « greffes » (25/10/2026)** : un artifact séparé, le proto principal ne change pas.
- Lien : https://claude.ai/artifact/SFfPcH76fCZXzo9VFfmX4p. Elle est fabriquée par `mk_greffe.py` à partir du proto : chaque correction du proto s'y retrouve en la régénérant.
- Choix par défaut : **Jira = le Cobra standard**, **le Cobaye = la griffe**. En jeu, chaque perso n'a que sa greffe.
- **Toutes les greffes restent testables pour le GDD** (demande de Sabri) : le panneau dev les liste toutes (« * » = la greffe du perso, « test » = les autres).
- **Les perks suivent la greffe équipée**, pas le perso : perforation, ricochet et double canon pour les tirs à projectiles (standard, rafale, shotgun), les 6 perks de griffe pour la griffe. Cadence, calibre et portée valent pour toutes les greffes. Rafale, laser, lame, shotgun, rayon et fauche n'ont pas encore de perks propres. ⚠️
- Les objets « greffe » ne tombent plus. Un objet d'un autre perso ne sort jamais (ni au loot, ni au niveau). La mécanique rare reste à inventer.
- **Perks de Jira** (Cobra) : cadence, calibre, portée, balles perforantes, ricochet, double canon (jusqu'à 3 tirs côte à côte).
- **Perks du Cobaye** (griffe) : bras long, prise, fauche toutes les 3 frappes, rayon latent toutes les 4 frappes, vorace (vol de vie), double griffe.
- **Items neutres** : 5 disquettes ; kit de soin, aimant, plaque de blindage, catalyseur ADN, récupérateur de scrap ; dash élargi, charge de dash, renaissance ; cassette shield ; les 3 échos.
- La carte de niveau indique à qui va le perk (JIRA, COBAYE ou TOUS).
- Testé : 400 tirages par perso, aucun objet de l'autre perso, aucune greffe au sol ; tous les perks fonctionnent ; aucune erreur.

## 15. v0.49.1 — tout en placeholder (25/10/2026)
- ✅ **Règle permanente (Sabri)** : tout nouvel élément visuel arrive avec son placeholder PNG remplaçable, sans demander.
- Fait : Cobaye, gamine (et mutée), cuves à spécimens, générateur, insigne / log / peluche, corps de Mechakuve (une frame par phase), tuiles de l'ambiance Cuves. Spécification : `PROTO_PIXEL_decor.md`. Placeholders : `sprites/v0.49.1/` (48 fichiers, pack complet).
- Testé en mode PNG : Mechakuve, les générateurs, la gamine et les cuves s'affichent bien depuis les feuilles.

## 16. Le Kibo comme refuge (Sabri, 25/10/2026)
- ✅ Ulysse n'est pas au Kibo au départ : on le rencontre en run, puis il rejoint le Kibo pour se mettre en sécurité. C'est déjà le comportement du proto (v0.48.0) : il n'apparaît au Kibo qu'une fois sauvé, et cette méta est gardée dans le navigateur.
- **Modèle à généraliser** : d'autres personnages pourront suivre le même chemin (rencontre en run, sauvetage, refuge au Kibo). Candidate évidente : la gamine calmée avec sa peluche. ⚠️ À décider.

## 17. v0.50.0 / v0.50.1 — la course (30/10/2026)
- ✅ v0.50.0 : maintenir le bouton du dash = courir (façon Hades). Le dash part à l'appui, la course prend le relais après 0,2 s. Pas de tir ni de visée pendant la course. Vitesse réglable dans le panneau (« Vitesse de course », 1,7× par défaut).
- ✅ v0.50.1 : vraie anim de course (2 frames) pour Jira (4 niveaux d'armure) et le Cobaye, bras-arme rabattu. 5 PNG remplaçables (voir PROTO_PIXEL_decor.md).
- ⚠️ Question ouverte : « je perds plus de PV par chute » — le code applique toujours 1 PV par chute (réglage `fallDmg`). À préciser : quelle situation, quel perso, quel étage ?

## 18. v0.51.0 — le C.O.M consultable (30/10/2026)
- ✅ Dans la salle de départ de chaque étage : s'approcher du C.O.M → invite « [C] Consulter le C.O.M » (C / Entrée, B à la manette, tap sur l'invite en tactile). Le jeu est en pause.
- ✅ Onglet **Améliorations** : perso, greffe active, niveau d'ADN, PV (dont bonus permanents des insignes), dash, scrap, liste des améliorations de la run (regroupées, ×n) avec rareté, catégorie et description.
- ✅ Onglet **Logs** : les 8 logs, ceux retrouvés sont relisibles en entier, les autres affichés « signal non capté ».
- ✅ Onglet **Quêtes** : objectif de l'étage (carte rouge, ascenseur), Ulysse et son équipage (insignes rendus / en poche, PV gagnés), la gamine (Lapin en main, ou issue), le Cobaye, archives (logs x/8). Une quête n'apparaît qu'une fois découverte.
- ✅ Police : ajout de « : » et de l'apostrophe (avant, ils étaient invisibles partout dans le jeu).
- À voir : C.O.M aussi consultable au Kibo ? Journal des quêtes terminées / historique des runs ?

## 19. v0.52.0 — étage 3 « Les labos » + boss « Le Chercheur » (30/10/2026)
- ✅ Étage 3 : ambiance « Labos » (85 % des salles), carrelage clinique, néon rouge. Mobilier : tables d'opération (3 états), silos de test cryo (occupé / vide / brisé, réf. capsules d'essai d'Akira), quelques cuves de spécimens. Pose sans jamais couper un passage.
- ✅ Dialogue d'arrivée de l'IA (« Expériences et personnel scientifique, accès restreint »).
- ✅ Boss **Le Chercheur** (300 PV) : il accueille Jira dans la salle de l'ascenseur (dialogue), puis le remède du docteur qu'il s'est injecté prend le dessus et son bras droit enfle en masse de chair qui avale le métal (réf. Tetsuo qui perd le contrôle).
  - Phase 1 : balayage (cône rouge) et écrasement (cercle au sol + anneau de tirs).
  - Phase 2 (66 %) « le bras grossit » : bras qui s'étire en ligne droite, jet de débris.
  - Phase 3 (33 %) « perte de contrôle » : chair au sol qui ralentit et blesse, le bras frappe au hasard, attaques plus rapides.
  - Après sa mort, l'IA : « Le remède du docteur. Quelqu'un continue de le produire, plus haut. »
- ✅ Banc de mobs : bouton « Le Chercheur ».
- ⚠️ Lore : le Chercheur est un **assistant du docteur**, pas le doc (qui reste le boss final). À valider, ainsi que son nom.
- À voir : étage 4 = mélange des ambiances + Tentaculaire (comportement actuel). Ennemis propres aux labos ? Combien d'étages pour la démo ?

## 20. v0.53.0 — l'étage 3 « fait labo de recherche » (retour Sabri, 30/10/2026)
- Retour : pas assez « labo de recherche », colorimétrie trop proche de l'étage 1, il manque des écrans de recherche.
- ✅ Colorimétrie : la seule ambiance claire du jeu (blanc clinique, gris-bleu, écrans bleus, obscurité réduite). 100 % des salles de l'étage 3 (avant : 85 %).
- ✅ Mobilier de recherche : postes à 2 écrans animés (ADN, électrocardiogramme, histogramme, données, scan), écrans muraux sur le mur nord (aussi au C.O.M, au marchand et dans la salle du boss), analyseurs, étagères de bocaux de spécimens. Moins de silos (un seul, et seulement dans les grandes salles), moins de cuves.
- ✅ Dialogue d'arrivée : « Les postes de recherche tournent encore. Les analyses du docteur n'ont jamais été arrêtées. »
- À voir en jeu : lisibilité des tirs cyan sur sol clair.

## 21. v0.54.0 — étage 4 « Les serres » (30/10/2026)
- ✅ Un étage pour souffler : 6 ou 7 salles seulement, pas de carte rouge, ni corridor ni faille. Des combats (1 vague, moitié moins d'ennemis), et un cœur offert dans chaque salle calme.
- ✅ Ambiance verte et lumineuse : herbe, fleurs, arbres (vert, fruitier, muté), bacs de culture, hydroponie.
- ✅ Pas de boss : la salle de l'ascenseur a un grand saule pleureur et une vieille dame en blouse assise sur un banc, hagarde, avec un port cortical à la tempe et un câble coupé. Le dialogue démarre quand on s'approche, puis l'ascenseur s'ouvre. Ensuite « [C] Parler » pour quelques phrases de plus.
- ✅ Elle n'est pas nommée (« La vieille dame »). Indices : « quelque chose à... brancher », « elle aimait s'endormir dessous », « tu as ses yeux », l'IA dont les données sur elle sont « corrompues », « on m'a dit d'attendre ici. Il reviendra me chercher » (elle n'est pas venue seule).
- ✅ C.O.M : objectif propre à l'étage 4 et quête « La vieille dame ».
- ⚠️ Lore : voir LORE_ACTUEL (Jira clone de la fille ? la scientifique vivante ?).
- À voir : l'étage 5 reprend aujourd'hui un mélange des ambiances et le Tentaculaire.

## 22. v0.55.0 — le C.O.M au centre de l'étage (Sabri, 30/10/2026)
- ✅ Le C.O.M arrive au centre de chaque étage : une sphère façon gacha ball cassette futurism (doc « Informations du projet ») sur une plateforme circulaire. 4 passerelles mènent aux 4 portes, au-dessus du vide où défilent les entrailles de la station (réf. le vaisseau de Matrix).
- ✅ La salle du gardien est toujours collée au C.O.M, sur un des 4 points cardinaux. Sa porte côté C.O.M est verrouillée (balise rouge) tant qu'on n'a pas la carte.
- ✅ Elle a une seule autre entrée, au bout d'un chemin qui ne passe pas par elle, et la carte rouge est sur ce chemin (dans la salle juste avant l'autre entrée, ou celle d'avant). Pas de retour en arrière : carte, gardien, puis la porte d'à côté ramène au C.O.M.
- ✅ Plus d'ascenseur dans la salle du gardien : une fois le gardien tombé, « Retourne au C.O.M ». Le terminal a un 4e onglet **Descente** avec un levier : verrouillé (il indique où est le gardien et si on a la carte) ou prêt (Entrée / clic / A). La sphère s'emballe, les entrailles défilent, étage suivant.
- ✅ Étage 4 (serres) : pas de carte, le saule est collé au C.O.M ; le C.O.M est prêt après la discussion.
- ✅ Testé : 600 étages générés (étages 1 à 4), 0 problème (C.O.M à 4 portes, gardien collé, 2 entrées, carte atteignable sans passer par le gardien, gardien atteignable depuis la carte sans repasser par le C.O.M, tous les rôles présents).
- À faire (doc) : les autres interactions du C.O.M en leviers, capsules et symboles : choisir l'ultime, orienter la run (N/S/E/O), modulateurs, accepter ou refuser des mutations.
- ⚠️ Lore : « le C.O.M est le Kibo » → voir LORE_ACTUEL.
- ✅ v0.55.1 : au départ, la passerelle du hub mène au **C.O.M à quai** (C.O.M = Kibo : seuls les noms ont évolué dans la doc). L'IA dit « Connexion au C.O.M établie… », le levier est déjà prêt, et on descend à l'étage 1. La porte sud ramène au hub.
- ✅ v0.55.2 (bug signalé par Sabri, 01/11/2026) : la porte du gardien s'ouvre dès qu'on ramasse la carte rouge. Avant, quand la carte était dans la salle collée au gardien, il fallait ressortir et revenir.

## 23. v0.56.0 — passerelle infestée et tracé moins répétitif (01/11/2026)
- Constat (2 000 étages générés en v0.55) : l'autre entrée du gardien était toujours dans son dos (100 %), et la carte toujours juste derrière lui ou dans la salle d'avant. Trop prévisible.
- ✅ **Passerelle infestée** (idée Sabri) : la passerelle du C.O.M vers le gardien est envahie par la mutation. C'est elle qui a arrêté le C.O.M à cet étage. Côté gardien, la porte est murée par la même chair qui bat. Infranchissable. À la mort du gardien, la chair se rétracte et on rentre directement au C.O.M. Petits dialogues au premier contact.
- ✅ **Tracé** : les portes suivent l'arbre de génération, avec quelques boucles. Deux salles voisines ne communiquent plus forcément. L'autre entrée du gardien est derrière lui (≈ 65 %) ou sur un côté (≈ 35 %).
- ✅ **Carte** : n'importe où sur le chemin (de 0 à 8 salles avant le gardien) ou dans une petite branche à côté.
- ✅ Testé : 3 000 étages (aucun problème de connexion, la carte toujours atteignable sans passer par le gardien), boucles dans environ la moitié des étages. En jeu : passerelle bloquée, porte murée, rétractation, retour au C.O.M.
- Étage 4 (serres) : pas d'infestation, le saule reste ouvert.

## 24. v0.56.1 — cœurs et suite de la quête d'Ulysse (retour Sabri, 01/11/2026)
- ✅ Avec tous ses PV, on ne ramasse plus les cœurs (et ils ne sont plus aimantés). Ils restent dans la salle pour plus tard.
- ✅ La quête d'Ulysse est suivie membre par membre. Le C.O.M affiche :
  - « [X] Ulysse sauvé » ;
  - puis chaque membre : insigne rendu, insigne en poche à rendre au Kibo, ou « ??? quelque part dans les étages » ;
  - le total rendu / 6 et les PV gagnés.
- ✅ Le « 1 / 6 » venait de l'insigne gagné en sauvant Ulysse. C'est en fait celui du quartier-maître (l'élite de sa salle), et il est maintenant nommé ainsi.
- ✅ Un membre déjà trouvé ne revient plus dans une salle Événement (avant, le nom était tiré au hasard et pouvait se répéter). Une fois tout l'équipage trouvé, la salle garde un « survivant muté » sans insigne.
- ✅ Les sauvegardes existantes sont reprises (les compteurs sont convertis en membres nommés).
- ✅ v0.56.2 (Sabri, 01/11/2026) : façon BotW, il faut **3 insignes rendus pour +1 PV max**, donc +2 au maximum avec les 6 membres (avant : 1 PV tous les 2 insignes, +3 au maximum). Le C.O.M affiche « prochain PV : x / 3 insignes ». Les sauvegardes existantes sont recalculées.

## 25. v0.57 → v0.59 — objets, cassettes, events (Sabri, 01/11/2026)
**v0.57.0 — les objets de la branche greffes rejoignent le proto**
- Pour tous : Aimant (ramasse de plus loin), Kit de soin (+3 PV), Plaque de blindage (+2 PV max, −6 % vitesse), Catalyseur ADN (+25 %), Récupérateur (plus de scrap).
- Greffes à projectiles (Standard, Rafale, Shotgun) : Ricochet, Balles perforantes, Double canon.
- Griffe du Cobaye : Bras long, Prise, Fauche, Rayon latent, Vorace, Double griffe.
- Un perk lié à une greffe ne sort que si cette greffe est active ; la carte de niveau l'indique.
- Remarque : l'Aimant et le Ricochet n'avaient jamais été dans le proto principal, seulement dans la branche greffes.

**v0.58.0 — les Données et la Cassette Shield (GDD Items v0.1)**
- **Données** : la monnaie permanente, gardée même si on meurt. Les élites en lâchent 2, les gardiens 12. Nouvelle **Disquette Données** (+30 %). Le compteur s'affiche sous le scrap.
- **Cassette Shield** :
  - NV1 trouvée en run, puis gardée d'une run à l'autre : on démarre chaque run avec. Elle absorbe 1 coup et revient à l'étage suivant. Une fois débloquée, elle ne sort plus dans le butin.
  - NV2 (30 Données) : se recharge après 10 s sans dégâts.
  - NV3 (60 Données), au choix et pour toujours : **A nettoyant** (à l'impact, onde qui blesse et efface les tirs proches) ou **B miroir** (les tirs ennemis proches repartent vers l'envoyeur).
- Achat au **Kibo** : onglet **Cassettes** du terminal, C.O.M à quai. En run, l'onglet se consulte mais n'achète pas.
- Les cassettes Mobilité et Cobra apparaissent « à définir (GDD) ».

**v0.59.0 — trois events du doc « Events et PNJ »**
- **Trésor** (cul-de-sac, ~70 % des étages, 50 % aux serres), deux variantes :
  - *stock* : tas de scrap, Données et une disquette sur piédestal. 4 fois sur 10, prendre la disquette réveille un « dormeur des pièces » (élite) ;
  - *carnage* : des corps, de l'ADN gratuit, de l'énergie.
- **Irruption** (remplace un combat, ~50 % des étages 1 à 3) : une cuve est vide, les lumières grésillent puis lâchent. Il ne reste que la torche de Jira. La chose rôde (deux yeux rouges), s'arrête, puis charge. **Un coup suffit, pour elle comme pour toi** : un contact tue, sauf bouclier ou Renaissance. La lumière revient quand elle meurt, avec un objet rare et des Données.
- **Le cartographe** (cul-de-sac, ~50 % des étages 1 à 3) : un gamin des conduits, casquette et lunettes. Il faut le protéger pendant 2 vagues (6 PV, une partie des mutants le visent). S'il survit, il donne la **carte complète de l'étage** (toutes les salles et leur rôle). Sinon, rien.
- Testé : 600 + 3 000 étages générés sans problème ; les trois events joués de bout en bout en automatique.

**Reste du GDD (liste du 01/11), pour plus tard**
- Purge et Régénération (prévues ensuite).
- Effet élémentaire. Perks Vitesse de déplacement + et Recharge dash +.
- Échos qui changent les règles : Trèfle à 4 feuilles, Attracteur de loot, Filtre de spawn, Mémoire du clone.
- Cassettes Mobilité et Cobra (à définir).
- Le clone mort qui revient muté.
- Les autres interactions du C.O.M : ultime, orientation, modulateurs, mutations.
- Codex.
- Events : « Le doc » (à renommer : c'est le nom du boss final), Droïde, Becoming human, Puits, Mutant pacifique, Agglomérat, Chamboulement, branche « on ignore la gamine ».
- Boss de l'étage 5.
- ✅ v0.59.1 (Sabri, 01/11/2026) : **écran d'accueil**. Si une progression existe dans le navigateur (Ulysse au Kibo, Cobaye, insignes, logs, cassettes, Données…), le jeu propose **Continuer** ou **Nouvelle partie**. Nouvelle partie efface tout et relance le réveil. Il faut appuyer deux fois pour confirmer. Sans progression, le jeu démarre directement. La progression est propre à chaque navigateur : un testeur qui ouvre le lien pour la première fois part toujours de zéro.

### v0.59.2 — invites manette Xbox (01/11/2026)
- `inputMode` bascule tout seul : clavier / souris -> invites clavier ; bouton ou stick manette -> invites Xbox. Helper `KB(clavier, manette)`.
- Couvert : HUD (Onde [RB], Holo [X], Ultime [Y]), C.O.M ([B]), pieds du terminal (croix / A / B), dialogues (A suite, B passer), titre, pause (Menu), mort ([A]), level-up, hub ([B] Parler / Incarner), salle d'entrainement (croix G/D greffes).
- Ajouts manette : B interagit partout et passe les dialogues ; croix G/D change de greffe en jeu.
- A faire si besoin : icones de boutons colorees (PNG) a la place du texte [A]/[B]/[X]/[Y].
- v0.59.3 : icones de boutons dessinees (placeholder code) a la place du texte. PNG `boutons_manette.png` 12x9, 11 frames (A B X Y LB RB LT RT Menu Vue Croix). Jetons `{A}` `{CROIX}`... utilisables dans tout texte du jeu.

## 26. Salles à condition (idée Sabri, 03/11/2026) — à écrire plus tard
On ne fait pas de salles secrètes à la Isaac (trop copié). À la place : des **salles à condition**, visibles mais fermées tant que la condition n'est pas remplie.
- **Référence : Cyberpunk 2077.** Certaines portes et certains butins demandent un attribut précis (Corps pour forcer, Capacités techniques, Intelligence). Chez nous, la mutation, l'humanité ou le perso joue ce rôle de clé.
- **Conditions possibles** : un niveau de mutation minimum (ex. stade 3), ou au contraire être sans mutation (humain), ou jouer un autre perso (le Cobaye, une autre Jira).
- **Ce qu'elles donnent** :
  - du narratif nécessaire pour finir le jeu (vraie fin) ;
  - un **objet unique**, disponible uniquement après avoir fait cette salle (ensuite dans le pool ? à préciser).
- **Première salle envisagée** : à l'étage 4 (les serres). Si on est complètement muté, on ne peut pas entrer : il faut être humain.
- Statut : ⚠️ aucune salle écrite pour l'instant. À trancher :
  - la porte se lit comment (couleur, icône de condition) ?
  - la condition se vérifie à l'entrée seulement, ou tout le long de la salle ?
  - l'objet unique entre-t-il ensuite dans le pool normal ?
  - où apparaissent ces salles (une par étage, au hasard) ?

## 27. v0.60.0 — déblocages entre les runs (retour gamefeel de Sabri, 03/11/2026)
Retour : il faut des objets, voire des greffes, qu'on débloque après plusieurs runs, comme dans Isaac ou Vampire Survivors.
- Les stats sont cumulées sur toutes les runs (`META.st`) : runs, morts, kills, étage max, stade de mutation max, boss vaincus par étage, boss vaincus avec le Cobaye.
- Une greffe ou un objet verrouillé n'apparaît nulle part (touches 1-5, socles, marchand, niveaux) tant que son objectif n'est pas rempli.
- Au départ, Jira n'a que **Standard et Rafale**.
- Le débloquage s'affiche en encart « DÉBLOQUÉ », est récapitulé sur l'écran de mort, et toute la liste est dans C.O.M > Quêtes > Déblocages (les objectifs restants s'affichent en « ??? : condition »).
- Le panneau dev a un bouton « Tout débloquer (test) ».

| Déblocage | Objectif |
|---|---|
| Perso : le Cobaye (v0.60.1) | Vaincre un gardien |
| Greffe Laser | Atteindre l'étage 2 |
| Double canon | Faire 3 runs |
| Cassette Shield (v0.60.1) | Atteindre l'étage 3 |
| Greffe Lame | Éliminer 150 ennemis au total |
| Catalyseur Berserk | Atteindre le stade 3 de mutation |
| Greffe Shotgun | Vaincre Mechakuve (étage 2) |
| Plaque de blindage | Rendre 3 insignes à Ulysse |
| Renaissance | Mourir 5 fois |
| Régulateur | Vaincre Le Chercheur (étage 3) |
| Charge de dash + | Atteindre les serres (étage 4) |
| Double griffe | Vaincre un gardien avec le Cobaye |

À valider : le rythme (trop rapide, trop lent ?), le choix des objets verrouillés, et les déblocages de persos ou de cassettes qu'on voudra ajouter. Les salles à condition (§26) pourront aussi donner des déblocages.

- v0.60.1 : les persos et les cassettes passent aussi par les déblocages (le Cobaye, la cassette Shield). Chaque déblocage affiche son type (Perso / Greffe / Cassette / Objet).

## 28. Objets actifs et objets passifs (Sabri, 03/11/2026) — à réfléchir
Les cassettes font partie des objets, mais ce sont **les objets activables**. Ça permet de séparer clairement deux familles (comme les objets actifs et passifs d'Isaac).
- **Passifs** (toujours actifs, rien à appuyer) : disquettes, perks, échos, greffes.
- **Actifs** (on les déclenche) : les cassettes.
- **Aujourd'hui dans le proto** : la cassette Shield est encore passive (elle absorbe un coup toute seule, et le NV3 se déclenche quand elle casse). Il faudra la rendre activable.

Pistes à trancher :
- **Touche dédiée** : Q au clavier, LB à la manette (LB et RB font tous les deux l'onde aujourd'hui, on libère LB).
- **Recharge** : au temps, ou par salles nettoyées comme Isaac (barre de charges) ?
- **Emplacements** : 1 seule cassette active à la fois (on échange au C.O.M), ou 2 ?
- **Shield en actif** : un bouclier de 2 s quand on appuie, et les NV2/NV3 en variantes (reflet, onde de purge) ?
- **Affichage** : un emplacement de cassette dans le HUD (icône + jauge de recharge) et l'étiquette ACTIF / PASSIF sur les cartes d'objets.

## 29. Trois sorts par perso, à débloquer (Sabri, 03/11/2026) — à réfléchir
Sabri n'est pas sûr de garder l'onde et l'ultime dès le départ. Piste : **des emplacements de sorts à débloquer**, pour se construire un build.
- **3 sorts par perso** :
  1. l'attaque de base (le mode de tir, la greffe) ;
  2. le coup spécial (l'onde aujourd'hui) ;
  3. l'ultime.
- On commence avec l'attaque de base seule. Le spécial et l'ultime arrivent via les déblocages (§27) ou en cours de run.
- Peut-être des **améliorations par sort** (comme les perks liés à la greffe aujourd'hui).
- Référence proche : Hades (attaque, spécial, sort, et un appel qui se charge comme notre ultime). Ça colle au positionnement.

À trancher :
- **L'hologramme (F / X)** : 4e sort, ou il devient une cassette, donc un objet actif (§28) ?
- Le spécial et l'ultime se débloquent pour de bon (méta), ou se trouvent à chaque run ?
- ~~Chaque perso a son propre set de sorts~~ → remplacé le 06/11 par le principe §31 (un perso = un kit de départ ; sorts communs, variété par les greffes). Ancienne note : ✅ **Chaque perso a son propre set de sorts** (Sabri, 03/11/2026) : sa propre attaque de base, son propre spécial et son propre ultime. Reste à écrire le set du Cobaye (la griffe en attaque ; le rayon, la fauche ?) et ceux des futures Jira.
- Les améliorations de sort : des perks dans le choix 1 parmi 3, ou des achats au C.O.M avec les Données ?
- Dans le proto aujourd'hui : l'onde, l'holo et l'ultime sont tous dispo dès le départ. Rien n'est changé tant que ce n'est pas tranché.

## 30. Réponses de l'équipe aux points à trancher (06/11/2026)
Source : `DOC\GDD\EON_Reponses_questions_a_trancher_06112026.pdf` (réponses de l'équipe + réponses de Sabri), en réponse au PDF « Points à trancher » du 03/11.

**Décidé**
| # | Sujet | Décision |
|---|---|---|
| 1.2 | Hologramme | On le garde de base pour les tests, puis il devient une cassette après les tests de level design (trop fort de base). |
| 1.3 | Améliorer les sorts | Par des choix en run (façon Vampire Survivors / Hades), pas par des achats au Kibo. |
| 1.4-1.7 | Persos | Un seul perso, **Jira**, avec des greffes. Plus de termes cobaye / ingénieur / xéno. En pixel, le Cobaye devient une autre version de Jira. La 3D reste sur Jira seule. |
| 2.1 | Touches | Les binds actuels conviennent. |
| 2.2 | Pourquoi activer | On devient trop fort trop vite (l'ultime se spamme à chaque salle) : il faut des contraintes d'activation. |
| 2.3 | Emplacements de cassette | 1 de base. Le 2e se débloque en run, par exemple en récompense d'une quête secondaire. |
| 2.4 | Shield | Actif, en parade avec timing (comme le bouclier de Sivir dans LoL). À tester in game. |
| 2.5 | Usure des cassettes | Mis de côté (inventaire trop lourd pour le rythme). |
| 2.6 | Cassette Mobilité | Vitesse de déplacement ou % d'esquive. (Cobra : à définir.) |
| 3.2 | Greffes au départ | Jira démarre avec **une seule** greffe. |
| 4.2 | Salles à condition | Zone protégée par des survivants qui te chassent au moindre signe de mutation. |
| 4.3 | Objet de la salle | Exemple : une passe récupérée à l'étage d'avant. |
| 4.4 | Lecture | Une indication, mais pas trop précise (façon Resident Evil). |
| 5.x | Lore | Voir `LORE_ACTUEL.md` §9 (clone de la fille acté, titre EON, etc.). |
| 6.1 | PV | Trop de PV au début. Sabri : **3 cœurs au départ**. Les mobs devront avoir des étages de spawn (ex. Méduse seulement à l'étage 2). |
| 6.3 | Chutes | On garde les dégâts de chute. Ne pas réapparaître juste au bord du trou. Varier les dangers d'étage en étage pour que les trous ne deviennent pas redondants. |
| 6.4 | Données | Elles servent à garder une partie du scrap d'une run à l'autre (comme dans Isaac). |
| 7.1 | Étages | 4 étages pour le jeu final. La démo = **1 étage**. Un étage bonus possible (peu coûteux, events ou narratif). |
| 7.2 | Fin | Un dernier étage avec seulement la salle du boss final (comme la fin de Hades). |
| 7.3 | Tentaculaire | On le garde (boss de l'étage 1), il faudra le dessiner. |
| 7.4 | Agglomérat | Boss ou mini-boss : plusieurs monstres fusionnés en un plus gros. Méduse et Brainless n'ont jamais été dessinés. |
| 7.5 | Events | Garder ceux qui n'amènent pas trop d'éléments roguelite ; trier et simplifier (faisabilité). |
| 7.6 | Purge / Régénération | Personne ne s'en souvient : abandonnées pour l'instant. |

**À réfléchir (pas encore tranché)**
- 3.1 Rythme des déblocages : il faut jouer plus.
- 6.2 Irruption : pas de réponse (l'équipe ne connaissait pas l'event).
- 6.5 Trop de salles cadeaux : les rendre rares. Quand c'est gratuit, justifier pourquoi les 2 autres objets disparaissent. Élites seulement après un certain nombre de salles ; une pelletée d'ennemis est plus dure qu'un élite seul.
- Doc demandé par l'équipe : un doc très clair sur perks, greffes, cassettes, disquettes, ADN (ce que c'est, comment on l'obtient).

**⚠️ Contradictions à trancher avant d'intégrer**
1. **Glossaire de Sabri vs proto.**
   - Sabri : Perks = bonus de stats (vitesse, vie, force, chance…) ; Disquettes = améliorent les perks (les pills d'Isaac) ; Cassettes = sorts activables (Onde, Holo, Shield…).
   - Proto : les disquettes sont les bonus de stats (+PV, +vitesse, +dégâts), les perks sont des effets d'arme (ricochet, perforation…), l'onde est un sort de base.
   - Il faut choisir les définitions avant d'écrire le doc pour l'équipe.
2. **L'onde : sort de base ou cassette ?** L'équipe (1.1) : le spécial et l'ultime sont là dès le départ et se boostent en run. Le glossaire de Sabri met l'Onde dans les cassettes. La §29 parlait d'emplacements de sorts à débloquer.
3. **§29 « chaque perso a son propre set de sorts »** vs **« un seul perso, Jira »** (1.4-1.7). Si les variantes sont des greffes, le set de sorts serait-il lié à la greffe ?
4. **Le Cobaye** : Sabri (6.1) dit « même greffe, même gameplay, même anim ». Dans le proto, il a la griffe et son propre gameplay, et il se débloque comme un perso (v0.60.1).
5. **3 cœurs** : 3 PV, ou 3 cœurs coupés en deux (= 6 demi-PV, comme aujourd'hui à l'écran) ?
6. **Insignes** : l'équipe trouve « 2 insignes = 1 PV » généreux. Le proto est déjà à 3 insignes = 1 PV. Garder 3, ou passer plus haut ?

## 31. Principe persos / greffes pour la 3D (Sabri, 06/11/2026) ✅
Contrainte : le jeu final est en 3D. Pour que ça ne coûte pas trop cher, chaque perso doit au minimum pouvoir utiliser tous les perks et tous les objets. Des gameplays trop différents deviennent des contraintes. Vampire Survivors y arrive parce que ses persos sont des **kits de départ**, pas des gameplays à part (Isaac pareil ; Hades fait varier l'arme, pas le perso).

**Règle : un perso = un kit de départ, jamais un système à part.**
- **Une seule Jira** : un squelette, un set d'anims de déplacement / dash / dégâts. C'est le gros du coût 3D, payé une fois.
- **Une variante de Jira** = une greffe de départ + un passif (ex. « mute plus vite, frappe plus fort ») + des stats de départ + un skin (texture, accessoires, couleurs).
- **Le gameplay différent vient des greffes.** Chaque greffe = un modèle de bras + ses anims d'attaque (haut du corps), utilisables par toutes les Jira.
- **Perks et objets** : ils agissent sur des stats ou des règles générales (dégâts, cadence, énergie, mutation, dash), donc sur toutes les greffes. Les rares perks propres à une greffe ne sortent que si elle est équipée (déjà le cas dans le proto). **Jamais de perk propre à un perso.**
- **Sorts** : l'onde et l'ultime sont communs à toute Jira, boostés en run ; la greffe peut éventuellement les colorer.
- Coût d'une nouvelle Jira : un skin, un passif, un équilibrage. Le vrai budget va dans les greffes, qui profitent à tous.

**Ce que ça règle dans les contradictions du §30**
- Point 3 : plus de « set de sorts par perso » (la §29 est remplacée sur ce point) ; la variété passe par les greffes.
- Point 4 : en 3D, le Cobaye = une Jira avec un skin, la griffe en greffe de départ et un passif lié à la mutation. En pixel, il reste jouable tel quel pour tester ; ce que le pixel valide (ex. une griffe fun) devient une greffe pour toutes.

**Conséquences pour le proto (à faire)**
- Rendre la griffe utilisable par Jira, et le Cobra par le Cobaye (aujourd'hui chacun est limité à sa greffe).
- Enlever les objets réservés à un perso (ex. « Double griffe » liée au Cobaye) : ils deviennent liés à la greffe.
- Donner au Cobaye un passif de mutation au lieu d'un gameplay à part.
