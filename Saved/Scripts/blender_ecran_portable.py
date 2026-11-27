# EON — dalles d'écran du poste du C.O.M (Vincent, 26/11 : « il y a encore un écran blanc », puis « certains écrans
# n'ont pas le matériau écran »).
# Dans Asset_Ecran_Com, plusieurs faces d'affichage n'ont pas le matériau Light_screen :
#   - le portable incliné (Ecran.004) : sa face d'affichage est en Metal_white, comme sa coque ;
#   - le moniteur double face (Ecran.006), la borne (Ecran.007) et la tablette (Mini ecran) : aucun matériau.
# On ne peut pas leur donner le matériau d'écran sans tout éclairer. On fabrique donc des dalles à part, posées 2 mm
# devant chaque face d'affichage, même pivot que le module, dans un seul maillage.
# UV de chaque dalle dans [k ; k+1] × [0 ; 1] ; M_EON_EcranCRT choisit le contenu d'après k :
#   2 radar, 3 ADN, 4 histogramme, 5 oscilloscope, 6 texte.
#   blender -b -P blender_ecran_portable.py
import bpy, bmesh, math
from mathutils import Vector

SRC = "D:/_PROJETS/briscard/EON_UE/SourceAssets/Modules/Com/SM_Ecran_Com.fbx"
OUT = "D:/_PROJETS/briscard/EON_UE/SourceAssets/Modules/Com/SM_Ecran_Com_Portable.fbx"
Z = Vector((0, 0, 1))
# objet, normale de la face d'affichage, plage du plan (d = position le long de la normale, m), z minimum, marge (m), k
DALLES = [
    ("Ecran.004", (-0.77, 0.64, 0), (1.724, 1.732), -9, 0.006, 2),         # portable incliné : radar
    ("Mini ecran", (0, 0, 1), (1.138, 1.146), -9, 0.022, 3),               # tablette posée à plat : ADN
    ("Ecran.007", (-0.98, -0.18, 0), (0.144, 0.1525), 0.75, 0.004, 4),     # borne, écran du haut : histogramme
    ("Ecran.006", (-0.89, 0.35, 0.29), (1.53, 1.55), -9, 0.004, 5),   # moniteur double face, côté incliné : oscilloscope
    ("Ecran.006", (0.93, -0.36, 0), (-0.8135, -0.8105), -9, 0.004, 6),     # moniteur double face, côté droit : texte
]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)
verts, faces, uvs = [], [], []
for nom, n, (dmin, dmax), zmin, marge, k in DALLES:
    o = bpy.data.objects[nom]
    bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world)
    bm.normal_update()   # normales recalculées (Ecran.006 a une échelle négative : on accepte les deux sens)
    N = Vector(n).normalized()
    fs = [f for f in bm.faces if abs(f.normal.dot(N)) > 0.97 and dmin < f.calc_center_median().dot(N) < dmax and f.calc_center_median().z > zmin]
    vs = [v.co.copy() for f in fs for v in f.verts]
    if abs(N.z) < 0.9:
        U = (Z - N * N.z).normalized()                 # haut de l'écran
    else:                                              # écran à plat : côtés du plus petit rectangle englobant
        best = None
        for a in range(0, 180):
            A = Vector((math.cos(math.radians(a)), math.sin(math.radians(a)), 0))
            B = N.cross(A)
            ar = (max(v.dot(A) for v in vs) - min(v.dot(A) for v in vs)) * (max(v.dot(B) for v in vs) - min(v.dot(B) for v in vs))
            if best is None or ar < best[0]:
                best = (ar, A)
        U = best[1]
    R = U.cross(N).normalized()                        # droite vue de face
    U = N.cross(R).normalized()
    d = sum(v.dot(N) for v in vs) / len(vs) + 0.002
    t0, t1 = min(v.dot(R) for v in vs) + marge, max(v.dot(R) for v in vs) - marge
    s0, s1 = min(v.dot(U) for v in vs) + marge, max(v.dot(U) for v in vs) - marge
    print("DALLE %s k=%d faces %d d=%.4f largeur %.3f hauteur %.3f" % (nom, k, len(fs), d, t1 - t0, s1 - s0))
    P = lambda t, s: N * d + R * t + U * s
    i = len(verts)
    verts += [P(t0, s0), P(t1, s0), P(t1, s1), P(t0, s1)]
    faces.append((i, i + 1, i + 2, i + 3))
    uvs.append([(k, 0), (k + 1, 0), (k + 1, 1), (k, 1)])
    bm.free()

for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob)
me = bpy.data.meshes.new("SM_Ecran_Com_Portable")
me.from_pydata([tuple(v) for v in verts], [], faces)
me.update()
uvl = me.uv_layers.new(name="UV")
for fi, poly in enumerate(me.polygons):
    for j, li in enumerate(poly.loop_indices):
        uvl.data[li].uv = uvs[fi][j]
mat = bpy.data.materials.new("Ecran_CRT")
me.materials.append(mat)
ob = bpy.data.objects.new("SM_Ecran_Com_Portable", me)
bpy.context.scene.collection.objects.link(ob)
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=False, object_types={"MESH"}, apply_unit_scale=True, bake_space_transform=False, mesh_smooth_type="FACE")
print("EXPORT", OUT)
