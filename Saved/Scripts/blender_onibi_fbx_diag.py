# EON — lecture seule : compare la course de l'Onibi dans Blender (Kamikaze_run.blend) et dans un FBX exporté
# (EON_FBX, défaut SourceAssets/Ennemis/Onibi/A_Onibi_Run.fbx), sommet par sommet ; les sommets NaN sont écartés.
#   blender -b Kamikaze_run.blend --python blender_onibi_fbx_diag.py
import bpy, os, math
from mathutils import Vector
FBX = os.environ.get("EON_FBX", "D:/_PROJETS/briscard/EON_UE/SourceAssets/Ennemis/Onibi/A_Onibi_Run.fbx")
sc = bpy.context.scene
body = bpy.data.objects["body"]
s, e = bpy.data.objects["Armature"].animation_data.action.frame_range
FR = list(range(int(s), int(e) + 1))
def coords(o):
    dg = bpy.context.evaluated_depsgraph_get()
    x = o.evaluated_get(dg); m = x.to_mesh()
    c = [x.matrix_world @ v.co for v in m.vertices]; x.to_mesh_clear(); return c
ref = {}
for f in FR:
    sc.frame_set(f); ref[f] = coords(body)
nan = {i for f in FR for i, p in enumerate(ref[f]) if any(math.isnan(t) for t in p)}
print("@@NAN dans Blender", len(nan))
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=FBX)
new = [o for o in bpy.data.objects if o not in before]
nb = next(o for o in new if o.type == "MESH" and len(o.data.vertices) == len(body.data.vertices))
na = next(o for o in new if o.type == "ARMATURE")
print("@@RACINES FBX", [b.name for b in na.data.bones if b.parent is None])
ok = [i for i in range(len(body.data.vertices)) if i not in nan]
idx = {g.index: g.name for g in body.vertex_groups}
grp = {v.index: (idx.get(max(v.groups, key=lambda g: g.weight).group) if v.groups else "-") for v in body.data.vertices}
def ecart(a, b):
    err = {}
    for i in ok:
        d = (a[i] - b[i]).length; err[grp[i]] = max(err.get(grp[i], 0), d)
    return err
# Décalage d'images de l'import FBX : on cherche le meilleur (0 à 2)
best = None
for off in (0, 1, 2):
    tot = 0
    for f in FR[:-2]:
        sc.frame_set(f + off); tot += max(ecart(coords(nb), ref[f]).values())
    print("@@DECALAGE", off, "%.4f" % tot)
    best = off if best is None or tot < best[1] else best
    best = (off, tot) if not isinstance(best, tuple) or tot < best[1] else best
off = best[0]
pire = {}
for f in FR[:-2]:
    sc.frame_set(f + off)
    for g, d in ecart(coords(nb), ref[f]).items(): pire[g] = max(pire.get(g, 0), d)
print("@@PIRE (décalage %d)" % off, ["%s %.4f" % w for w in sorted(pire.items(), key=lambda x: -x[1])[:8]])
amp = {}
for i in ok:
    a = max((ref[f][i] - ref[FR[0]][i]).length for f in FR); amp[grp[i]] = max(amp.get(grp[i], 0), a)
print("@@AMPLITUDE Blender", ["%s %.3f" % (g, amp[g]) for g in ("foot.L", "foot.R", "toe.L", "toe.R", "leg.L", "spine1") if g in amp])
