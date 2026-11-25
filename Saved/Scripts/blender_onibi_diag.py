# EON — lecture seule : diagnostic du skin de l'Onibi (Kamikaze_run.blend). Os non déformants portant des poids,
# sommets sans poids de déformation, contraintes des os, os animés par l'action.
#   blender -b Kamikaze_run.blend --python blender_onibi_diag.py
import bpy
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
print("@@ARM", arm.name, "os", len(arm.data.bones), "deform", sum(b.use_deform for b in arm.data.bones))
deform = {b.name for b in arm.data.bones if b.use_deform}
for o in bpy.data.objects:
    if o.type != "MESH": continue
    mods = [m for m in o.modifiers if m.type == "ARMATURE"]
    print("@@MESH", o.name, "verts", len(o.data.vertices), "armmods", [(m.object.name if m.object else None) for m in mods], "parent", o.parent.name if o.parent else None, o.parent_type, o.parent_bone)
    if not mods: continue
    idx = {g.index: g.name for g in o.vertex_groups}
    nondef = {}; zero = 0
    for v in o.data.vertices:
        w = 0.0
        for g in v.groups:
            n = idx.get(g.group)
            if g.weight <= 0: continue
            if n in deform: w += g.weight
            else: nondef[n] = nondef.get(n, 0) + 1
        if w < 1e-4: zero += 1
    print("@@  sans poids deform:", zero, " poids sur os non deform/absents:", sorted(nondef.items(), key=lambda x: -x[1])[:20])
for pb in arm.pose.bones:
    if pb.constraints:
        print("@@CON", pb.name, "deform" if pb.bone.use_deform else "ctrl", [(c.type, getattr(c, "subtarget", "")) for c in pb.constraints])
a = arm.animation_data.action if arm.animation_data else None
if a:
    import re
    bones = sorted({re.findall(r'pose.bones\["(.+?)"\]', f.data_path)[0] for f in a.fcurves if "pose.bones" in f.data_path})
    print("@@ACTION", a.name, a.frame_range[:], "os animés", len(bones))
    print("@@  ", bones)
print("@@DEFORM", sorted(deform))
