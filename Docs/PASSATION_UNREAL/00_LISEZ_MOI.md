# EON — passation du proto pixel vers Unreal

**Pour : Vincent (et son Claude). De : Sabri. Date : 13/11/2026. Proto de référence : v0.60.1.**

## Le but

On refait le proto pixel **en 3D dans Unreal, depuis zéro**, sans rien reprendre du projet Unity. Le proto pixel nous plaît tel qu'il est : c'est lui la référence. Une fois la version Unreal jouable, on compare les deux côte à côte.

Ce dossier contient tout ce qu'il faut pour que ton Claude puisse le faire : les règles du jeu, tous les chiffres, la marche à suivre, et la liste de ce qui existe ou manque en 3D. Il ne contient **pas** les modèles, anims et textures : tu les as déjà (dossier `3D`).

## Ce qu'il y a dans le dossier

| Fichier | À quoi il sert |
|---|---|
| `CLAUDE.md` | Les consignes pour ton Claude. À copier à la racine du projet Unreal. |
| `01_VISION_ET_PERIMETRE.md` | Ce qu'on refait, l'échelle pixel → Unreal, la caméra, ce qui compte comme « pareil que le proto ». |
| `02_JOUEUR_ET_COMBAT.md` | Contrôles, déplacement, dash, greffes, énergie, mutation, sorts, game feel. |
| `03_ENNEMIS_ET_BOSS.md` | Les 7 mobs, les vagues, les élites, les 3 boss, phase par phase. |
| `04_ETAGES_ET_SALLES.md` | Génération d'étage, rôles de salle, le C.O.M, le Kibo, les events. |
| `05_PROGRESSION_ET_META.md` | ADN, scrap, Données, objets, déblocages, cassettes, quêtes, sauvegarde. |
| `06_UI_ET_DIALOGUES.md` | HUD, terminal du C.O.M, level-up, dialogues, carte. |
| `07_ASSETS_3D.md` | Quel fichier 3D sert à quoi, ce qui manque, et comment faire les placeholders. |
| `08_PLAN_DE_CONSTRUCTION.md` | La marche à suivre, jalon par jalon, avec quoi comparer à chaque étape. |
| `09_APPORTS_DU_PROTO.md` | Ce que le proto a apporté au projet, et les décisions prises depuis qui ne sont pas encore dedans. |
| `MANQUANTS.md` | La liste vivante de ce qui manque. Ton Claude la tient à jour. |
| `DONNEES/` | Les tables du proto en JSON (greffes, ennemis, objets, déblocages, salles, dialogues, réglages). Extraites du code, donc exactes. |
| `REFERENCE_PROTO/` | Le proto lui-même : la build joueur à ouvrir dans un navigateur, la build dev avec le panneau de réglages, et le code source. Plus le journal des versions du proto (`JOURNAL_DU_PROTO.md`) et la fiche de lore (`LORE_ACTUEL.md`), pour le contexte. |
| `PLACEHOLDERS_PIXEL/` | 79 PNG : tous les sprites, tuiles et éléments du proto, pour servir de placeholder dans Unreal. `_index.json` donne la taille et le nombre de frames de chacun, `_FORMAT_DES_PNG.md` dit ce que représente chaque frame. |
| `CAPTURES_PROTO/` | 17 captures du proto (chaque étage, chaque gardien, le C.O.M, le terminal, le level-up). |

## Comment t'en servir

1. **Joue au proto.** Ouvre `REFERENCE_PROTO/EON_v0.60.1.html` dans Chrome ou Edge (double-clic, rien à installer). La version `_DEV` a un panneau de réglages à droite et un bouton « Tout débloquer ».
2. Crée ton projet Unreal, mets ce dossier dedans (par exemple `Docs/PASSATION_UNREAL/`), et copie `CLAUDE.md` à la racine du projet.
3. Lance ton Claude dans le projet et dis-lui :

> Lis `CLAUDE.md` puis `Docs/PASSATION_UNREAL/08_PLAN_DE_CONSTRUCTION.md`. On refait le proto pixel EON en 3D dans Unreal. Commence par le jalon 0 et dis-moi ce que tu as besoin que je fasse dans l'éditeur.

4. Avance jalon par jalon. À la fin de chacun, joue les deux versions côte à côte.

## Ce que tu dois décider toi (ton Claude te posera la question)

- **La version d'Unreal.** Rien dans ce dossier ne dépend d'une version précise.
- **C++ ou Blueprints.** Le dossier conseille le C++ pour le gameplay, parce que Claude lit et écrit du texte et pas les fichiers `.uasset`. À toi de voir.
- **La caméra.** Le proto est en vue de dessus 2D. Le dossier propose une caméra plongeante fixe (façon Hades). Si vous aviez un autre angle sur Unity et qu'il vous plaisait, garde-le.
- **L'export des FBX manquants.** La Nuke, l'Onibi et le Kama n'existent qu'en `.blend` dans le dossier 3D (détail dans `07_ASSETS_3D.md`).

## Ce qui n'est pas dans le dossier

- Les modèles, rigs, anims et textures 3D (tu les as).
- Le GDD complet, la bible, le lore détaillé : ils restent dans le dossier du studio. Seul ce qui est **dans le proto** est décrit ici.
- Les sons : le proto synthétise tout par code. La liste des sons à produire est dans `MANQUANTS.md`.
