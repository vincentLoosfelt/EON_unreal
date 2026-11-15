# EON — M_PP_EON_Peinture : met à jour, sur place, le code du nœud Custom avec celui d'eon_painted_pp.py (lectures
# bornées à la zone visible, 15/11). Sur place : les PostProcessVolume des cartes gardent leur référence au matériau.
# Éditeur fermé : UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=Saved/Scripts/eon_pp_bords.py
import re, unreal
MEL = unreal.MaterialEditingLibrary
src = open("D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_painted_pp.py", encoding="utf-8").read()
code = re.search(r"CODE = r'''(.*?)'''", src, re.S).group(1)
m = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Peinture")
cu = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.log("EON_PPB nœud de l'émission : %s" % (cu.get_class().get_name() if cu else None))
if isinstance(cu, unreal.MaterialExpressionCustom):
    cu.set_editor_property("code", code)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    unreal.log("EON_PPB code mis à jour (%d lignes, bornes : %s)" % (code.count("\n"), "uvMin" in code))
