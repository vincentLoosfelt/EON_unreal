# Sonde : sections de COMv1 (matériau, hauteur des sommets en repère monde du HUB, collision).
import unreal
sm = unreal.load_asset("/Game/EON/Unity/import/fbx/enviro/COMv1")
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
act = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "COMv1"][0]
xf = act.get_actor_transform()
out = ["COM %s" % xf]
mats = sm.get_editor_property("static_materials")
for i in range(sm.get_num_sections(0)):
    v, tri, nrm, uv, tan = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(sm, 0, i)
    zs = [xf.transform_location(p).z for p in list(v)[::7]]
    ok = sm.get_section_info_map().get(0, i).enable_collision if False else sm.get_editor_property("section_info_map") if False else "?"
    mi = mats[i].get_editor_property("material_interface") if i < len(mats) else None
    out.append("section %d mat=%s z=%.0f..%.0f n=%d coll=%s" % (i, mi.get_name() if mi else None, min(zs), max(zs), len(v), ok))
open(unreal.Paths.project_saved_dir() + "Logs/eon_com_probe.txt", "w").write("\n".join(out))
