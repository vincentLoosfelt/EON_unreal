# EON — mesure la torsion des jambes de Jira : genou (plan de flexion) vs pied, gauche et droite,
# dans Jira_rig_v3.blend (origine) puis Jira_UEFN.blend (squelette UEFN), + symétrie du maillage des jambes.
import bpy, math, mathutils
V = mathutils.Vector
J = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
def flat(v): return V((v.x, v.y, 0)).normalized()
def ang(a, b):  # angle signé autour de Z, a vers b
    a, b = flat(a), flat(b); return math.degrees(math.atan2(a.cross(b).z, a.dot(b)))
for f, names in (("Jira_rig_v3.blend", {"L": ("leg_up.L", "leg_down.L", "foot.L", "toes.L"), "R": ("leg_up.R", "leg_down.R", "foot.R", "toes.R")}),
                 ("Jira_UEFN.blend", {"L": ("thigh_l", "calf_l", "foot_l", "ball_l"), "R": ("thigh_r", "calf_r", "foot_r", "ball_r")})):
    bpy.ops.wm.open_mainfile(filepath=J + f)
    arm = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
    mw = arm.matrix_world
    print("@@", f, arm.name)
    for s, (t, c, ft, b) in names.items():
        B = arm.data.bones
        hip = mw @ B[t].head_local; knee = mw @ B[c].head_local; ank = mw @ B[ft].head_local; toe = mw @ B[b].head_local
        mid = hip.lerp(ank, ((knee - hip).length) / ((knee - hip).length + (ank - knee).length))
        kd = knee - mid   # direction du genou (vers l'avant si plié vers l'avant)
        fd = toe - ank
        # axes locaux des os : X et Z en monde
        ax = {n: (mw.to_3x3() @ B[n].matrix_local.to_3x3()) for n in (t, c)}
        print("@@ %s hanche %s genou %s cheville %s orteil %s" % (s, tuple(round(x, 3) for x in hip), tuple(round(x, 3) for x in knee), tuple(round(x, 3) for x in ank), tuple(round(x, 3) for x in toe)))
        print("@@ %s flexion genou %.2f cm  dir genou yaw %.1f°  dir pied yaw %.1f°  écart genou/pied %.1f°" % (
            s, kd.length * (100 if f.startswith("Jira_rig") else 1), math.degrees(math.atan2(kd.x, -kd.y)) if kd.length > 1e-6 else float('nan'),
            math.degrees(math.atan2(fd.x, -fd.y)), ang(fd, kd) if kd.length > 1e-6 else float('nan')))
        for n in (t, c):
            M = ax[n]; print("@@   %s axe Y(long) %s  X %s  Z %s" % (n, tuple(round(x, 3) for x in M.col[1]), tuple(round(x, 3) for x in M.col[0]), tuple(round(x, 3) for x in M.col[2])))
    # symétrie maillage : sommets des jambes x<0 vs miroir x>0
    for o in bpy.data.objects:
        if o.type == 'MESH' and o.name.startswith("legs"):
            vs = [o.matrix_world @ v.co for v in o.data.vertices]
            L = [v for v in vs if v.x > 0]; R = [v for v in vs if v.x < 0]
            from mathutils import kdtree
            kd_ = kdtree.KDTree(len(R))
            for i, v in enumerate(R): kd_.insert(v, i)
            kd_.balance()
            d = sorted(kd_.find(V((-v.x, v.y, v.z)))[2] for v in L)
            print("@@ maillage %s : %d/%d sommets, écart miroir médian %.4f max %.4f" % (o.name, len(L), len(R), d[len(d)//2], d[-1]))
