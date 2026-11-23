# EON — inventaire des matériaux du HUB et de leurs paramètres d'émission (lecture seule).
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MEL = unreal.MaterialEditingLibrary
vu = {}
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.MeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m:
                vu.setdefault(m.get_path_name(), [m, 0])[1] += 1
L = []
for p, (m, n) in sorted(vu.items()):
    ligne = "%s  x%d" % (p, n)
    if isinstance(m, unreal.MaterialInstance):
        par = m.get_base_material().get_name()
        vec = []
        for nm in MEL.get_vector_parameter_names(m):
            v = MEL.get_material_instance_vector_parameter_value(m, nm)
            vec.append("%s=(%.2f,%.2f,%.2f)" % (nm, v.r, v.g, v.b))
        sca = []
        for nm in MEL.get_scalar_parameter_names(m):
            if "mis" in str(nm).lower() or "glow" in str(nm).lower() or "int" in str(nm).lower():
                sca.append("%s=%.2f" % (nm, MEL.get_material_instance_scalar_parameter_value(m, nm)))
        tex = []
        for nm in MEL.get_texture_parameter_names(m):
            t = MEL.get_material_instance_texture_parameter_value(m, nm)
            if t and "mis" in str(nm).lower():
                tex.append("%s=%s" % (nm, t.get_name()))
        ligne += "  [%s] %s %s %s" % (par, " ".join(vec), " ".join(sca), " ".join(tex))
    L.append(ligne)
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_hub_emissifs.txt", "w", encoding="utf-8").write("\n".join(L))
