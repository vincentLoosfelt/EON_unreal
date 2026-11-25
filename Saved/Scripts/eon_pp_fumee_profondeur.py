# EON — traits blancs sous la fumée (Vincent, 25/11 : « pourquoi il y a des traits blancs sous certaines sphères ? »).
# Cause : la profondeur custom de la fumée (stencil 3) déborde un peu de la fumée visible (déplacement / érosion pas
# identiques d'une passe à l'autre). Sous ce débord, le sol passait le test « celluloïd » (scène DERRIÈRE la profondeur
# custom) : il échappait à l'assombrissement du décor (M_PP_EON_Decor) et ressortait pâle, en croissant sous les sphères.
# Correction : pour la fumée seulement, la scène doit être À la profondeur custom (pas derrière). Patche les nœuds Custom
# de M_PP_EON_Decor et M_PP_EON_Trait en place (réglages gardés) et les sources eon_decor_peint.py / eon_trait_cel.py.
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
SCR = os.path.join(unreal.Paths.project_saved_dir(), "Scripts")
# fumée (stencil 3) : scène à moins de 0,15 % + 1 cm derrière la profondeur custom
FUMOK0 = "(abs(LK(u, 25).r - 3.0) > 0.5 || LK(u, 1).r < LK(u, 13).r * 1.0015 + 1.0)"
# v2 : la profondeur custom de la fumée Niagara a une image de retard (sphères à leur position précédente) ; au contact du
# sol, le sol sous ce « fantôme » est à quelques cm seulement : on le reconnaît aussi à sa normale tournée vers le haut.
FUMOK2 = "(abs(LK(u, 25).r - 3.0) > 0.5 || LK(u, 1).r < LK(u, 13).r * 1.0015 + 1.0 || normalize(LK(u, 8).rgb).z < 0.7)"
# v3 : le fantôme traverse le sol et les rebords ; autour de l'intersection il est à quelques cm de la surface, d'où une
# bande en croissant que v1/v2 laissaient passer (et v2 laissait passer les murs). Fumée = scène À la profondeur custom :
# 0,02 % + 3 mm (même géométrie -> mêmes valeurs ; la bande tombe sous le pixel).
FUMOK3 = "(abs(LK(u, 25).r - 3.0) > 0.5 || LK(u, 1).r < LK(u, 13).r * 1.0002 + 0.3)"
# v4 (diagnostic du 25/11) : la vraie cause est de L'AUTRE côté. Les sphères s'enfoncent dans le sol ; la profondeur custom
# voit leur partie enterrée, juste SOUS le sol. Le test d'origine tolère la scène jusqu'à 0,3 % + 1 cm DEVANT la profondeur
# custom (pieds des personnages posés au sol) : le sol au-dessus d'une sphère enterrée passait pour de la fumée et était
# éclairci (CelExpo = 2 diaphs) -> anneaux et croissants pâles au pied des sphères. Fumée : scène = profondeur custom
# à 0,02 % + 3 mm près, des deux côtés.
FUMOK = "(abs(LK(u, 25).r - 3.0) > 0.5 || (LK(u, 1).r < LK(u, 13).r * 1.0002 + 0.3 && LK(u, 1).r > LK(u, 13).r * 0.9998 - 0.3))"
CIBLES = [
    ("/Game/EON/Materials/Peint/M_PP_EON_Decor", "eon_decor_peint.py",
     "#define CEL(u) (LK(u, 25).r > 0.5 && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0)",
     "#define CEL(u) (LK(u, 25).r > 0.5 && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0 && " + FUMOK + ")"),
    ("/Game/EON/Materials/M_PP_EON_Trait", "eon_trait_cel.py",
     "#define VIS(u) ((abs(LK(u, 25).r - 1.0) < 0.5 || abs(LK(u, 25).r - 3.0) < 0.5) && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0)",
     "#define VIS(u) ((abs(LK(u, 25).r - 1.0) < 0.5 || abs(LK(u, 25).r - 3.0) < 0.5) && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0 && " + FUMOK + ")"),
]
log = []
for chemin, src, a, b in CIBLES:
    p = os.path.join(SCR, src)
    t = open(p, encoding="utf-8").read()
    t = t.replace(a[:-1] + " && " + FUMOK0 + ")", a).replace(a[:-1] + " && " + FUMOK2 + ")", a).replace(a[:-1] + " && " + FUMOK3 + ")", a)   # v1/v2/v3 -> d'origine
    if a in t:
        open(p, "w", encoding="utf-8").write(t.replace(a, b, 1)); log.append("source OK " + src)
    else:
        log.append(("source déjà " if b in t else "source ABSENT ") + src)
    g = unreal.load_asset(chemin)
    n = 0
    for e in MEL.get_material_expressions(g):
        if isinstance(e, unreal.MaterialExpressionCustom):
            c = e.get_editor_property("code").replace(a[:-1] + " && " + FUMOK0 + ")", a).replace(a[:-1] + " && " + FUMOK2 + ")", a).replace(a[:-1] + " && " + FUMOK3 + ")", a)
            if a in c:
                e.set_editor_property("code", c.replace(a, b, 1)); n += 1
            elif b in c:
                log.append("asset déjà " + chemin)
    log.append("%s : %d nœud(s) patché(s)" % (chemin, n))
    MEL.recompile_material(g)
    log.append("sauvé %s" % EAL.save_loaded_asset(g))
open(os.path.join(unreal.Paths.project_saved_dir(), "pp_fumee_profondeur.txt"), "w", encoding="utf-8").write("\n".join(log))
