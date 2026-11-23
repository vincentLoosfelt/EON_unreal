# EON — inventaire (lecture seule) des matériaux des blueprints de salles BP_Salle_* : chaque salle est posée dans un
# monde temporaire (L_Etage chargée, non enregistrée), on relève les matériaux de ses composants, puis on la retire.
#   UnrealEditor-Cmd.exe EON.uproject -run=pythonscript -script=D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_salles_materiaux.py
import unreal
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_salles_materiaux.txt"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/EON/Maps/L_Etage")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tous, LOG = {}, []
for a in unreal.EditorAssetLibrary.list_assets("/Game/EON/Salles", False):
    nom = a.rsplit("/", 1)[-1].split(".")[0]
    if not nom.startswith("BP_Salle_"):
        continue
    bp = unreal.EditorAssetLibrary.load_blueprint_class(a.split(".")[0])
    act = eas.spawn_actor_from_class(bp, unreal.Vector(0, 0, -100000))
    for c in act.get_components_by_class(unreal.MeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m:
                tous.setdefault(m.get_path_name().split(".")[0], set()).add(nom)
    eas.destroy_actor(act)
for m in sorted(tous):
    o = unreal.load_asset(m)
    par = o.get_editor_property("parent").get_name() if isinstance(o, unreal.MaterialInstance) and o.get_editor_property("parent") else ""
    LOG.append("%s [%s] : %d salles" % (m, par, len(tous[m])))
open(OUT, "w", encoding="utf-8").write("\n".join(LOG))
