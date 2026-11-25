# EON — lecture seule : compare SK_Onibi.fbx (maillage du jeu) et A_Onibi_Run.fbx (course) : os, repos, poids.
#   blender -b --factory-startup --python blender_onibi_sk_diag.py
import bpy
D = "D:/_PROJETS/briscard/EON_UE/SourceAssets/Ennemis/Onibi/"
def load(fn):
    before = set(bpy.data.objects); bpy.ops.import_scene.fbx(filepath=D + fn)
    return [o for o in bpy.data.objects if o not in before]
bpy.ops.wm.read_factory_settings(use_empty=True)
A = load("SK_Onibi.fbx"); B = load("A_Onibi_Run.fbx")
aa = next(o for o in A if o.type == "ARMATURE"); ba = next(o for o in B if o.type == "ARMATURE")
print("@@SK objets", [(o.name, o.type, len(o.data.vertices) if o.type == "MESH" else len(o.data.bones)) for o in A])
print("@@RUN objets", [(o.name, o.type, len(o.data.vertices) if o.type == "MESH" else len(o.data.bones)) for o in B])
na = {b.name: b for b in aa.data.bones}; nb = {b.name: b for b in ba.data.bones}
print("@@os seulement SK", sorted(set(na) - set(nb))); print("@@os seulement RUN", sorted(set(nb) - set(na)))
for n in sorted(set(na) & set(nb)):
    x, y = na[n], nb[n]
    dp = ((aa.matrix_world @ x.head_local) - (ba.matrix_world @ y.head_local)).length
    pa = x.parent.name if x.parent else None; pb = y.parent.name if y.parent else None
    ang = x.matrix_local.to_quaternion().rotation_difference(y.matrix_local.to_quaternion()).angle
    if dp > 0.001 or pa != pb or ang > 0.01: print("@@DIFF", n, "pos %.4f" % dp, "ang %.3f" % ang, "parent", pa, pb)
for ma in [o for o in A if o.type == "MESH"]:
    mb = next((o for o in B if o.type == "MESH" and o.name.split(".")[0] == ma.name.split(".")[0]), None)
    if not mb: print("@@mesh sans pendant", ma.name); continue
    if len(ma.data.vertices) != len(mb.data.vertices): print("@@verts diff", ma.name, len(ma.data.vertices), len(mb.data.vertices)); continue
    ia = {g.index: g.name for g in ma.vertex_groups}; ib = {g.index: g.name for g in mb.vertex_groups}
    nd = 0; ex = None
    for va, vb in zip(ma.data.vertices, mb.data.vertices):
        wa = {ia[g.group]: round(g.weight, 3) for g in va.groups if g.weight > 0.001}
        wb = {ib[g.group]: round(g.weight, 3) for g in vb.groups if g.weight > 0.001}
        if wa != wb or (va.co - vb.co).length > 0.001:
            nd += 1; ex = ex or (va.index, wa, wb, tuple(round(c, 3) for c in va.co), tuple(round(c, 3) for c in vb.co))
    print("@@POIDS", ma.name, "sommets différents", nd, "ex", ex)
