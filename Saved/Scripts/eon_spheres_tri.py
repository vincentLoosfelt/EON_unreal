import unreal, os
out=[]
for p in ["/Engine/BasicShapes/Sphere","/Engine/EngineMeshes/Sphere","/Engine/EditorMeshes/EditorSphere","/Engine/EditorMeshes/ArcadeEditorSphere","/Engine/EngineMeshes/ParticleCube","/Engine/EditorMeshes/AssetViewer/Sphere_inversenormals","/Engine/EngineSky/SM_SkySphere","/Engine/EditorLandscapeResources/SplineEditorMesh"]:
    m=unreal.load_asset(p)
    if m: out.append("%s tris=%s r=%.1f"%(p, unreal.EditorStaticMeshLibrary.get_number_verts(m,0) if hasattr(unreal,"EditorStaticMeshLibrary") else "?", m.get_bounds().sphere_radius))
open(os.path.join(unreal.Paths.project_saved_dir(),"spheres.txt"),"w").write("\n".join(out))
