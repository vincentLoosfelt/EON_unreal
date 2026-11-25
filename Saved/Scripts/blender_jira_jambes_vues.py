# EON — vues de face / dessus / profil des jambes de Jira (pose de repos) : Jira_rig_v3 (origine) et Jira_UEFN.
#   blender -b --factory-startup --python blender_jira_jambes_vues.py -- <dossier_sortie>
import bpy, sys, math, mathutils
V = mathutils.Vector
OUT = sys.argv[sys.argv.index("--") + 1]
J = "D:/_PROJETS/briscard/EON_UE/Saved/Jira/"
for f in ("Jira_rig_v3.blend", "Jira_UEFN.blend"):
    bpy.ops.wm.open_mainfile(filepath=J + f)
    sc = bpy.context.scene
    arm = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
    if arm.animation_data: arm.animation_data.action = None
    for pb in arm.pose.bones: pb.matrix_basis = mathutils.Matrix()
    bpy.context.view_layer.update()
    ms = [o for o in bpy.data.objects if o.type == 'MESH' and not o.hide_render]
    dg = bpy.context.evaluated_depsgraph_get()
    pts = []
    for o in ms:
        e = o.evaluated_get(dg); pts += [e.matrix_world @ v.co for v in e.data.vertices]
    lo = V((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = V((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    c = (lo + hi) / 2; H = hi.z - lo.z
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'MATERIAL'
    sc.display.shading.show_cavity = True
    sc.render.resolution_x, sc.render.resolution_y = 600, 900
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); sc.collection.objects.link(cam); sc.camera = cam
    cam.data.type = 'ORTHO'
    leg = lo.z + H * 0.38   # centre des jambes
    for nom, pos, rot, scale, ctr in (
            ("face", V((c.x, lo.y - H * 3, leg)), (math.pi / 2, 0, 0), H * 0.8, None),
            ("dos", V((c.x, hi.y + H * 3, leg)), (math.pi / 2, 0, math.pi), H * 0.8, None),
            ("profil", V((lo.x - H * 3, c.y, leg)), (math.pi / 2, 0, -math.pi / 2), H * 0.8, None),
            ("dessous", V((c.x, c.y, lo.z - H * 3)), (math.pi, 0, 0), (hi.y - lo.y) * 1.6 + H * 0.2, None)):
        cam.location = pos; cam.rotation_euler = rot; cam.data.ortho_scale = scale
        cam.data.clip_end = H * 10
        if nom == "dessous":   # ne garder que les jambes (sous la taille) visibles par en dessous
            pass
        sc.render.filepath = "%s/%s_%s.png" % (OUT, f.split(".")[0], nom)
        bpy.ops.render.render(write_still=True)
    print("@@", f, "lo", tuple(round(x, 2) for x in lo), "hi", tuple(round(x, 2) for x in hi))
