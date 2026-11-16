# Maj n'est plus une touche de dash (Vincent, 16/11) : retire LeftShift de IA_Dash dans IMC_EON. La course sur Maj est lue en C++.
import unreal
out = open("D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_maj_course.txt", "w")
imc = unreal.load_asset("/Game/EON/Input/IMC_EON")
ia = unreal.load_asset("/Game/EON/Input/IA_Dash")
dkm = imc.get_editor_property("default_key_mappings")
maps = list(dkm.get_editor_property("mappings"))
out.write("avant %d\n" % len(maps))
keep = []
for m in maps:
    kn = str(m.get_editor_property("key").get_editor_property("key_name"))
    an = m.get_editor_property("action").get_name() if m.get_editor_property("action") else "-"
    if an == "IA_Dash" and kn == "LeftShift":
        out.write("retire %s %s\n" % (an, kn)); continue
    if an == "IA_Dash": out.write("garde %s %s\n" % (an, kn))
    keep.append(m)
dkm.set_editor_property("mappings", keep)
imc.set_editor_property("default_key_mappings", dkm)
out.write("apres %d\n" % len(imc.get_editor_property("default_key_mappings").get_editor_property("mappings")))
out.write("save %s\n" % unreal.EditorAssetLibrary.save_asset("/Game/EON/Input/IMC_EON", only_if_is_dirty=False))
out.close()
