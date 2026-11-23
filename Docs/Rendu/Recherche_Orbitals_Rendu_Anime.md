# Recherche : rendu « animé rétro » façon Orbitals, pour EON (UE 5.8)

Version 1, 23/11/2026. Recherche web faite par Claude. Toutes les URL ont été consultées ce jour, sauf mention « extrait de recherche seulement ».
Règle suivie : rien d'inventé. Une information introuvable est signalée comme telle.

Non relu en détail ici, parce que déjà connu : le blog Unreal « Stepping inside a retro anime-inspired game: a look into the rendering of Orbitals » (J. Varmedal, 03/09/2026) et l'interview Inven Global. unrealengine.com, gamedev.net, GameSpot, Eurogamer, IGN, Polygon et ArtStation Magazine refusent les lectures automatiques (403 ou blocage). Leur contenu n'est connu que par des extraits de recherche ou des reprises.

---

## 1. Orbitals : sources techniques autres que le blog Unreal

### 1.1 Denfami Nico Gamer, interview des développeurs (LA source la plus utile après le blog Unreal)
- **Titre** : 『らんま1/2』『エヴァ』『ドラゴンボール』。日本アニメに魂を形作られた開発者たちが贈る、「希望の時代」へのラブレター『オービタルズ』開発者インタビュー
- **URL** : https://news.denfaminicogamer.jp/interview/2604233a
- **Date** : 23/04/2026. Interview menée par Ena (恵那).
- **Apports concrets** (extraits japonais vérifiés mot pour mot) :
  - **Jakob Lundgren** : « 動かない背景は手書きのペイントシェーダーで表現する ». Le décor fixe passe par un *paint shader* peint à la main, tout ce qui bouge par un *cel shader*. Changer la façon de dessiner les personnages et le décor change leur texture (« キャラと背景の描き方を変えることで、両者の質感が変わり »). Ce contraste sert aussi au gameplay : ce qui bouge est interactif.
  - **Jakob, sur l'éclairage** : « ゲームエンジンに演算させるのではなくて、直接光を書き入れたりもしています ». Une partie de la lumière est **peinte directement** au lieu d'être calculée par le moteur.
  - **Jakob** : tout ce qui est animé tourne à 12 ou 24 images/s.
  - **Marcos Ramos** : l'éclairage suit le sens artistique (« どう照らせば美しく見えるか ») plutôt que l'exactitude physique. Certaines étapes ont été faites au vrai crayon sur papier. Il a étudié les animés image par image.
  - **Johannes Varmedal** : le rendu simule en 3D la prise de vue au banc-titre des années 80. Il cite deux phénomènes physiques qu'ils recréent : « セルの影が背景にわずかに写り込んだり、光源の点滅が干渉し合ったり », c'est-à-dire **l'ombre du cellulo légèrement visible sur le décor** et des lumières qui clignotent en interférant. Il cite aussi « 中身のシェーディングが1コマごとに揺れたり » : l'ombrage interne des personnages **tremble d'une image à l'autre**, ce qu'ils simulent.
  - **Johannes, point capital pour EON** : « 遠景を筆で描いたようなタッチに見せる「桑原フィルター」というものがあるのですが、そのリアルタイム処理に苦労しました ». Ils utilisent bien un **filtre de Kuwahara (桑原)**, mais **seulement sur le lointain (遠景)**, pour lui donner une touche de pinceau. L'optimisation en temps réel a pris plus de six mois selon le résumé. Le Kuwahara n'est donc pas leur technique de décor principale : c'est un complément de distance, posé par-dessus le *paint shader*.
  - **Megumi Varmedal** : la charge de calcul en temps réel a été le grand obstacle. Avec Studio Massket, les assets sont partagés et la mise en page de la prise de vue est fixée dès le début.

### 1.2 Denfami Nico Gamer, table ronde avec Yohei Kameyama (Milky☆Subway)
- **URL** : https://news.denfaminicogamer.jp/interview/260902a
- **Date** : 02/09/2026
- **Apports** :
  - **Marcos, sur la palette** : couleurs légèrement pastel des grands animés, teintes **ternies et nostalgiques**, **fort contraste noir/blanc**, et refus des oranges vifs et des couleurs criardes. Il parle de « nostalgie de rêve ».
  - **Kameyama** juge la palette d'Orbitals plus claire et cohérente que celle des vrais animés rétro, qui sont souvent chaotiques. Il faut moderniser pour embellir.
  - **Jakob** : le but est de rendre une **sensation**, pas de copier fidèlement l'époque.
  - Les passages sur les cels, les décors et la pellicule ne sont pas dans le texte récupéré. Seuls les intertitres en parlent.

### 1.3 Post X d'@OrbitalsGame sur la fabrication des éléments de décor
- **URL** : https://x.com/OrbitalsGame/status/2088951935260823762 (lue via api.fxtwitter.com)
- **Date** : 16/08/2026
- **Texte** : « If you're curious about how we make background elements in Orbitals… 🖌️ »
- **Médias** : une vidéo 4K de 35 s (https://video.twimg.com/amplify_video/2088951770126925824/vid/avc1/3840x2160/7q5ah6IdqvsCw_zn.mp4?tag=29) et une image 1920×1080.
- **Ce que montre la vignette de la vidéo** (vue directement) : une **vraie peinture sur papier**, une bande allongée qui représente une surface de métal ou de couloir en violets et gris bleutés, avec de grands coups de pinceau plats. Autour : un **pinceau plat large de type hake**, deux pinceaux plats, une serviette, des **pots de peinture** et un carnet « KOWLOON NIGHTS ». Ce que cela suggère, sans preuve complète : des textures ou éléments de décor **peints à la main, physiquement, à la gouache ou à la poster color**, puis numérisés.
- **Non vérifié** : le reste des 35 s, notamment la numérisation, la projection sur la 3D et la résolution. La vidéo n'a pas été téléchargée, ce qui demanderait ton accord. **À regarder par Vincent.**
- **Image jointe au post** (vue directement) : les personnages ont un cel à 2 tons, des traits fins sombres et des **ombres portées au sol en aplats sombres simples**. Derrière, la machinerie est peinte avec des coups de pinceau visibles, des reflets d'arêtes clairs, moins de saturation et un peu plus de flou que les personnages. Le fond est une aurore en dégradé vertical. On voit aussi un léger vignettage, du grain et une légère frange chromatique.

### 1.4 80 Level, « This Co-Op Game Feels Like You're Playing Retro Anime »
- **URL** : https://80.lv/articles/this-retro-anime-style-2-player-co-op-game-has-hand-drawn-backgrounds
- **Date** : 18/08/2026, par Amber Rutherford
- **Apports** : l'article renvoie au post X ci-dessus sans le décrire. Ramos y dit que l'équipe aime « making things with our own hands and getting dirty », et qu'elle **n'utilise aucune IA générative**. D'après l'article, Hidetoshi Yoshida a fait tout le storyboard à la main. Un autre post X montre Ramos dessinant Maki et Omura (https://x.com/OrbitalsGame/status/2083539569941553627, 01/08/2026, vidéo de 8 min 42).

### 1.5 Blog officiel orbitalsgame.com, « Hello from the team behind Orbitals »
- **URL** : https://www.orbitalsgame.com/post/hello-from-the-team-behind-orbitals
- **Date** : 18/03/2026 (mis à jour le 28/04)
- **Apports concrets, les outils** :
  - **Décors peints dans Rebelle** (Escape Motions), un logiciel qui simule l'aquarelle et la gouache avec écoulement et papier.
  - Personnages dans **Procreate**, animation 2D et VFX dans **Blender**, montage dans Resolve.
  - Process « tradigital » : crayon et papier d'abord, numérique ensuite (Ramos).
  - Megumi : « emotion and expression over perfect linework and consistency ».
  - Johannes : équilibrer niveau de détail et couleurs pour un rendu pictural « 2D fait main » en pleine 3D. Travail sur la **correction gamma** pour que le jeu soit juste sur tout écran.
  - Taliesan Arnold (lead animator) : les méthodes d'animation des années 80 ont été étudiées et transposées en 3D.

### 1.6 Image d'en-tête Unreal du blog (post X @UnrealEngine)
- **URL** : https://x.com/UnrealEngine/status/2095543624046203354, 03/09/2026. Image : https://pbs.twimg.com/media/HRTd0a2bUAAZeJu.jpg
- **Ce qu'on y voit** (vu directement) :
  - Le **décor a lui aussi des contours** : traits sombres irréguliers sur les structures métalliques et les astéroïdes, plus fins et plus cassés que ceux des personnages.
  - Les flammes du réacteur sont des **cels VFX à bords durs** en jaune-vert, avec un halo lumineux autour.
  - Le laser a un cœur saturé et un halo.
  - Le vaisseau est en cel 2 tons bleus, avec des traits sombres et des reflets d'arêtes.

### 1.7 Nintendo « Creator's Voice » et Siliconera
- **Vidéo** : https://www.youtube.com/watch?v=REewmdZ65XA (Nintendo of America, 12/08/2026, selon Wikipédia). Siliconera donne aussi https://www.youtube.com/watch?v=KjZUGfIwSKQ.
- **Article** : https://www.siliconera.com/orbitals-switch-2-video-goes-over-its-80s-anime-style/ (12/08/2026, Jenni Lada)
- **Apports** : la nostalgie du **trait** des dessins animés des années 80 (Ramos). Hidetoshi Yoshida, key animator depuis plus de 40 ans, insiste sur le jeu d'acteur et les expressions, et sur les mêmes couleurs, style et **timing** que l'animé. **La vidéo elle-même n'a pas pu être visionnée** : aucun détail de pipeline n'est vérifié.

### 1.8 Famitsu, présentation du gameplay
- **URL** : https://www.famitsu.com/article/202604/72585
- **Date** : 23/04/2026, par Kazuya Teramura
- **Apports** :
  - Grain discret (« ざらつき »), action à 30 images/s parfois descendue à 12.
  - UE5, avec rendu et éclairage ajustés finement.
  - **Environ un an d'essais** pour la direction artistique.
  - Les conseils de Yoshida, par exemple sur le **dessin des explosions**, sont repris dans les effets du jeu.
  - Les cinématiques de Massket utilisent directement des assets du jeu.

### 1.9 Inven Global, compléments (l'interview est déjà connue)
- **URL** : https://www.invenglobal.com/articles/21282/memories-captured-in-30-fps-the-80s-inspired-co-op-action-game-orbitals (24/04/2026)
- **Ajouts utiles** :
  - Tests sur un **vieux téléviseur CRT** : l'effet rétro y est « definitely much stronger ».
  - Un fixe à 24 images/s a été testé puis rejeté, trop saccadé.
  - Références d'ambiance des décors : *Akira*, *Ghost in the Shell*, *Les Ailes d'Honnêamise*.
  - Métal rouillé et usé : *Venus Wars*.

### 1.10 Reprises du blog Unreal avec quelques ajouts
- **Nintendúo** : https://nintenduo.com/orbitals-graficos-anime-noventero-unreal-engine-5-switch-2/ (15/09/2026). Il confirme : cel à **3 valeurs**, *paint shader* à **textures de coups de pinceau** qui modulent la lumière, personnages à 12 images/s, **parfois 6**, caméra à 30. Distorsions de bruit sur l'ombrage et les traits. Grain, halation, aberration chromatique. Décentrer les éléments, espacer les détails au hasard, déformer les formes parfaitement droites ou rondes. Shading models, buffers et post-process sur mesure grâce au code source.
- **GoNintendo** : https://gonintendo.com/contents/64987 (15/09/2026). Une phrase de Varmedal : l'accès au code source a permis de « customize the default render pipeline ».
- **TheSixthAxis**, critique : https://www.thesixthaxis.com/2026/09/01/orbitals-review-switch-2/ (01/09/2026). Elle parle de « hand-drawn watercolour animation cells », de 12 images/s, de grain de film et de filtres « CRT-style ». C'est une formulation de journaliste, pas technique.
- **Kotaku** : https://kotaku.com/orbitals-nintendo-switch-2-it-takes-two-retro-anime-2000699552 (27/05/2026). « soft pastels and film-grain filter », comparé à *Yu Yu Hakusho* en VHS.
- **GameSpot** (non lisible, extrait de recherche seulement) : https://www.gamespot.com/articles/this-co-op-anime-adventure-could-not-have-been-anything-else/1100-6539566/. Varmedal y décrit la stylisation comme des objets « inflated… ballooning out a bit ». Les volumes sont **gonflés**, sans arêtes vives.

### 1.11 Ce qui n'a PAS été trouvé
- Aucune conférence (GDC, Unreal Fest, CEDEC, BitSummit) sur Orbitals en 2026.
- Aucun ArtStation de Johannes Varmedal ou des artistes décor, ni de compte Bluesky ou Mastodon technique.
- Aucun article de 4Gamer ou d'Automaton.
- Pas de résolution de texture, pas de méthode de projection ou d'UV des décors peints, pas de valeurs chiffrées du *paint shader*.
- Les deux vidéos (Creator's Voice et post X des décors) n'ont pas été visionnées.

---

## 2. Techniques connues pour un décor 3D « peint » sans post-process global

### 2.1 Blue Protocol, CEDEC 2020 (décors) et CEDEC 2021 (implémentation)
- **CEDEC 2020, GAME Watch** : https://game.watch.impress.co.jp/docs/news/1275034.html (04/09/2020)
  - Les modèles sont aussi précis qu'en réaliste. Les textures ont **moins d'informations** et des **inégalités de couleur façon peinture à la main**.
  - Les matériaux typiques des décors d'animé sont **standardisés dans Substance** pour que tous les artistes obtiennent le même rendu.
  - Les arêtes des objets sont **un peu plus marquées** que dans la réalité.
  - Herbe : **moins d'informations de couleur au loin**, variations de teinte sur toute la prairie, reflets qui scintillent au vent.
  - **Filtre SNN** (variante du Kuwahara) **coupé de près, activé seulement au loin**. C'est le même choix que le Kuwahara d'Orbitals.
  - **Couleur d'ombre proche et couleur d'ombre lointaine** avec une distance de transition : le proche tire vers le bleu avec peu de reflet, le lointain vers le vert avec un peu plus.
  - Post-process de « finition film » : dégradés d'écran et flares liés au soleil, qui changent sur plusieurs secondes.
- **CEDEC 2021, GameBusiness** : https://www.gamebusiness.jp/article/2021/09/13/19051.html (13/09/2021)
  - Moteur UE4 en différé et légèrement modifié. BaseColor et **ShadowColor séparés**.
  - Contours en post-process à partir de profondeur, ID, normales et dessins dans la texture. Épaisseur passée par TexCoord1 (venue des vertex colors). Canaux : R épaisseur, G ID, B dessin, A décalage de profondeur.
  - Le **TAA faisait fondre les contours** d'environ 1 pixel : stencil et ResponsiveAA.
  - Les zones cel **ne reçoivent pas d'auto-ombre** de la shadow map : il y a une shadow map séparée pour le toon et une « offset shadow ».

### 2.2 Gabriel de Laubier, « Stylized paint shader breakdown » (UE 5.1, Lumen, sans post-process)
- **URL** : https://cyn-prod.com/stylized-paint-shader-breakdown (2023)
- **Apports** :
  - Une texture regroupe le normal en RG, le **masque de pinceau** en B et un masque contrasté en A.
  - Le masque B sert à la fois de **dégradé entre deux couleurs** et de hauteur pour un **BumpOffset**, qui donne un faux relief de touche.
  - Rampe de lumière en marches **0 / 0,25 / 1** contre la direction du soleil.
  - **Normales lissées cuites en vertex color** (Blender) pour les arêtes et le rim.
  - Le masque alpha mange le pourtour (**rim érodé** en 1 bit, projeté écran) et les contours sont des coques avec un décalage aléatoire par instance.
  - Ombre stylisée par opacité forcée dans la passe d'ombre. Fonctionne avec Lumen.

### 2.3 80 Level, « Paint Stroke Mask » (UE5)
- **URL** : https://80.lv/articles/paint-stroke-mask-adding-painterly-details-for-stylized-project-in-ue5 (25/10/2022, Ghjuvannu-Carlu Sozzi)
- **Apports** :
  - Trois formes de coups de pinceau, une par canal RGB, faites dans Substance Designer (bruit, dessin vectoriel, slope blur).
  - Mappées en **world-aligned** pour que deux mêmes meshes n'aient pas les mêmes touches.
  - Opacité et tiling par canal. Fonction matériau mélangée dans la base color, avec un masque qui exclut le métal.
  - Aucune valeur chiffrée.

### 2.4 Guilty Gear Xrd et Arc System Works (cel personnages, transposable aux ennemis)
- **GDC 2015, J. C. Motomura**, « GuiltyGearXrd's Art Style: The X Factor Between 2D and 3D » : https://gdcvault.com/play/1022031/GuiltyGearXrd-s-Art-Style-The. Support : https://www.ggxrd.com/Motomura_Junya_GuiltyGearXrd.pdf (serveur injoignable ce jour). Vidéo YouTube yhGjCzxJV3E.
  - Données stockées dans les **sommets plutôt qu'en texture**, pour ne pas dépendre de la résolution.
  - **Normales éditées à la main** (visages) pour des zones d'ombre et de lumière propres.
  - Selon Famitsu, 400 à 600 os par personnage pour déformer comme en 2D.
- **ASW Academy, « Guilty Gear Toon Line Control Techniques »** : https://www.docswell.com/s/ASW_Academy/5LVY67-GG-Toonline-Eng (28/11/2025)
  - **Coque inversée** : alpha de la vertex color pour l'épaisseur (**0 = pas de trait, 0,5 = standard, 1 = double**).
  - **Bleu** = décalage en Z pour cacher les traits parasites sur les cheveux et les paupières.
  - Compensation de distance et de champ de vision : B = A × tan(FOV/2).
  - **Mettre le trait à 0 dans les creux** (plis, arête du nez).
  - Seconde série de normales dans les tangentes.
  - Comparaison avec le contour en post-process : épaisseur constante et traits aux intersections, mais réglage par pièce difficile.

### 2.5 Hi-Fi Rush, GDC 2024, « 3D Toon Rendering in Hi-Fi RUSH »
- **Vault** : https://gdcvault.com/play/1034330/3D-Toon-Rendering-in-Hi. Résumé 80 Level : https://80.lv/articles/the-making-of-hi-fi-rush-s-3d-toon-rendering-style (10/07/2024).
- **Apports** : un renderer toon en différé sur UE4, toon sur **tout, décor compris**, 60 images/s. Sujets annoncés : toon light, static shadow map, GI, comic shader. **Détails non lus**, il faut la vidéo du Vault.

### 2.6 Sable, GDC 2022
- **URL** : https://www.gamedeveloper.com/marketing/how-shedworks-refined-the-art-of-sable-in-pursuit-of-readability
- **Apport** : les **contours s'estompent avec la distance** (« fading opacity ») pour la profondeur et la lisibilité.

### 2.7 Genshin Impact, GDC 2021
- **URL** : https://gdcvault.com/play/1027538/-Genshin-Impact-Crafting-an (22/07/2021, Haoyu Cai)
- **Apport** : pipeline d'un monde ouvert anime. **Détails de rendu non trouvés** en texte.

### 2.8 Spice Frontier (Steamroller) et Nitro Gen Omega (extraits de recherche seulement, unrealengine.com refusé)
- **Spice Frontier** : https://www.unrealengine.com/spotlights/from-1980s-cartoons-to-the-metaverse-a-voyage-into-spice-frontiers-retro-2d-look (29/05/2025). Personnages en cel sur **décors picturaux**. Un plan 2D a été refait dans Unreal pour comparer, et un membre de l'équipe ne distinguait plus les deux. Pas de technique détaillée lue.
- **Nitro Gen Omega** : https://www.unrealengine.com/developer-interviews/nitro-gen-omega-bringing-the-art-style-of-spaghetti-anime-to-life-with-ue5 (09/10/2025). Règle : tout ce qui fait « trop 3D » est retiré. Beaucoup de « mouvements de caméra » d'animé sont en fait des **panoramiques 2D faits au compositing**. Pas de shader détaillé lu.

### 2.9 Académique et cinéma (pour mémoire)
- **Disney Deep Canvas (Tarzan, 1999)** : https://history.siggraph.org/learning/deep-canvas-in-disneys-tarzan. Les **coups de pinceau sont stockés en 3D** et repeints depuis plusieurs angles de caméra. Voir aussi Treasure Planet : https://history.siggraph.org/learning/creating-3d-painterly-environments-for-disneys-treasure-planet-by-odermatt-and-springfield.
- **Montesdeoca et al.**, aquarelle art-dirigée en temps réel (NPAR 2016, C&G 2017, MNPR 2018) : https://diglib.eg.org/items/908efe9f-fb6a-4d3e-98b6-bda5d952c71e/full. Assombrissement des bords par différence de gaussiennes, distorsion du papier par les normales, granulation, paramètres **peints sur le modèle**. Pour l'essentiel, c'est du post-process piloté par des données matériau.
- **Gouache ou poster color** : **aucun papier spécifique trouvé**. Les modèles qui s'en approchent (Kubelka-Munk pour les peintures « watery », IMPaSTo) visent la simulation de peinture, pas le rendu 3D.
- **Henrik Pettersson, « Recreating the starfield background art from Gunbuster (1988) »** : https://vghpe.github.io/posts/gunbuster/ (17/04/2026). En **comprimant la plage tonale** d'une capture haute définition, il observe : les bleus décalés vers la droite, le **rouge qui bave**, du grain et du halo. Les étoiles sont pointées au pinceau, les nébuleuses pulvérisées, avec un probable calque transparent pour la parallaxe. Recréé avec flous, aberration chromatique, grain et filtres film. Aucune valeur chiffrée.

---

## 3. Références sur les cels et les décors des années 80-90

### 3.1 Cellulos
- **FAQ technique des cels** : https://www.animanga.com/cels/info/cels.html (© 1999)
  - Le trait est **xérographié** (poudre, photoconduction) sur le cellulo.
  - La peinture est posée **au dos**, couleur par couleur sur toute une séquence.
  - Plusieurs niveaux (A, B, C…) : un personnage ou une bouche par niveau.
  - Décor commun à une scène, souvent fait par une autre société.
- **Synthèse de recherche** (japanesegallery.com, animation-cel.com, cellector.net) :
  - Les studios japonais ont adopté Xerox et leurs propres **trace machines** dès les années 60. Le trait de carbone pâlit et se reprend à la main là où il bave.
  - Une **charte de couleurs numérotée (色指定 iro-shitei)** fixe ombres, reflets et aplats par personnage, de jour, de nuit et en effets.
  - Les cels finis sont vérifiés posés sur le décor.
- **Traits de couleur sur le genga** (Bulbapedia, glossaire d'amateur) : https://bulbapedia.bulbagarden.net/wiki/User:Lewtwo/Genga
  - **Bleu** = limite ombre/lumière, **rouge** = reflets ou zones spéciales.
  - Ces traits n'apparaissent pas tels quels au final : ils deviennent des limites de couleur, ou des **traits colorés (色トレス iro-tore)** quand le studio les garde.
  - L'histoire du *iro-tore* (qui, quand) **n'a pas été trouvée**.
- **Empilement** (Taylor & Francis, chapitre sur l'ink and paint numérique) : on ne pouvait empiler que **6 à 7 cellulos**, car chaque feuille absorbe un peu de lumière et de couleur. Les niveaux du dessous sont donc légèrement plus ternes.
- **Ombre du cel sur le décor** : confirmée comme phénomène par Varmedal (§1.1). **Aucune source historique détaillée** sur l'écart cel/décor à la prise de vue n'a été trouvée.
- **Lumière transmise (透過光 tōkakō)**, Animation Obsessive, « Dangerous Light » : https://animationobsessive.substack.com/p/dangerous-light
  - Le cel ou le décor est **percé et éclairé par dessous** à travers une gélatine colorée, en passe séparée. Le bord est net, puis une passe givrée ou floue adoucit le halo.
  - **Bloom** : la diffusion dans l'émulsion donne un halo **cyan** autour du blanc.
  - **Halation** : halo **rouge-orangé** dû au rebond sur la base du film. *Akira* a du bloom sans halation rouge.
  - Le centre de la source change de couleur en s'éloignant. Exemples : *Neo Tokyo*, *Akira*, *Giant Robo*, *Evangelion*.
- **Harmony** (« postcard memories ») : selon une liste d'amateur sur MyAnimeList (https://myanimelist.net/stacks/829), Shichirō Kobayashi, avec Dezaki et Sugino, superposait des cels et des **peintures à la gouache** pour les arrêts sur image. C'est à vérifier.

### 3.2 Décors à la poster color
- **Kazuo Oga**, Animation Obsessive : https://animationobsessive.substack.com/p/what-kazuo-oga-thinks-about-when (21/11/2022)
  - **Nicker Poster Color**, opaque et peu chère, environ **21 couleurs**.
  - **Papier poster épais mouillé avant** pour les dégradés.
  - **Ji-nuri** (fond) posé d'un coup sur papier humide en 30 à 60 min. Le détail vient une fois sec.
  - **Couches fines**, jamais épaisses. Des dégradés bien fondus imitent l'aquarelle.
  - Il ne peint pas tout : il **insiste sur l'essentiel** et laisse le reste impressionniste.
  - Pinceaux plats japonais et pinceaux fins Sakuyo.
- **Décors d'Akira**, James Gurney : https://gurneyjourney.blogspot.com/2019/03/painting-backgrounds-of-akira.html (14/03/2019)
  - **Pinceau, aérographe et poster color**, sur un format d'environ **9 × 12 pouces**.
  - Une **règle-pont** au-dessus de la peinture pour les lignes droites.
  - Une **baguette de verre à bout rond** pour aligner les points de fenêtres.
- **Hiromasa Ogura** : début en 1977 chez Kobayashi Production. Outils : table, papier, crayon, pinceau. Ses originaux contiennent des pigments **hors gamut CMJN** (selon la page Solegraph). **Pas de description technique détaillée trouvée.**
- **Shichirō Kobayashi, Mukuo Takamura** : **pas de source technique trouvée** sur l'éponge, la réserve de papier ou les aplats. Le titre de l'exposition « Drama of Light and Shadow » est la seule piste.
- **Miyazaki**, aquarelle : https://animationobsessive.substack.com/p/how-to-paint-like-hayao-miyazaki (03/02/2023). Les reflets se font à la **peinture diluée plutôt qu'au blanc**, et il faut éviter de trop superposer pour ne pas salir.

---

## 4. Ce qu'on peut appliquer dans EON (UE 5.8)

Classement par priorité : P1 = gros effet pour peu de travail, à faire d'abord.

### 4.1 Celluloïd (personnages, ennemis, objets interactifs)
- **P1. Séparer le cel du décor.** Tout ce qui bouge ou se touche passe en cel, tout le fixe en matériau « peint » (Orbitals, Jakob). C'est aussi un signal de gameplay pour un twin-stick : ennemis et pickups ressortent du décor.
- **P1. Cel en 2 ou 3 valeurs à couleurs d'asset.** Ombre et ombre profonde se choisissent par asset au lieu d'un assombrissement automatique : BaseColor et ShadowColor séparés comme dans Blue Protocol. La palette suit le §1.2 : pastels ternis, noir et blanc contrastés, **pas d'orange vif**.
- **P1. Animation à 12 images/s** (tenue de 2 sur 24), parfois 6, avec le jeu et la caméra à 30 ou plus. Varier l'ombrage et le trait d'un léger bruit **à chaque image tenue** reproduit le « 1コマごとに揺れ » de Varmedal.
- **P2. Contours en coque inversée pilotés par vertex color** : épaisseur en alpha avec 0,5 de base, 0 dans les creux, et compensation de distance et de FOV (méthode ASW). En vue de dessus, la caméra est presque fixe, donc l'épaisseur se règle une fois. **Trait sombre brun ou teinté** plutôt que noir pur. Le *iro-tore* sur les zones de lumière reste une option, faute de source précise.
- **P2. Normales éditées ou lissées** (cuites en vertex color ou en seconde série de normales) pour des masses d'ombre propres. Volumes « gonflés », sans arêtes vives (Varmedal, GameSpot).
- **P2. Pas d'auto-ombre calculée sur les cels.** À la place, une **ombre portée en aplat sombre décalée sur le décor** (décal), comme sur l'image du §1.3 et dans le « cel shadow » de Varmedal.
- **P3. Highlights peints** : un reflet dur sur la limite d'ombre, et des reflets d'arêtes par vertex color.

### 4.2 Décor gouache (sans filtre plein écran)
- **P1. Peindre les textures du décor**, à la main ou dans Rebelle comme Orbitals, voire sur papier puis scan comme le suggère la vidéo du §1.3. Le principe vient d'Oga : **fond (ji-nuri) en grands aplats dégradés, détail seulement sur l'essentiel**, couches fines. Il faut **peu d'informations par texture** et des inégalités de couleur peintes (Blue Protocol 2020).
- **P1. Éclairage peint ou baké, pas Lumen dynamique sur le décor.** Orbitals peint une partie de la lumière directement. Dans EON : lightmaps ou vertex color baké pour la lumière principale, et lumière peinte dans la texture là où elle compte. Lumen reste possible, mais c'est l'artiste qui décide du résultat.
- **P1. Masques de coups de pinceau dans le matériau** : RGB à 3 touches, en world-aligned. Ils **modulent la transition lumière/ombre** (seuil de la rampe déformé par le masque, comme les « brush stroke textures » d'Orbitals) au lieu d'un simple voile sur la base color. Rampe en marches 0 / 0,25 / 1 à essayer (de Laubier). BumpOffset sur la hauteur du masque pour l'épaisseur de touche.
- **P2. Arêtes du décor** : liseré clair d'arête en vertex color (reflets d'Orbitals), et **traits sombres fins et cassés** sur les structures, plus discrets que ceux des cels (image du §1.6). Les estomper avec la distance (Sable).
- **P2. Ombres teintées** : couleur d'ombre proche et lointaine (Blue Protocol, bleu près et vert loin), au lieu d'un gris.
- **P3. Lointain seulement : abstraction type Kuwahara ou SNN**, masquée par la profondeur et coupée de près. C'est ce que font **Orbitals et Blue Protocol**, et non un filtre plein écran comme l'essai refusé. En vue de dessus, EON n'a presque pas de lointain : **probablement inutile**, à ne pas refaire sans l'accord de Vincent.
- **P3. Projection caméra de peintures** façon Deep Canvas, ou peinture d'un plan fixe projeté. C'est intéressant pour les salles vues sous un angle fixe, mais aucune source Orbitals ne le confirme.

### 4.3 Effets film
- **P1. VFX en cels à 12 images/s**, bords durs, 2 tons, avec un halo seulement autour du cœur lumineux (flammes du §1.6). Pour les explosions, s'inspirer des conseils de Yoshida (Famitsu).
- **P1. Grain léger, halation rouge-orangée autour des hautes lumières, bloom à tendance cyan, légère aberration chromatique, vignettage** (Orbitals, Dangerous Light, Gunbuster). Bloom masqué : seuls les éléments émissifs voulus en ont.
- **P2. Lumière transmise (tōkakō)** pour les tirs, yeux et écrans : cœur saturé à bord net et halo coloré dont la teinte change vers l'extérieur.
- **P2. Calibrage gamma et test sur CRT ou sur écran ancien**, comme Shapefarm.
- **P3. Rouge qui bave et bleu décalé** (analyse Gunbuster), ombre du cel « transparaissant » légèrement sur le décor, lumières qui clignotent et interfèrent.

---

## 5. Résumé en 10 lignes

1. Orbitals sépare strictement le cel (tout ce qui bouge) du *paint shader* (décor fixe). C'est aussi un outil de lisibilité du gameplay.
2. Les décors sont peints dans **Rebelle**. Une vidéo officielle montre des éléments de décor **peints physiquement sur papier** au pinceau plat.
3. Une partie de l'éclairage est **peinte à la main** plutôt que calculée (Jakob Lundgren, Denfami 23/04/2026).
4. Orbitals utilise bien un **Kuwahara**, mais **seulement sur le lointain**, et l'a optimisé pendant plus de six mois. Blue Protocol fait pareil avec un SNN. Le filtre plein écran est donc à proscrire, comme l'a jugé Vincent.
5. Le cel est en 3 valeurs à couleurs d'asset, avec un trait et un ombrage qui **tremblent d'une image à l'autre**, à 12 images/s (parfois 6) sur une caméra à 30.
6. Varmedal recrée l'**ombre du cel visible sur le décor** et les interférences de lumière du banc-titre.
7. La palette est faite de pastels ternis et d'un fort contraste noir et blanc, sans orange vif. Les volumes sont « gonflés ».
8. Le décor a aussi des **contours fins et cassés** et des liserés clairs d'arête.
9. Techniques prêtes pour UE5 : masques de pinceau world-aligned qui modulent la rampe, BumpOffset, normales lissées en vertex color, ombres teintées proche/loin, contours en coque à vertex color (ASW).
10. Introuvables : les valeurs du *paint shader*, la résolution et la projection des textures, le contenu complet des deux vidéos, et des sources techniques sur Kobayashi et Takamura.

## 6. Les 10 sources les plus utiles

1. Denfami Nico Gamer, interview Orbitals (23/04/2026) : https://news.denfaminicogamer.jp/interview/2604233a
2. Post X @OrbitalsGame, fabrication des éléments de décor (16/08/2026) : https://x.com/OrbitalsGame/status/2088951935260823762
3. orbitalsgame.com, « Hello from the team » (18/03/2026), outils dont Rebelle : https://www.orbitalsgame.com/post/hello-from-the-team-behind-orbitals
4. GAME Watch, Blue Protocol CEDEC 2020, décors (04/09/2020) : https://game.watch.impress.co.jp/docs/news/1275034.html
5. GameBusiness, Blue Protocol CEDEC 2021, implémentation (13/09/2021) : https://www.gamebusiness.jp/article/2021/09/13/19051.html
6. Gabriel de Laubier, paint shader UE5 sans post-process (2023) : https://cyn-prod.com/stylized-paint-shader-breakdown
7. ASW Academy, Toon Line Control (28/11/2025) : https://www.docswell.com/s/ASW_Academy/5LVY67-GG-Toonline-Eng
8. Animation Obsessive, Kazuo Oga (21/11/2022) : https://animationobsessive.substack.com/p/what-kazuo-oga-thinks-about-when
9. Animation Obsessive, « Dangerous Light » (lumière transmise, bloom, halation) : https://animationobsessive.substack.com/p/dangerous-light
10. Denfami Nico Gamer, table ronde avec Kameyama (palette) (02/09/2026) : https://news.denfaminicogamer.jp/interview/260902a
