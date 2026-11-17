# EON — lancé par Unreal au démarrage de l'éditeur : menu « EON Salles » dans la barre de menus principale.
import unreal


def _entry(menu, name, label, tip, cmd):
    e = unreal.ToolMenuEntry(name=name, type=unreal.MultiBlockType.MENU_ENTRY)
    e.set_label(label)
    e.set_tool_tip(tip)
    e.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", cmd)
    menu.add_menu_entry("EONSalles", e)


def _menu():
    menus = unreal.ToolMenus.get()
    main = menus.find_menu("LevelEditor.MainMenu")
    if not main:
        return
    m = main.add_sub_menu("EON", "EONSalles", "EONSalles", "EON Salles", "Salles dessinées (.eonsalle) <-> Blueprints")
    _entry(m, "Ouvrir", "Ouvrir l'éditeur de salles", "Ouvre Tools/EditeurSalles/EditeurSalles.html dans le navigateur",
           "import eon_salles; eon_salles.open_editor()")
    _entry(m, "Importer", "Importer toutes les salles (.eonsalle -> Blueprint)", "Crée ou met à jour /Game/EON/Salles/BP_<nom> pour chaque fichier du dossier Salles",
           "import eon_salles; eon_salles.import_all()")
    _entry(m, "Changees", "Importer les salles modifiées", "Seulement les fichiers changés depuis le dernier import",
           "import eon_salles; eon_salles.import_all(only_changed=True)")
    _entry(m, "Exporter", "Exporter les Blueprints vers .eonsalle", "Réécrit le dossier Salles depuis les Blueprints de /Game/EON/Salles (après une retouche dans Unreal)",
           "import eon_salles; eon_salles.export_all()")
    _entry(m, "Assets", "Générer la bibliothèque d'assets (.eonasset)", "Crée les Blueprints BPA_* manquants dans /Game/EON/RoomAssets et réécrit les .eonasset de SourceAssets/FBX",
           "import eon_assets; eon_assets.generate_all()")
    _entry(m, "Drive", "Publier la bibliothèque sur le Drive", "Copie bibliothèque d'assets, textures et éditeur dans G:/My Drive/EON_EditeurSalles (les salles y sont déjà : dossier maître)",
           "import eon_salles; eon_salles.publish_drive()")
    _entry(m, "Auto", "Réimport automatique (marche / arrêt)", "Toutes les 2 s, réimporte les fichiers .eonsalle enregistrés depuis l'éditeur de salles",
           "import eon_salles; eon_salles.toggle_watch()")
    menus.refresh_all_widgets()


try:
    if unreal.is_editor() if hasattr(unreal, "is_editor") else True:
        _menu()
except Exception as e:  # le menu ne doit jamais bloquer le démarrage
    unreal.log_warning("EON Salles : menu non créé (%s)" % e)
