# Clavier AZERTY par défaut (Vincent, 26/11) : IA_Move garde ZQSD et les flèches (W et A, le QWERTY, partent) ;
# greffe précédente = X, greffe suivante = V (aucune touche au clavier avant). Les chiffres 1..5 restent One..Five
# dans l'asset : EONOptions::BuildMappingContext les remplace par la touche du clavier actif (& é " ' ( en AZERTY).
# Lancement : UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=<ce fichier> ; compte rendu dans eon_azerty.txt.
import unreal

OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_azerty.txt"
P = "/Game/EON/Input/"
out = open(OUT, "w", encoding="utf-8")
imc = unreal.load_asset(P + "IMC_EON")
ia = {n: unreal.load_asset(P + n) for n in ("IA_GreffePrev", "IA_GreffeNext")}


def nom(m):
    a = m.get_editor_property("action")
    return (a.get_name() if a else "-"), str(m.get_editor_property("key").get_editor_property("key_name"))


def dump(tag):
    maps = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
    out.write("%s : %d touches\n" % (tag, len(maps)))
    for m in maps:
        an, kn = nom(m)
        mods = ",".join(type(x).__name__ for x in m.get_editor_property("modifiers"))
        out.write("  %-16s %-28s %s\n" % (an, kn, mods))


def key(name):
    k = unreal.Key()
    k.import_text(name)
    return k


dump("avant")
dkm = imc.get_editor_property("default_key_mappings")
keep = []
for m in dkm.get_editor_property("mappings"):
    an, kn = nom(m)
    if an == "IA_Move" and kn in ("W", "A"):
        out.write("retire IA_Move %s\n" % kn)
        continue
    if (an, kn) in (("IA_GreffePrev", "X"), ("IA_GreffeNext", "V")):
        continue  # script relancé : pas de doublon
    keep.append(m)
dkm.set_editor_property("mappings", keep)
imc.set_editor_property("default_key_mappings", dkm)
imc.map_key(ia["IA_GreffePrev"], key("X"))
imc.map_key(ia["IA_GreffeNext"], key("V"))
dump("apres")
out.write("save %s\n" % unreal.EditorAssetLibrary.save_asset(P + "IMC_EON", only_if_is_dirty=False))
out.close()
