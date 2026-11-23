# EON — branche le trait celluloïd retenu (Vincent, 23/11 : cumul des effets + ombre décalée E1) dans les cartes et
# supprime les essais refusés : passe gouache M_PP_EON_Gouache (le décor passera par le shader de ses matériaux, doc
# de Vincent à venir) et contour en coque M_EON_ContourCel. Restent de l'essai précédent, voulus :
#   effets M_EON_Particule / M_EON_Anneau / M_EON_Arc / M_EON_ChampForce en translucidité « après DOF » (au-dessus du
#   trait) ; DT_GameFeel PPPaint = True (poids du blendable piloté par AEONPlayerController::ApplyPostProcess).
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_trait_branche.py
import unreal
EAL = unreal.EditorAssetLibrary
LOG = []


def log(m):
    LOG.append(str(m)); unreal.log("EON_TRB " + str(m))


trait = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Trait")
film = unreal.load_asset("/Game/EON/Materials/M_PP_EON_Film")   # passe finale F4 retenue (eon_film.py)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for carte in ("/Game/EON/Maps/L_HUB", "/Game/EON/Maps/L_Etage"):
    les.load_level(carte)
    for v in eas.get_all_level_actors():
        if isinstance(v, unreal.PostProcessVolume):
            s = v.get_editor_property("settings")
            s.set_editor_property("weighted_blendables", unreal.WeightedBlendables([unreal.WeightedBlendable(1.0, trait), unreal.WeightedBlendable(1.0, film)]))
            v.set_editor_property("settings", s)
            log("%s %s : blendables = M_PP_EON_Trait + M_PP_EON_Film" % (carte, v.get_actor_label()))
    log("%s enregistrée : %s" % (carte, les.save_current_level()))
les.load_level("/Game/EON/Maps/L_Sandbox")
for p in ("/Game/EON/Materials/M_PP_EON_Gouache", "/Game/EON/Materials/M_EON_ContourCel"):
    refs = unreal.AssetRegistryHelpers.get_asset_registry().get_referencers(p.rsplit("/", 1)[0] + "/" + p.rsplit("/", 1)[1],
                                                                           unreal.AssetRegistryDependencyOptions()) or []
    log("%s référencé par %s" % (p, [str(r) for r in refs]))
    log("%s supprimé : %s" % (p, EAL.delete_asset(p) if EAL.does_asset_exist(p) else "absent"))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_trait_branche.txt", "w", encoding="utf-8").write("\n".join(LOG))
