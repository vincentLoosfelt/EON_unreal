# 09 — Ce que le proto a apporté, et ce qui a été décidé depuis

Ce fichier sert à deux choses. Vincent y voit ce qui est **nouveau** par rapport à ce qu'il connaît du projet (le GDD et la version 3D). Et son Claude y trouve les décisions prises après la v0.60.1, à **ne pas** intégrer sans feu vert.

L'état exact du projet Unity n'est pas pris en compte ici. La liste ci-dessous est donc : ce qui est né dans le proto, ou ce que le proto a tranché en le jouant.

## 1. Ce qui est né dans le proto

Ces mécaniques n'étaient dans aucun document avant d'être jouées.

| Apport | En quoi ça change le jeu |
|---|---|
| **Le vide** | Né en cherchant juste à varier les salles. Une règle, trois usages : un danger pour Jira, une **arme** (le recul y pousse les ennemis), et un passage que seul le dash franchit. C'est un des meilleurs leviers de game feel du proto. Il a depuis son propre GDD. |
| **Le C.O.M au centre de l'étage** | Il arrive au milieu, avec 4 passerelles. Le gardien est toujours collé au C.O.M, mais sa passerelle est **infestée** : on le voit, on sait où il est, et il faut faire le tour. La carte rouge est sur ce tour. Pas de retour en arrière : à la mort du gardien, la chair se rétracte et on rentre directement. |
| **Le levier de descente** | Il n'y a plus d'ascenseur chez le gardien. C'est le C.O.M qui descend. |
| **La course en maintenant le dash** | Façon Hades. Elle sert à traverser une salle vide sans marteler le dash. |
| **L'armure de mutation** | La mutation laisse une trace permanente : une armure qui bloque des coups mais coûte de l'énergie max. Le corps de Jira et son HUD de ciblage évoluent avec. |
| **La nuée des Nuke** | Elles se regroupent, débordent, et fuient quand elles sont seules. Le comportement était écrit dans le bestiaire, le proto l'a rendu jouable. |
| **Les déblocages entre les runs** | Greffes, objets, cassette et perso verrouillés par des objectifs cumulés, façon Isaac et Vampire Survivors. |
| **Le Kibo comme refuge** | Un PNJ sauvé en run (Ulysse) vient s'y installer pour de bon. Le modèle peut servir à d'autres personnages. |
| **Les Données et la cassette persistante** | Une seule monnaie permanente. La cassette trouvée en run reste acquise et s'améliore au Kibo. |
| **Trois events peu coûteux** | Trésor, Irruption, Cartographe : presque aucun asset nouveau, beaucoup de variété. |
| **L'étage sans gardien** | Les serres : un étage court pour souffler, qui se termine par une rencontre et non par un combat. |
| **Les invites qui suivent le périphérique** | Clavier ou icônes Xbox, selon la dernière entrée. |

## 2. Ce que le proto a tranché

| Sujet | Décision |
|---|---|
| Les monnaies | **ADN** = expérience (choix 1 parmi 3). **Scrap** = monnaie de la run. **Données** = seule monnaie permanente. |
| Le gardien de l'étage 1 | Le **Tentaculaire**. Mechakuve passe à l'étage 2. « Ubume » désigne le mob invocateur. |
| Les noms des mobs | Nuke, Onibi, Kama, Ikiryo, Ubume. Méduse et Brainless restent provisoires. |
| Les élites | Gardées. Une par salle au plus, puis 2 salles sans. |
| La Faille | Une salle de fuite avec une seule vague, pas une salle de combat avec un gimmick. |
| L'érosion de la chair | Testée comme mécanique (trouer l'ennemi pour le tuer), écartée. Gardée seulement comme effet visuel en pixel. |
| Les PV des fiches concept | Ne font pas foi. L'équilibrage du proto est le sien. |
| Le rythme narratif | Aucune scène ne bloque plus de quelques secondes. Les logs se lisent sans pause. |
| Le Cobaye | Une variante de Jira, pas un personnage à part. |

## 3. Décidé depuis, mais pas encore dans le proto

**À ne pas intégrer d'office.** Ces décisions datent du 06/11/2026 (réponses de l'équipe) et le proto v0.60.1 ne les applique pas. Pour que la comparaison soit honnête, le portage reproduit d'abord le proto tel qu'il est. Mais chaque point doit être **facile à changer** : une valeur dans les données, pas une règle en dur.

| Sujet | Proto v0.60.1 | Décision du 06/11 |
|---|---|---|
| PV de départ | 6 | « 3 cœurs ». À préciser : 3 PV, ou 3 cœurs de 2 demi-PV. |
| Greffes au départ | Standard et Rafale | Une seule |
| Les persos | Jira et le Cobaye, chacun avec ses greffes | Un seul perso, Jira. Une variante = un kit de départ (greffe, passif, stats, skin). Toutes les greffes pour toutes. Jamais de perk propre à un perso. |
| L'hologramme | Disponible dès le départ | Deviendra une cassette après les tests de level design (trop fort de base) |
| La cassette Shield | Passive : absorbe un coup toute seule | Active : une parade avec un timing. À tester en jeu. |
| Emplacements de cassette | Pas de notion d'emplacement | 1 de base, le 2e se gagne en run |
| L'ultime | Se recharge vite, utilisable presque à chaque salle | Trop fort : il faut des contraintes d'activation |
| Les mobs par étage | Tous partout, selon le budget | Des étages d'apparition (par exemple la Méduse seulement à l'étage 2) |
| Les salles cadeaux | Opportunité, Trésor, récompenses fréquentes | Les rendre plus rares |
| Les Données | Achètent les niveaux de cassette | Serviront aussi à garder une partie du scrap d'une run à l'autre |
| Les étages | 4 jouables, plus un mode sans fin | 4 pour le jeu final. La démo : 1 seul. Un dernier étage avec seulement le boss final. |
| Les insignes | 3 insignes = +1 PV | L'équipe trouvait « 2 insignes = 1 PV » généreux. Le proto est déjà passé à 3. |
| Purge et Régénération | Absentes | Abandonnées pour l'instant |

## 4. Idées notées, pas encore écrites

Pour mémoire. Rien à coder.

- **Les salles à condition** : visibles mais fermées tant qu'une condition n'est pas remplie (un stade de mutation, ou au contraire être « humaine »). Référence : les portes à attribut de Cyberpunk 2077. Première envisagée : aux serres, une zone tenue par des survivants qui chassent Jira au moindre signe de mutation.
- **Objets actifs et passifs** : les cassettes seraient les objets qu'on déclenche, tout le reste serait passif.
- **Trois sorts à débloquer** : attaque de base, spécial, ultime, améliorés par des choix en run.
- Les comportements du GDD pas encore portés : la charge imparable du Kama, les grenades de l'Ikiryo, la résistance de l'Onibi, les zones d'apparition, le bouclier à casser des élites.
- L'Agglomérat (plusieurs monstres fusionnés) comme boss ou mini-boss.
- Le clone de la run précédente qui revient muté.
- Le Codex.

## 5. Contradictions encore ouvertes

Elles concernent Sabri et l'équipe, pas le portage. Mais si le Claude de Vincent tombe dessus, il saura que ce n'est pas une erreur de sa part.

1. **Le glossaire.** Dans le proto, les *disquettes* sont des bonus de stats et les *perks* des effets d'arme. Sabri décrit l'inverse (perks = stats, disquettes = améliorations de perks), et range l'onde dans les *cassettes*. Les noms des catégories peuvent donc changer : garde-les dans les données, pas dans le code.
2. **L'onde** : un sort de base, ou une cassette ?
3. **Les noms** : reste-t-on sur du japonisant pour tous les mobs, Méduse et Brainless compris ?
4. **La vieille dame** : dans le proto, c'est la scientifique, vivante mais diminuée. Le lore dit qu'elle s'est sacrifiée. Non bloquant.

## 6. Ce que l'expérience du proto a appris

Trois leçons utiles pour le portage.

- **Un test automatique prouve qu'un système fonctionne, jamais qu'il est bon.** Une régression d'équilibrage (les events avaient mangé les salles de combat) n'a été vue que par Sabri en jouant. Vincent doit jouer chaque jalon.
- **Mesurer plutôt que deviner.** Le générateur d'étage a été corrigé en générant des milliers d'étages et en comptant. Fais pareil.
- **Un outil qui ne connaît pas le projet invente.** Le proto a ajouté un jour un mob qui n'existait dans aucun document, et il a fallu le retirer. Quand il manque une information, demande.
