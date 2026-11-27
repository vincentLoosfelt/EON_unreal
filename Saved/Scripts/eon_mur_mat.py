import unreal
MEL = unreal.MaterialEditingLibrary
for p in ["/Script/EON.Default__EONTileRoom", "/Game/EON/Salles/BP_Salle_Calme_A"]:
    try:
        if p.startswith("/Script"):
            cdo = unreal.get_default_object(unreal.EONTileRoom)
        else:
            cdo = unreal.get_default_object(unreal.load_asset(p).generated_class())
        unreal.log("EONMUR %s hub=%s mat=%s light=%s" % (p, cdo.get_editor_property("hub_walls"), cdo.get_editor_property("hub_wall_material"), cdo.get_editor_property("hub_wall_light_material")))
    except Exception as e:
        unreal.log("EONMUR %s err %s" % (p, e))
for m in ["/Game/EON/Materials/BG/MI_BG_walll_painted", "/Game/EON/Materials/BG/MI_BG_Room_light"]:
    a = unreal.load_asset(m)
    unreal.log("EONMUR %s parent=%s vec=%s tex=%s" % (m, a.get_editor_property("parent"), [(str(n), MEL.get_material_instance_vector_parameter_value(a, n)) for n in MEL.get_vector_parameter_names(a)], [(str(n), MEL.get_material_instance_texture_parameter_value(a, n)) for n in MEL.get_texture_parameter_names(a)]))
sm = unreal.load_asset("/Game/EON/Unity/import/fbx/enviro/wall/WallA320")
for s in sm.get_editor_property("static_materials"):
    unreal.log("EONMUR slot %s -> %s" % (s.material_slot_name, s.material_interface.get_path_name() if s.material_interface else None))
