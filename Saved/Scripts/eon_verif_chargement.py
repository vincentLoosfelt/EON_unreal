# EON — vérifie que la carte L_HUB, le pion BP_Jira, l'ABP de locomotion et SK_Jira_UEFN se chargent et compilent.
import unreal
EAL = unreal.EditorAssetLibrary
L = []
for p in ("/Game/EON/Maps/L_HUB", "/Game/EON/Blueprints/BP_Jira", "/Game/Blueprints/SandboxCharacter_CMC_ABP",
          "/Game/EON/Characters/Jira/SK_Jira_UEFN", "/Game/EON/Blueprints/BP_EONGameMode"):
    a = EAL.load_asset(p)
    L.append("%s : %s" % (p, "chargé" if a else "ÉCHEC"))
    if isinstance(a, unreal.Blueprint):
        unreal.BlueprintEditorLibrary.compile_blueprint(a)
        L.append("   statut %s" % a.get_editor_property("status") if hasattr(a, "status") else "   compilé")
ar = unreal.AssetRegistryHelpers.get_asset_registry()
L.append("assets restants sous Jira : %d" % len(ar.get_assets_by_path("/Game/EON/Characters/Jira", recursive=True)))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/verif_chargement.txt", "w", encoding="utf-8").write("\n".join(L))
