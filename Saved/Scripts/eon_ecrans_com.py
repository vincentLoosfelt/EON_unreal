# EON — écrans du poste du C.O.M (Vincent, 26/11 : « vert cathodique animé », choix 1).
#
# 1) M_EON_EcranCRT (/Game/EON/Materials/Ecrans) : écran cathodique vert, contenus procéduraux animés repris du placeholder
#    de la passation (PLACEHOLDERS_PIXEL/ecran_mural.png : hélice d'ADN, histogramme, courbe, lignes de texte, jauge),
#    avec scanlines, vignettage, bande de balayage qui descend et léger scintillement.
#    Les 3 petits écrans du module partagent la même zone d'UV ([0 ; 0,28] × [0,376 ; 0,712]) : on les distingue par leur
#    X local (centres à -126, -162 et -192 cm, axes Blender = Unreal avec Y inversé). V est inversé à l'import FBX :
#    plages ci-dessous en V Unreal = 1 - V Blender. Le grand écran vertical a ses UV
#    à part ([0,657 ; 0,83] × [0,37 ; 0,602]) : texte qui défile + jauge. La dalle du portable incliné
#    (SM_Ecran_Com_Portable, blender_ecran_portable.py, UV dans [2 ; 3]) : radar. Les autres dalles de ce maillage
#    (moniteur double face Ecran.006, borne Ecran.007, tablette Mini ecran, sans matériau d'écran dans le FBX) :
#    UV dans [3 ; 4] ADN, [4 ; 5] histogramme, [5 ; 6] oscilloscope, [6 ; 7] texte.
# 2) L_HUB : SM_Ecran_Com posé avec la position, la rotation et l'échelle (1,5) du C.O.M (même pivot dans le .blend),
#    3 fois à 120° pour faire le tour (un module = un tiers du tour), acteurs « COM_Ecrans_1 » à « _3 », mobile et attaché au C.O.M pour descendre avec lui (EONFloorManager déplace l'acteur COMv1).
#    Matériaux changés sur le composant seulement (Light_screen -> CRT, damier Material_0 -> métal gris foncé) :
#    les assets de /Game/EON/Meshes/COM ne sont pas modifiés.
# À lancer dans l'éditeur (charge et enregistre L_HUB) :
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_ecrans_com.py"
import unreal

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
P = unreal.MaterialProperty
MDIR, MNAME = "/Game/EON/Materials/Ecrans", "M_EON_EcranCRT"
M = MDIR + "/" + MNAME
LABEL = "COM_Ecrans"
DECAL = 0.0   # cm : poussée vers le bord le long de l'axe du module ; 0 obligatoire, le module contient aussi le pilier central
NB = 4   # modules autour du pilier (Vincent, 26/11 : 3 = un tiers du tour chacun, puis « 4 en tout »)
LOGF = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_ecrans_com.txt"
LOG = []


def log(s):
    LOG.append(s)
    unreal.log("EON_ECRANS " + s)


# ---------------------------------------------------------------- matériau
if EAL.does_asset_exist(M):
    EAL.delete_asset(M)
m = AT.create_asset(MNAME, MDIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("used_with_nanite", True)   # SM_Ecran_Com est en Nanite


def node(cls, y, **kw):
    e = MEL.create_material_expression(m, cls, -900, y)
    for k, v in kw.items():
        e.set_editor_property(k, v)
    return e


col = node(unreal.MaterialExpressionVectorParameter, -300, parameter_name="Couleur", default_value=unreal.LinearColor(0.08, 1.0, 0.22, 1), group="Ecran")
inten = node(unreal.MaterialExpressionScalarParameter, -200, parameter_name="Intensite", default_value=1.6, group="Ecran")
uv = node(unreal.MaterialExpressionTextureCoordinate, -100)
wp = node(unreal.MaterialExpressionWorldPosition, 0)
lp = node(unreal.MaterialExpressionTransformPosition, 50)
lp.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
lp.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
MEL.connect_material_expressions(wp, "", lp, "")
tim = node(unreal.MaterialExpressionTime, 100)
cu = node(unreal.MaterialExpressionCustom, 200)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cu.set_editor_property("description", "Ecran cathodique")
ins = []
for n in ("UV", "LP", "T", "Col", "Int"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    ins.append(ci)
cu.set_editor_property("inputs", ins)
cu.set_editor_property("code", """
float2 uv;
int id;
// uv final : x de gauche a droite, y de haut en bas, vus de face.
if (UV.x > 1.5) { uv = float2(frac(UV.x), UV.y); id = UV.x < 3.0 ? 4 : (UV.x < 4.0 ? 0 : (UV.x < 5.0 ? 1 : (UV.x < 6.0 ? 2 : 3))); }   // dalles (SM_Ecran_Com_Portable) : [2;3] radar, [3;4] ADN, [4;5] histogramme, [5;6] oscilloscope, [6;7] texte
else if (UV.x > 0.5) { uv = 1.0 - float2((UV.x - 0.657) / 0.173, (UV.y - 0.398) / 0.232); id = 3; }   // grand ecran : tourne de 180 deg
else { float2 q = float2(UV.x / 0.280, (UV.y - 0.288) / 0.336); uv = float2(1.0 - q.y, q.x); id = LP.x > -144.0 ? 0 : (LP.x > -177.0 ? 1 : 2); }   // petits : 90 deg
uv = saturate(uv);
float g = 0.0;
if (id == 0)
{
    // Helice d'ADN qui defile
    float y = uv.y * 2.5 + T * 0.35;
    float a = sin(y * 6.2832) * 0.3;
    float s1 = smoothstep(0.045, 0.0, abs(uv.x - 0.5 - a));
    float s2 = smoothstep(0.045, 0.0, abs(uv.x - 0.5 + a));
    float rung = step(frac(y * 7.0), 0.3) * step(abs(uv.x - 0.5), abs(a));
    g = max(max(s1, s2), rung * 0.45);
}
else if (id == 1)
{
    // Histogramme dont les barres changent en douceur
    float n = 6.0;
    float i = floor(uv.x * n);
    float fx = frac(uv.x * n);
    float k = floor(T * 1.5);
    float h1 = 0.15 + 0.7 * frac(sin(i * 12.9898 + k * 4.1) * 43758.5453);
    float h2 = 0.15 + 0.7 * frac(sin(i * 12.9898 + (k + 1.0) * 4.1) * 43758.5453);
    float h = lerp(h1, h2, smoothstep(0.0, 1.0, frac(T * 1.5)));
    g = step(0.18, fx) * step(fx, 0.82) * step(1.0 - uv.y, h) * 0.9;
    g = max(g, step(abs(uv.y - 0.93), 0.012) * 0.6);
}
else if (id == 2)
{
    // Oscilloscope : grille + courbe qui defile
    float gx = step(frac(uv.x * 6.0), 0.05) + step(frac(uv.y * 4.0), 0.06);
    float w = 0.5 + 0.28 * sin(uv.x * 13.0 - T * 4.0) * sin(uv.x * 3.1 + T * 1.3);
    g = saturate(gx) * 0.25 + smoothstep(0.05, 0.0, abs(uv.y - w));
}
else if (id == 4)
{
    // Radar : cercles, croix, balayage qui tourne avec sa traine, echos
    float2 p = (uv - 0.5) * 2.0;
    float r = length(p);
    float ang = atan2(p.y, p.x);
    float sweep = frac((ang / 6.2832) - T * 0.25);
    float trail = pow(1.0 - sweep, 6.0) * step(r, 0.95);
    float rings = step(abs(r - 0.32), 0.012) + step(abs(r - 0.63), 0.012) + step(abs(r - 0.94), 0.015);
    float cross = (step(abs(p.x), 0.008) + step(abs(p.y), 0.008)) * step(r, 0.94);
    float2 b1 = float2(0.35, -0.42), b2 = float2(-0.5, 0.2), b3 = float2(0.15, 0.6);
    float blips = smoothstep(0.06, 0.0, length(p - b1)) + smoothstep(0.05, 0.0, length(p - b2)) + smoothstep(0.045, 0.0, length(p - b3));
    g = saturate(rings * 0.45 + cross * 0.3 + trail * 0.7 + blips * (0.3 + trail * 3.0));
}
else
{
    // Lignes de texte qui defilent + jauge
    float ly = uv.y * 16.0 + T * 1.2;
    float li = floor(ly);
    float fy = frac(ly);
    float len = 0.25 + 0.65 * frac(sin(li * 7.13) * 4375.85);
    float cx = floor(uv.x * 22.0);
    float ch = step(0.3, frac(sin((cx + li * 31.0) * 3.7) * 9731.1));
    g = step(0.25, fy) * step(fy, 0.75) * step(uv.x, len) * ch * 0.85;
    if (uv.y > 0.86)
    {
        float lvl = 0.5 + 0.45 * sin(T * 0.7);
        g = step(uv.x, lvl) * step(0.89, uv.y) * step(uv.y, 0.96) + step(abs(uv.y - 0.875), 0.006) * 0.5;
    }
}
float scan = 0.72 + 0.28 * sin(uv.y * 260.0);
float2 d = uv - 0.5;
float vig = saturate(1.0 - dot(d, d) * 2.2);
float roll = 1.0 + 0.3 * smoothstep(0.08, 0.0, abs(frac(uv.y - T * 0.2) - 0.5));
float flick = 0.94 + 0.06 * sin(T * 53.0);
return Col * (0.08 + g) * scan * vig * roll * flick * Int;
""")
for src, pin in ((uv, "UV"), (lp, "LP"), (tim, "T"), (col, "Col"), (inten, "Int")):
    MEL.connect_material_expressions(src, "", cu, pin)
MEL.connect_material_property(cu, "", P.MP_EMISSIVE_COLOR)
noir = node(unreal.MaterialExpressionConstant3Vector, 300, constant=unreal.LinearColor(0.01, 0.015, 0.01, 1))
MEL.connect_material_property(noir, "", P.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionConstant, 400, r=0.25)   # verre de l'écran : reflets doux
MEL.connect_material_property(rough, "", P.MP_ROUGHNESS)
MEL.recompile_material(m)
EAL.save_asset(M, only_if_is_dirty=False)
log("M_EON_EcranCRT compilé")

# ---------------------------------------------------------------- maillages (Blender -> FBX -> Unreal)
def importer(fbx, nom, nanite):
    task = unreal.AssetImportTask()
    task.filename = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "SourceAssets/Modules/Com/" + fbx
    task.destination_path = "/Game/EON/Meshes/Ecrans"
    task.destination_name = nom
    task.automated = True; task.replace_existing = True; task.save = True
    fo = unreal.FbxImportUI()
    fo.import_mesh = True; fo.import_materials = False; fo.import_textures = False; fo.import_as_skeletal = False
    fo.static_mesh_import_data.set_editor_property("combine_meshes", True)
    fo.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
    task.options = fo
    AT.import_asset_tasks([task])
    sm = unreal.load_asset("/Game/EON/Meshes/Ecrans/" + nom)
    if sm and nanite:
        ns = sm.get_editor_property("nanite_settings")
        ns.set_editor_property("enabled", True)
        sm.set_editor_property("nanite_settings", ns)
        EAL.save_loaded_asset(sm)
    if sm:
        log("%s : importé, taille %s, emplacements %s" % (nom, sm.get_bounds().box_extent * 2, [str(x.material_slot_name) for x in sm.get_editor_property("static_materials")]))
    else:
        log("%s : ÉCHEC" % nom)
    return sm


# Dalle du portable incliné (blender_ecran_portable.py) et module par groupes de matériaux (blender_ecran_com_peint.py).
port = importer("SM_Ecran_Com_Portable.fbx", "SM_Ecran_Com_Portable", False)
mesh = importer("SM_Ecran_Com_Peint.fbx", "SM_Ecran_Com_Peint", True)

# ---------------------------------------------------------------- matériaux du concept art (Vincent, 26/11)
# Décor peint (M_EON_DecorPeint, réglages de la variante B aquarelle du HUB, copiés de MI_Peint_BaseWhite_B).
# Trois teintes de coque comme les trois planches du concept : gris, gris bleu, beige ; une par module (4e = gris).
PEINT = unreal.load_asset("/Game/EON/Materials/Peint/MI_Peint_BaseWhite_B")
TEINTES = {"Gris": {"Coque": (0.50, 0.51, 0.50), "Metal": (0.28, 0.29, 0.29)},
           "Bleu": {"Coque": (0.38, 0.45, 0.52), "Metal": (0.22, 0.26, 0.30)},
           "Beige": {"Coque": (0.56, 0.47, 0.38), "Metal": (0.32, 0.27, 0.23)}}
COMMUNS = {"Sombre": (0.045, 0.045, 0.05), "Cables": (0.03, 0.03, 0.035), "Tuyau": (0.85, 0.28, 0.03)}
ORDRE = ["Gris", "Bleu", "Beige", "Gris"]


def nouvelle_mi(nom):
    p = MDIR + "/" + nom
    if EAL.does_asset_exist(p):
        return p, unreal.load_asset(p)
    return p, AT.create_asset(nom, MDIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())


def mi_peint(nom, rgb):
    p, mi = nouvelle_mi(nom)
    MEL.set_material_instance_parent(mi, PEINT.get_editor_property("parent"))
    for sp in PEINT.get_editor_property("scalar_parameter_values"):
        MEL.set_material_instance_scalar_parameter_value(mi, sp.parameter_info.name, sp.parameter_value)
    for vp in PEINT.get_editor_property("vector_parameter_values"):
        MEL.set_material_instance_vector_parameter_value(mi, vp.parameter_info.name, vp.parameter_value)
    MEL.set_material_instance_vector_parameter_value(mi, "BC1", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.update_material_instance(mi)
    EAL.save_asset(p, only_if_is_dirty=False)
    return mi


def mi_de(parent, nom, vecs):
    p, mi = nouvelle_mi(nom)
    MEL.set_material_instance_parent(mi, parent)
    for k, v in vecs.items():
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    MEL.update_material_instance(mi)
    EAL.save_asset(p, only_if_is_dirty=False)
    return mi


MAT_COMMUNS = {g: mi_peint("MI_ComEcran_" + g, c) for g, c in COMMUNS.items()}
MAT_TEINTES = {t: {g: mi_peint("MI_ComEcran_%s_%s" % (g, t), c) for g, c in d.items()} for t, d in TEINTES.items()}
# Écrans ambre comme le concept (le vert reste le défaut de M_EON_EcranCRT).
crt = mi_de(unreal.load_asset(M), "MI_EON_EcranCRT_Ambre", {"Couleur": (1.0, 0.3, 0.015)})
MEL.set_material_instance_scalar_parameter_value(crt, "Intensite", 1.1)
MEL.update_material_instance(crt)
EAL.save_asset(MDIR + "/MI_EON_EcranCRT_Ambre", only_if_is_dirty=False)
# Voyants : clignotants de M_EON_Voyant (eon_voyants_clignote.py), intensité des voyants des modules (1,54).
VOY = unreal.load_asset("/Game/EON/Materials/M_EON_Voyant")
if VOY and not VOY.get_editor_property("used_with_nanite"):
    MEL.set_material_usage(VOY, unreal.MaterialUsage.MATUSAGE_NANITE)
    MEL.recompile_material(VOY)
    EAL.save_loaded_asset(VOY, False)
MAT_COMMUNS["Voyant_Vert"] = mi_de(VOY, "MI_ComEcran_Voyant_Vert", {"EmissiveColor": (0, 1.54, 0), "DiffuseColor": (0, 1, 0)})
MAT_COMMUNS["Voyant_Rouge"] = mi_de(VOY, "MI_ComEcran_Voyant_Rouge", {"EmissiveColor": (1.54, 0, 0), "DiffuseColor": (1, 0, 0)})
MAT_COMMUNS["Voyant_Jaune"] = mi_de(VOY, "MI_ComEcran_Voyant_Jaune", {"EmissiveColor": (1.54, 0.9, 0), "DiffuseColor": (1, 0.7, 0)})
MAT_COMMUNS["Ecran"] = crt
log("instances : %s + teintes %s" % (sorted(MAT_COMMUNS), sorted(MAT_TEINTES)))


def matiere(slot, teinte):
    g = slot.split(".")[0]
    if g in MAT_TEINTES[teinte]:
        return MAT_TEINTES[teinte][g]
    return MAT_COMMUNS.get(g)


# ---------------------------------------------------------------- pose dans L_HUB
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
com = None
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor):
        sm = a.static_mesh_component.static_mesh
        if sm and sm.get_name().startswith("COMv1"):
            com = a
    if a.get_actor_label().startswith(LABEL):
        log("ancien %s retiré" % a.get_actor_label())
        eas.destroy_actor(a)
if not com or not mesh:
    log("ÉCHEC : C.O.M (COMv1) introuvable dans L_HUB ou module non importé")
else:
    import math
    t = com.get_actor_transform()
    # NB modules tournés autour de l'axe du C.O.M (le pivot du module est l'axe du C.O.M ; le module contient le pilier).
    for k in range(NB):
        r = t.rotation.rotator()
        r.yaw += 360.0 / NB * k
        bc = mesh.get_bounds().origin
        ang = math.radians(math.degrees(math.atan2(bc.y, bc.x)) + r.yaw)
        loc = t.translation + unreal.Vector(math.cos(ang) * DECAL, math.sin(ang) * DECAL, 0)
        teinte = ORDRE[k % len(ORDRE)]
        ecr = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, r)
        ecr.set_actor_label("%s_%d" % (LABEL, k + 1))
        ecr.set_actor_scale3d(t.scale3d)
        smc = ecr.static_mesh_component
        smc.set_mobility(unreal.ComponentMobility.MOVABLE)
        smc.set_static_mesh(mesh)
        manque = []
        for i, sl in enumerate(mesh.get_editor_property("static_materials")):
            mt = matiere(str(sl.material_slot_name), teinte)
            if mt:
                smc.set_material(i, mt)
            else:
                manque.append(str(sl.material_slot_name))
        ecr.attach_to_actor(com, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
        log("%s posé (lacet %.0f, teinte %s)%s" % (ecr.get_actor_label(), r.yaw, teinte, (" - sans matériau : %s" % manque) if manque else ""))
        if port:
            pa = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, r)
            pa.set_actor_label("%s_%d_Portable" % (LABEL, k + 1))
            pa.set_actor_scale3d(t.scale3d)
            pa.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            pa.static_mesh_component.set_static_mesh(port)
            pa.static_mesh_component.set_material(0, crt)
            pa.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            pa.attach_to_actor(com, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    les.save_current_level()
    log("L_HUB enregistré")
open(LOGF, "w", encoding="utf-8").write("\n".join(LOG))
open(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Logs/eon_ecrans_com.fini", "w").write("ok")
