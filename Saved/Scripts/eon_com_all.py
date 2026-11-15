import os
H = "D:/_PROJETS/briscard/EON_UE/Saved/Scripts/"
for f in ("eon_com_dome.py", "eon_com_materials.py"):
    exec(open(H + f, encoding="utf-8").read(), {"__name__": "__main__"})
