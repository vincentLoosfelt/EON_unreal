# EON — membres coupables : lecture des os et des sommets (os principal de chaque sommet) des 5 ennemis (25/11).
# Sorties : Saved/os_<nom>.txt, Saved/verts_<nom>.txt, lues par eon_membres_table.py.
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal, os
L=[]
SAV = unreal.Paths.project_saved_dir()
for n in ["Kama","Nuke","Onibi","Ubume","Ikiryo"]:
    sk = unreal.load_asset("/Game/EON/Ennemis/%s/SK_%s" % (n, n))
    sm = unreal.SkeletonModifier(); sm.set_skeletal_mesh(sk)
    names = [str(x) for x in sm.get_all_bone_names()]
    with open(os.path.join(SAV, "os_%s.txt" % n), "w") as f:
        for b in names:
            t = sm.get_bone_transform(b, True).translation
            f.write("%s|%s|%.2f|%.2f|%.2f\n" % (b, sm.get_parent_name(b), t.x, t.y, t.z))
    dm = unreal.DynamicMesh()
    unreal.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(sk, dm, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
    nv = unreal.GeometryScript_MeshQueries.get_vertex_count(dm)
    pl = unreal.GeometryScriptBoneWeightProfile()
    with open(os.path.join(SAV, "verts_%s.txt" % n), "w") as f:
        for i in range(nv):
            p = unreal.GeometryScript_MeshQueries.get_vertex_position(dm, i)
            p = p[0] if isinstance(p, tuple) else p
            b = -1
            r = unreal.GeometryScript_BoneWeights.get_largest_vertex_bone_weight(dm, i, pl)
            for x in (r if isinstance(r, tuple) else (r,)):
                if hasattr(x, "bone_index"): b = x.bone_index
            f.write("%.2f %.2f %.2f %d\n" % (p.x, p.y, p.z, b))
    L.append("%s os %d sommets %d" % (n, len(names), nv))
open(os.path.join(SAV, "dump_tous.txt"), "w").write("\n".join(L))
