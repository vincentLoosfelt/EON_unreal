# EON — dalle de sol dans les Blueprints de murs BPA_WallA* (Vincent, 25/11 : les murs n'avaient pas de sol dessous).
# Composant « Sol » : SM_Cube_N (100 cm, pivot au centre) à l'empreinte en tuiles du mur (.eonasset), 40 cm d'épaisseur
# comme le sol des salles, dessus 0,5 cm sous le sol de la salle (pas de scintillement là où la salle a déjà son sol).
# Sans collision : la salle bloque par ses tuiles. Relancer ce script remet la dalle à jour (rien n'est dupliqué).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_murs_dalle.py
import glob, json, os, unreal

SRC = "D:/_PROJETS/briscard/EON_UE/SourceAssets/FBX/enviro/wall"
CUBE = unreal.load_asset("/Game/EON/Meshes/SM_Cube_N")
MAT = unreal.load_asset("/Game/EON/Materials/Peint/MI_Peint_Grille01_B")
EP, DESSOUS, TILE = 40.0, 0.5, 100.0
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary
LOG = lambda s: unreal.log("EON_DALLE " + s)

for f in sorted(glob.glob(SRC + "/*.eonasset")):
    a = json.load(open(f, encoding="utf-8"))
    bp_path = a["unreal"].split(".")[0]
    bp = unreal.load_asset(bp_path)
    if not bp:
        LOG("%s : Blueprint introuvable" % bp_path)
        continue
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    comp = None
    for h in handles:
        o = SDL.get_object(SDL.get_data(h))
        if isinstance(o, unreal.StaticMeshComponent) and o.get_name().startswith("Sol"):
            comp = o
    if comp is None:
        h, fail = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=unreal.StaticMeshComponent, blueprint_context=bp))
        if not fail.is_empty():
            LOG("%s : ajout raté %s" % (bp_path, fail))
            continue
        sub.rename_subobject(h, unreal.Text("Sol"))
        comp = SDL.get_object(SDL.get_data(h))
    ex, ey = a["empreinte"]
    cx, cy = a["centre_cm"]
    sol = float(a.get("sol_cm", 0.0))
    # Origine du Blueprint : centre du maillage, au pied du maillage ; la salle le pose à (centre empreinte − centre_cm), z = −sol_cm.
    comp.set_editor_property("static_mesh", CUBE)
    comp.set_material(0, MAT)
    comp.set_editor_property("relative_location", unreal.Vector(cx, cy, sol - DESSOUS - EP * 0.5))
    comp.set_editor_property("relative_scale3d", unreal.Vector(ex * TILE / 100.0, ey * TILE / 100.0, EP / 100.0))
    comp.set_collision_profile_name("NoCollision")
    comp.set_editor_property("can_ever_affect_navigation", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    ok = unreal.EditorAssetLibrary.save_asset(bp_path, only_if_is_dirty=False)
    LOG("%s : dalle %dx%d tuiles, centre (%.1f, %.1f), dessus à z %.1f, enregistré %s" % (os.path.basename(bp_path), ex, ey, cx, cy, sol - DESSOUS, ok))
