# EON — trait de la fumée toon (Vincent, 24/11 : « fusionner les sphères sur leur contact, pas de line art intérieur,
# que l'extérieur ») : la fumée a le stencil 3. Sur elle, le trait ne se pose qu'au bord de la masse (voisin qui n'est
# pas de la fumée et qui est derrière, ou hors celluloïd) ; ni saut de profondeur entre lobes, ni trait intérieur.
# Patche le nœud Custom de M_PP_EON_Trait en place (les réglages des paramètres restent) et la source eon_trait_cel.py.
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
R = [
 ("#define VIS(u) (LK(u, 25).r > 0.5 && LK(u, 25).r < 1.5 && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0)",
  "#define VIS(u) ((abs(LK(u, 25).r - 1.0) < 0.5 || abs(LK(u, 25).r - 3.0) < 0.5) && LK(u, 1).r > LK(u, 13).r * 0.997 - 1.0)\n"
  "// Fumée toon (stencil 3) : ses lobes forment une seule masse, détourée seulement à l'extérieur.\n"
  "#define FUM(u) (abs(LK(u, 25).r - 3.0) < 0.5)"),
 ("float Wmax = max(Ws, Wi) + 0.5 + Soft;",
  "bool fum = FUM(uv);\nif (fum) Wi = 0.0;\nfloat Wmax = max(Ws, Wi) + 0.5 + Soft;"),
 ("if (dq > d0 * (1.0 + DepthTol) || !VIS(q)) { dS = min(dS, r - 0.25); hits += 1; break; }",
  "if ((dq > d0 * (1.0 + DepthTol) && !(fum && FUM(q))) || !VIS(q)) { dS = min(dS, r - 0.25); hits += 1; break; }"),
 ("if (r <= Wi + Soft) {", "if (!fum && r <= Wi + Soft) {"),
]
log = []
g = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")
src = os.path.join(unreal.Paths.project_saved_dir(), "Scripts", "eon_trait_cel.py")
txt = open(src, encoding="utf-8").read()
for a, b in R:
    if a in txt: txt = txt.replace(a, b, 1); log.append("source OK " + a[:40])
open(src, "w", encoding="utf-8").write(txt)
done = False
for e in MEL.get_material_expressions(g) if hasattr(MEL, "get_material_expressions") else []:
    if isinstance(e, unreal.MaterialExpressionCustom) and "Trait" in str(e.get_editor_property("description")):
        c = e.get_editor_property("code")
        for a, b in R:
            if a in c: c = c.replace(a, b, 1); log.append("asset OK " + a[:40])
            else: log.append("asset ABSENT " + a[:40])
        e.set_editor_property("code", c); done = True
log.append("noeud trouvé %s" % done)
MEL.recompile_material(g)
log.append("sauvé %s" % EAL.save_loaded_asset(g))
open(os.path.join(unreal.Paths.project_saved_dir(), "trait_fumee.txt"), "w", encoding="utf-8").write("\n".join(log))
