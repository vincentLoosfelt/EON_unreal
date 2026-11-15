# EON — génère Source/EON/Private/EONDialogData.inl depuis DONNEES/dialogues.json (personnages, dialogues, logs, équipage, vieille dame).
import json
R = "D:/_PROJETS/briscard/EON_UE"
d = json.load(open(R + "/Docs/PASSATION_UNREAL/DONNEES/dialogues.json", encoding="utf-8"))
BS = chr(92)
def s(t):
    return 'TEXT("' + t.replace(BS, BS + BS).replace('"', BS + '"') + '")'
o = ["// Généré par Saved/Scripts/gen_dialogues.py depuis DONNEES/dialogues.json. Ne pas éditer à la main.", ""]
o.append("static const FEONChar GChars[] = {")
for k, v in d["personnages"].items():
    c = v["c"].lstrip("#")
    o.append("\t{ %s, %s, FColor(%d, %d, %d) }," % (s(k), s(v["n"]), int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)))
o.append("};")
o.append("static const FEONDlgSrc GDlgs[] = {")
for k, lines in d["dialogues"].items():
    for l in lines:
        o.append("\t{ %s, %s, %s, %s }," % (s(k), s(l["who"]), s(l.get("ex", "neutre")), s(l["t"])))
o.append("};")
for name, key in (("GLogs", "logs"), ("GCrew", "equipage_ulysse"), ("GVieille", "vieille_dame_repliques")):
    o.append("static const TCHAR* %s[] = {" % name)
    for t in d[key]:
        o.append("\t%s," % s(t))
    o.append("};")
open(R + "/Source/EON/Private/EONDialogData.inl", "w", encoding="utf-8-sig").write("\n".join(o) + "\n")
print("ok", len(d["dialogues"]), "dialogues")
