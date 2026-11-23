# EON — reprise : A_Onibi_Run_Variation (réexport) et Nanite sur l'écran du C.O.M (907 k triangles).
import unreal, os
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
LOG = []
skel = unreal.load_asset("/Game/EON/Ennemis/Onibi/SKEL_Onibi")
o = unreal.FbxImportUI()
o.automated_import_should_detect_type = False
o.import_mesh = False; o.import_animations = True; o.import_materials = False; o.import_textures = False
o.import_as_skeletal = True; o.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION; o.skeleton = skel
o.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
o.anim_sequence_import_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
t = unreal.AssetImportTask()
t.filename = os.path.join(ROOT, "SourceAssets/Ennemis/Onibi/A_Onibi_Run_Variation.fbx"); t.destination_path = "/Game/EON/Ennemis/Onibi/Anims"
t.destination_name = "A_Onibi_Run_Variation"; t.automated = True; t.replace_existing = True; t.save = True; t.options = o
AT.import_asset_tasks([t])
a = unreal.load_asset("/Game/EON/Ennemis/Onibi/Anims/A_Onibi_Run_Variation")
LOG.append("A_Onibi_Run_Variation : %s" % ("%.2f s" % a.get_play_length() if a else "ÉCHEC"))
for p in ("/Game/EON/Meshes/COM/SM_Ecran_Com", "/Game/EON/Ennemis/Drone/SM_Drone"):
    sm = unreal.load_asset(p)
    ns = sm.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", True)
    sm.set_editor_property("nanite_settings", ns)
    EAL.save_loaded_asset(sm)
    LOG.append("%s : Nanite activé, matériaux %s" % (p, [str(m.get_editor_property("material_slot_name")) for m in sm.get_editor_property("static_materials")]))
open(os.path.join(ROOT, "Saved/Logs/eon_ennemis_fix.txt"), "w", encoding="utf-8").write("\n".join(LOG))
