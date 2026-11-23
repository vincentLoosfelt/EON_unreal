# EON — applique le décor peint (variante B aquarelle, choisie par Vincent le 23/11) dans L_HUB.
# Chaque emplacement MI_BG_* (sauf MI_BG_Room_light) passe sur MI_Peint_*_B ; le post-process MI_PP_EON_Decor_B est mis
# en tête des blendables du PostProcessVolume (avant le trait). Les MI_BG_* ne sont pas modifiés : retour possible avec
# EON_PEINT_RETOUR=1 (remet les MI_BG_* d'origine et retire le post-process du décor).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_hub_applique_peint.py
import os, unreal
V = os.environ.get("EON_PEINT_VAR", "B")
RETOUR = os.environ.get("EON_PEINT_RETOUR", "0") == "1"
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_HUB_PEINT " + str(m))


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_HUB")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cache = {}


def cible(mt):
    n = mt.get_name()
    if RETOUR:
        if n.startswith("MI_Peint_"):
            base = n[len("MI_Peint_"):].rsplit("_", 1)[0]
            return unreal.load_asset("/Game/EON/Materials/BG/MI_BG_" + base) or None
        return None
    if n.startswith("MI_BG_") and n != "MI_BG_Room_light":
        if n not in cache:
            cache[n] = unreal.load_asset("/Game/EON/Materials/Peint/" + n.replace("MI_BG_", "MI_Peint_") + "_" + V)
        return cache[n]
    return None


n_ok, n_manque = 0, 0
for act in eas.get_all_level_actors():
    for comp in act.get_components_by_class(unreal.MeshComponent):
        for i in range(comp.get_num_materials()):
            mt = comp.get_material(i)
            if not mt:
                continue
            t = cible(mt)
            if t is None:
                if mt.get_name().startswith("MI_BG_") and mt.get_name() != "MI_BG_Room_light" and not RETOUR:
                    n_manque += 1; log("manque : %s" % mt.get_name())
                continue
            act.modify(); comp.modify()
            comp.set_material(i, t); n_ok += 1
log("emplacements changés : %d ; sans équivalent : %d" % (n_ok, n_manque))

ppd = unreal.load_asset("/Game/EON/Materials/Peint/MI_PP_EON_Decor_" + V)
for ppv in [v for v in eas.get_all_level_actors() if isinstance(v, unreal.PostProcessVolume)]:
    s = ppv.get_editor_property("settings")
    bl = [b for b in s.get_editor_property("weighted_blendables").get_editor_property("array")
          if not (b.get_editor_property("object") and b.get_editor_property("object").get_name().startswith("MI_PP_EON_Decor"))]
    if not RETOUR:
        bl.insert(0, unreal.WeightedBlendable(1.0, ppd))
    ppv.modify()
    s.set_editor_property("weighted_blendables", unreal.WeightedBlendables(bl))
    ppv.set_editor_property("settings", s)
    log("%s : blendables %s" % (ppv.get_actor_label(), [b.get_editor_property("object").get_name() for b in bl if b.get_editor_property("object")]))

log("sauvegarde : %s" % les.save_current_level())
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_hub_applique_peint.txt", "w", encoding="utf-8").write("\n".join(LOG))
