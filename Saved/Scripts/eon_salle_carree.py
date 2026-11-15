# EON — crée BP_Salle_Combat_Carree : salle 32 x 32, 4 portes extérieures posées (composants Porte EON)
# et une porte intérieure qui ferme la réserve du coin sud-est.
import unreal

SRC = "/Game/EON/Salles/BP_Salle_Combat_A"
DST_DIR, DST_NAME = "/Game/EON/Salles", "BP_Salle_Combat_Carree"
DST = DST_DIR + "/" + DST_NAME
W = H = 32

g = [["." for _ in range(W)] for _ in range(H)]
def put(x, y, c):
    if 0 <= x < W and 0 <= y < H: g[y][x] = c
def rect(x, y, w, h, c):
    for yy in range(y, y + h):
        for xx in range(x, x + w): put(xx, yy, c)
# murs du tour
for i in range(W):
    put(i, 0, "#"); put(i, H - 1, "#"); put(0, i, "#"); put(W - 1, i, "#")
# réserve du coin sud-est (porte intérieure dans son mur ouest)
rect(22, 22, 9, 1, "#")
rect(22, 22, 1, 9, "#")
rect(27, 26, 2, 2, "X"); put(24, 29, "K"); put(29, 24, "K")
# quatre piliers autour du centre
for (x, y) in ((9, 9), (21, 9), (9, 21)):
    rect(x, y, 2, 2, "#")
# fosse centrale franchissable au dash, bordée de caisses
rect(14, 14, 4, 4, "=")
for (x, y) in ((12, 15), (19, 16), (15, 12), (16, 19)):
    put(x, y, "X")
# couverts près des portes
rect(4, 5, 3, 1, "X"); rect(25, 5, 3, 1, "X"); rect(4, 26, 1, 3, "X")
# cuves
put(6, 15, "K"); put(26, 12, "K")
LAYOUT = ["".join(r) for r in g]

# Portes : (nom, x cm, y cm, lacet, extérieure, largeur). Repère : pivot = centre, +X = est, +Y = sud.
DOORS = [
    ("Porte_N", -600.0, -1600.0, -90.0, True, 2),   # nord, décalée vers l'ouest
    ("Porte_S", 0.0, 1600.0, 90.0, True, 2),
    ("Porte_E", 1600.0, 0.0, 0.0, True, 2),
    ("Porte_W", -1600.0, 0.0, 180.0, True, 2),
    ("Porte_Reserve", 650.0, 1000.0, 0.0, False, 2),  # flèche vers l'est : passage ouest -> est, porte en travers
]

eal = unreal.EditorAssetLibrary
if eal.does_asset_exist(DST):
    eal.delete_asset(DST)
bp = eal.duplicate_asset(SRC, DST)
bp = unreal.load_asset(DST)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("layout", LAYOUT)
cdo.set_editor_property("shape", "carree")
cdo.set_editor_property("room_role", "COMBAT")
cdo.set_editor_property("extra_roles", ["TENSION"])
cdo.set_editor_property("weight", 6.0)

sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = sub.k2_gather_subobject_data_for_blueprint(bp)
root = handles[0]
door_cls = unreal.load_class(None, "/Script/EON.EONDoorComponent")
for (name, x, y, yaw, ext, larg) in DOORS:
    h, fail = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=door_cls, blueprint_context=bp))
    if not fail.is_empty():
        unreal.log_error("ajout %s : %s" % (name, fail))
        continue
    sub.rename_subobject(h, unreal.Text(name))
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    comp.set_editor_property("relative_location", unreal.Vector(x, y, 60.0))
    comp.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, yaw))
    comp.set_editor_property("exterieure", ext)
    comp.set_editor_property("largeur", larg)

unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_asset(DST, only_if_is_dirty=False)

# relecture
bp = unreal.load_asset(DST)
cdo = unreal.get_default_object(bp.generated_class())
lay = cdo.get_editor_property("layout")
unreal.log("CARREE : %d rangees x %d, forme %s" % (len(lay), len(lay[0]), cdo.get_editor_property("shape")))
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    if isinstance(o, unreal.EONDoorComponent):
        unreal.log("CARREE porte %s loc %s rot %s ext %s" % (o.get_name(), o.get_editor_property("relative_location"), o.get_editor_property("relative_rotation"), o.get_editor_property("exterieure")))
for r in lay:
    unreal.log("CARREE " + r)
