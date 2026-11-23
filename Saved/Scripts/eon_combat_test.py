# EON — combat de test (Vincent, 24/11 : « lance un combat test avec les persos »). PIE sur L_Etage, pilote
# automatique de Jira (eon.auto 1), salle de combat verrouillée (eon.combat N), captures régulières pendant le combat.
#   UnrealEditor.exe EON.uproject -ExecCmds="py D:/_PROJETS/briscard/EON_UE/Saved/Scripts/eon_combat_test.py"
import os, unreal
CARTE = os.environ.get("EON_COMBAT_CARTE", "/Game/EON/Maps/L_Etage")
VAGUES = os.environ.get("EON_COMBAT_VAGUES", "3")
OUT = "D:/_PROJETS/briscard/EON_UE/Saved/Shots/Combat"
os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):
    if f.startswith("combat_"):
        os.remove(os.path.join(OUT, f))
NB, PAS = int(os.environ.get("EON_COMBAT_NB", "10")), 150          # captures toutes les 150 images d'éditeur
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level(CARTE)
LOG = []
st = {"n": 0, "k": 0}


def log(m):
    LOG.append(str(m)); unreal.log("EON_COMBAT " + str(m))


def monde():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def cmd(c):
    unreal.SystemLibrary.execute_console_command(monde(), c); log("> " + c)


def compter():
    w = monde()
    if not w:
        return
    noms = {}
    for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character):
        n = a.get_class().get_name()
        noms[n] = noms.get(n, 0) + 1
    log("personnages : %s" % noms)


def tick(dt):
    st["n"] += 1
    n = st["n"]
    if n == 300:
        les.editor_request_begin_play()
    elif n == 600:
        cmd("eon.auto 1")
    elif n == 900:
        cmd("eon.combat " + VAGUES)
    elif n >= 1000 and (n - 1000) % PAS == 0 and st["k"] < NB:
        compter()
        cmd("HighResShot 1920x1080 filename=%s/combat_%02d.png" % (OUT, st["k"]))
        st["k"] += 1
    elif st["k"] >= NB and n == 1000 + NB * PAS + 100:
        compter()
        les.editor_request_end_play()
    elif st["k"] >= NB and n == 1000 + NB * PAS + 250:
        open(OUT + "/combat_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
        unreal.unregister_slate_post_tick_callback(h)
        unreal.SystemLibrary.quit_editor()


h = unreal.register_slate_post_tick_callback(tick)
