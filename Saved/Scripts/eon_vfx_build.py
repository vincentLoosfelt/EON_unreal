# EON — Fabrique les effets Niagara du proto (particules carrées « part() » et anneaux).
# Lancer : UnrealEditor-Cmd EON.uproject -run=pythonscript -script=<ce fichier>
#
# Le proto n'a qu'un modèle de particule : part(x, y, vx, vy, vie, couleur, taille, frein), un carré qui file en ligne
# droite, freiné (v -= v·frein·dt), sans gravité, opaque jusqu'au tiers de sa vie puis qui s'efface
# (alpha = min(1, 1,5 × vie restante)). Et l'anneau : un cercle de 1 px qui grandit de 0 à R en s'effaçant.
# Ces deux effets sont donc génériques : le C++ (EONFx) règle chaque gerbe par paramètres utilisateur.
#
#   NS_EON_Gerbe     : User.Count (int), Color, Life, Size (vec2, cm), Speed (cm/s, vitesse max), Drag, Radius (cm, dispersion du départ)
#                      Vitesses : demi-sphère vers le haut (cône total 180°, sans les 40° du sommet), de 25 à 100 % de Speed.
#   NS_EON_GerbeDir  : idem + User.Dir (vec3) : cône de 63° autour de la direction (gerbe de mort, éclats de coup).
#   NS_EON_Anneau    : User.Color, Life, Size (vec2 = diamètre final) : un anneau à plat sur le sol.
#   M_EON_Particule / M_EON_Anneau : non éclairés, translucides, fondu par l'âge de la particule (dans le matériau).
import unreal

L = unreal.EONVfxLib
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
DOS = "/Game/EON/VFX"
log = []


def ok(b, what):
    log.append(("OK   " if b else "ERR  ") + what)
    if not b:
        unreal.log_error("EONVFX " + what)


def material(name, ring):
    path = DOS + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    m = AT.create_asset(name, DOS, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_niagara_sprites", True)
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
    pc = E(unreal.MaterialExpressionParticleColor, -900, 0)
    rt = E(unreal.MaterialExpressionParticleRelativeTime, -900, 300)
    boost = E(unreal.MaterialExpressionScalarParameter, -900, -200)
    boost.set_editor_property("parameter_name", "Boost")
    boost.set_editor_property("default_value", 4.0)
    emis = E(unreal.MaterialExpressionMultiply, -500, -100)
    MEL.connect_material_expressions(pc, "RGB", emis, "A")
    MEL.connect_material_expressions(boost, "", emis, "B")
    MEL.connect_material_property(emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # 1 - âge normalisé
    inv = E(unreal.MaterialExpressionOneMinus, -700, 300)
    MEL.connect_material_expressions(rt, "", inv, "")
    if not ring:
        # alpha = saturate(1,5 × (1 - âge)) × alpha de la couleur
        k = E(unreal.MaterialExpressionMultiply, -500, 300)
        k.set_editor_property("const_b", 1.5)
        MEL.connect_material_expressions(inv, "", k, "A")
        sat = E(unreal.MaterialExpressionSaturate, -350, 300)
        MEL.connect_material_expressions(k, "", sat, "")
        op = E(unreal.MaterialExpressionMultiply, -200, 200)
        MEL.connect_material_expressions(sat, "", op, "A")
        MEL.connect_material_expressions(pc, "A", op, "B")
    else:
        # anneau : distance au centre d (0 au centre, 1 au bord) ; trait à d = âge, épaisseur Width ; alpha × (1 - âge)
        uv = E(unreal.MaterialExpressionTextureCoordinate, -1100, 500)
        half = E(unreal.MaterialExpressionConstant2Vector, -1100, 650)
        half.set_editor_property("r", 0.5)
        half.set_editor_property("g", 0.5)
        sub = E(unreal.MaterialExpressionSubtract, -950, 550)
        MEL.connect_material_expressions(uv, "", sub, "A")
        MEL.connect_material_expressions(half, "", sub, "B")
        ln = E(unreal.MaterialExpressionLength, -820, 550)
        MEL.connect_material_expressions(sub, "", ln, "")
        d2 = E(unreal.MaterialExpressionMultiply, -700, 550)
        d2.set_editor_property("const_b", 2.0)
        MEL.connect_material_expressions(ln, "", d2, "A")
        dif = E(unreal.MaterialExpressionSubtract, -580, 550)
        MEL.connect_material_expressions(d2, "", dif, "A")
        MEL.connect_material_expressions(rt, "", dif, "B")
        ab = E(unreal.MaterialExpressionAbs, -460, 550)
        MEL.connect_material_expressions(dif, "", ab, "")
        w = E(unreal.MaterialExpressionScalarParameter, -600, 700)
        w.set_editor_property("parameter_name", "Width")
        w.set_editor_property("default_value", 0.035)
        dv = E(unreal.MaterialExpressionDivide, -340, 550)
        MEL.connect_material_expressions(ab, "", dv, "A")
        MEL.connect_material_expressions(w, "", dv, "B")
        om = E(unreal.MaterialExpressionOneMinus, -230, 550)
        MEL.connect_material_expressions(dv, "", om, "")
        sat = E(unreal.MaterialExpressionSaturate, -120, 550)
        MEL.connect_material_expressions(om, "", sat, "")
        f = E(unreal.MaterialExpressionMultiply, -120, 350)
        MEL.connect_material_expressions(sat, "", f, "A")
        MEL.connect_material_expressions(inv, "", f, "B")
        op = E(unreal.MaterialExpressionMultiply, 0, 300)
        MEL.connect_material_expressions(f, "", op, "A")
        MEL.connect_material_expressions(pc, "A", op, "B")
    MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    ok(True, "matériau " + name)
    return m


def system(name, template, mat, setup):
    path = DOS + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    s = AT.duplicate_asset(name, DOS, unreal.load_asset(template))
    e = L.get_stateless_emitter(s, 0)
    ok(e is not None, name + " : émetteur Lightweight")

    def P(p, v):
        ok(L.set_prop(e, p, v), "%s %s = %s" % (name, p, v))

    def B(p, user, t):
        ok(L.bind_user(s, e, p, user, t), "%s %s -> User.%s" % (name, p, user))

    # Le matériau lit l'âge normalisé (fondu, anneau qui grandit) : l'émetteur Lightweight ne le sort que si on le force.
    P("bForceOutputAllAttributes", "True")
    P("FixedBounds", "(Min=(X=-1500,Y=-1500,Z=-300),Max=(X=1500,Y=1500,Z=800),IsValid=True)")
    P("EmitterState.LoopDuration", "(Min=2.5,Max=2.5,Mode=UniformConstant,ChannelConstantsAndRanges=(2.5))")
    B("Modules[0].LifetimeDistribution", "Life", "float")
    B("Modules[0].ColorDistribution", "Color", "color")
    B("Modules[0].SpriteSizeDistribution", "Size", "vec2")
    P("Modules[0].MassDistribution", "(Min=1,Max=1,Mode=UniformConstant,ChannelConstantsAndRanges=(1.0))")
    # pas de gravité, pas de fondu de couleur ni d'étirement par la vitesse (le matériau fait le fondu)
    for i in (7, 16, 19):
        P("Modules[%d].bModuleEnabled" % i, "False")
    P("RendererProperties[0].Material", mat.get_path_name())
    P("RendererProperties[0].Alignment", "Unaligned")
    setup(s, e, P, B)
    L.finish_system(s, e)
    EAL.save_loaded_asset(s)
    ok(True, "système " + name)
    return s


def gerbe(directional):
    def setup(s, e, P, B):
        B("SpawnInfos[0].Amount", "Count", "int")
        B("Modules[6].DragDistribution", "Drag", "float")
        P("Modules[6].bModuleEnabled", "True")
        P("Modules[2].bModuleEnabled", "True")
        P("Modules[2].ShapePrimitive", "Sphere")
        B("Modules[2].SphereRadius", "Radius", "float")
        P("Modules[3].VelocityType", "InCone")
        B("Modules[3].ConeVelocityDistribution", "Speed", "float")
        P("Modules[3].ConeVelocityScale", "(Min=0.25,Max=1,Mode=UniformRange,ChannelConstantsAndRanges=(0.25,1.0))")
        P("Modules[3].CoordinateSpace", "World")
        if directional:
            P("Modules[3].ConeRotationType", "Direction")
            B("Modules[3].ConeDirection", "Dir", "vec3")
            P("Modules[3].ConeAngle", "126")  # angle total : ±63° (écart de 2,2 rad du proto)
            P("Modules[3].InnerCone", "0")
        else:
            P("Modules[3].ConeRotationType", "Direction")
            P("Modules[3].ConeDirection", "(Min=(X=0,Y=0,Z=1),Max=(X=0,Y=0,Z=1),Mode=NonUniformConstant,ChannelConstantsAndRanges=(0.0,0.0,1.0))")
            P("Modules[3].ConeAngle", "180")  # angle total : demi-sphère
            P("Modules[3].InnerCone", "40")
        P("RendererProperties[0].FacingMode", "FaceCamera")
    return setup


def anneau(s, e, P, B):
    P("SpawnInfos[0].Amount", "(Min=1,Max=1)")
    for i in (2, 3, 6):
        P("Modules[%d].bModuleEnabled" % i, "False")
    P("RendererProperties[0].FacingMode", "CustomFacingVector")
    P("RendererProperties[0].Alignment", "CustomAlignment")
    # à plat sur le sol : la face regarde vers le haut
    P("Modules[22].bModuleEnabled", "True")
    P("Modules[22].bSpriteFacingEnabled", "True")
    P("Modules[22].bSpriteAlignmentEnabled", "True")
    P("Modules[22].SpriteFacing", "(Min=(X=0,Y=0,Z=1),Max=(X=0,Y=0,Z=1),Mode=NonUniformConstant,ChannelConstantsAndRanges=(0.0,0.0,1.0))")
    P("Modules[22].SpriteAlignment", "(Min=(X=1,Y=0,Z=0),Max=(X=1,Y=0,Z=0),Mode=NonUniformConstant,ChannelConstantsAndRanges=(1.0,0.0,0.0))")


unreal.EditorAssetLibrary.make_directory(DOS)
mp = material("M_EON_Particule", False)
ma = material("M_EON_Anneau", True)
T = "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurstLightweight"
system("NS_EON_Gerbe", T, mp, gerbe(False))
system("NS_EON_GerbeDir", T, mp, gerbe(True))
system("NS_EON_Anneau", T, ma, anneau)
import os
open(os.path.join(unreal.Paths.project_saved_dir(), "vfx_build.txt"), "w", encoding="utf-8").write("\n".join(log))
