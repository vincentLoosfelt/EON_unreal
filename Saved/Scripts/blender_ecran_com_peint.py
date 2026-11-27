# EON — module d'écrans du C.O.M avec des emplacements de matériaux comme le concept art (Vincent, 26/11 : trois teintes
# gris / gris bleu / beige, tuyau orange au sol, câbles sombres du pilier, écrans ambre).
# Dans Asset_Ecran_Com (SM_Ecran_Com.fbx), la plupart des objets n'ont pas de matériau : dans Unreal ils tombent tous dans
# un seul emplacement (Material_0). On repart du même FBX (même pivot, mêmes UV) et on range chaque objet dans un groupe :
#   Coque, Metal, Sombre, Cables, Tuyau, Ecran, Voyant_Vert, Voyant_Rouge, Voyant_Jaune.
# Les objets qui avaient déjà des matériaux par face les gardent, renommés vers ces groupes.
# Sortie : SourceAssets/Modules/Com/SM_Ecran_Com_Peint.fbx (SM_Ecran_Com.fbx n'est pas modifié).
#   blender -b -P blender_ecran_com_peint.py
import bpy

SRC = "D:/_PROJETS/briscard/EON_UE/SourceAssets/Modules/Com/SM_Ecran_Com.fbx"
OUT = "D:/_PROJETS/briscard/EON_UE/SourceAssets/Modules/Com/SM_Ecran_Com_Peint.fbx"

# Matériaux d'origine (par face) -> groupe
PAR_MAT = {"Metal_white": "Coque", "Plastic_very_light_grey": "Coque", "Metal_grey": "Metal", "Plastic_dark_grey": "Sombre",
           "Metal_dark_grey": "Sombre", "Light_screen": "Ecran", "Light_green": "Voyant_Vert", "Light_red": "Voyant_Rouge",
           "Light_yellow": "Voyant_Jaune"}


# Objets sans matériau -> groupe, d'après leur nom (premier préfixe qui correspond, en minuscules).
def groupe_objet(nom):
    n = nom.lower().strip()
    if n.startswith(("tuyaux 1", "tuyaux 2", "entrée/sorti tuyaux", "bloc tuyaux", "enrée tuyaux", "entrée tuyaux")):
        return "Tuyau"            # tuyau orange qui court au sol devant le poste
    if n.startswith(("petit cable", "cables", "gros cables", "cables dessous")):
        return "Cables"           # faisceaux du pilier et sous les tables
    if n.startswith(("tuyaux", "gros tuyaux", "double tuyaux", "ressort tuyaux", "attache tuyaux", "machine", "details machine",
                     "vent machine", "mur electric", "tables", "bas table", "dessous tables")):
        return "Metal"
    if n.startswith(("clavier", "claviers", "regulateur", "manette", "pied manette", "bouton", "dessous bouton", "support bouton",
                     "support touche", "disque dur", "entrée disque", "details", "détails", "souris", "jauge", "mini ecran", "ecran",
                     "plaque", "tableau")):
        return "Sombre"
    return "Coque"                # murs, bornes, supports, pied, pilier (centre dessus / dessous), planche


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)
mats = {}


def mat(g):
    if g not in mats:
        mats[g] = bpy.data.materials.new(g)
    return mats[g]


VOYANTS = ["Voyant_Vert", "Voyant_Rouge", "Voyant_Jaune"]


def pieces(o):
    """Pièces détachées de l'objet : liste de (plus grande dimension en m, indices des faces)."""
    import bmesh
    bm = bmesh.new(); bm.from_mesh(o.data); bm.transform(o.matrix_world); bm.faces.ensure_lookup_table()
    vu, res = set(), []
    for f in bm.faces:
        if f.index in vu:
            continue
        pile, comp = [f], []
        vu.add(f.index)
        while pile:
            g = pile.pop(); comp.append(g.index)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in vu:
                        vu.add(h.index); pile.append(h)
        vs = [v.co for i in comp for v in bm.faces[i].verts]
        res.append((max(max(v[a] for v in vs) - min(v[a] for v in vs) for a in range(3)), comp))
    bm.free()
    return res


compte = {}
for o in [o for o in bpy.context.scene.objects if o.type == "MESH"]:
    me = o.data
    olds = [m.name.split(".")[0] if m else None for m in me.materials]
    if any(olds):
        groupes = [PAR_MAT.get(n, "Coque") for n in olds]
    else:
        groupes = [groupe_objet(o.name)]
    par_face = [groupes[p.material_index] if p.material_index < len(groupes) else groupes[0] for p in me.polygons]
    # Boutons des ordinateurs (Vincent, 26/11) : touches des claviers / pavés (pièces détachées de moins de 6 cm)
    # et objets « Bouton » -> matériaux des voyants lumineux, vert / rouge / jaune mélangés.
    n = o.name.lower()
    if n.startswith("clavier"):
        for k, comp in enumerate(pieces(o)):
            if comp[0] < 0.06:
                for fi in comp[1]:
                    par_face[fi] = VOYANTS[(k * 7 + len(o.name)) % 3]
    elif n.startswith("bouton"):
        par_face = ["Voyant_Rouge"] * len(par_face)
    uniq = sorted(set(par_face))
    idx = {g: i for i, g in enumerate(uniq)}
    me.materials.clear()          # attention : clear() remet toutes les faces sur l'indice 0
    for g in uniq:
        me.materials.append(mat(g))
    for p, g in zip(me.polygons, par_face):
        p.material_index = idx[g]
    for g in uniq:
        compte[g] = compte.get(g, 0) + 1
print("GROUPES", compte)
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=False, object_types={"MESH"}, apply_unit_scale=True,
                         bake_space_transform=False, mesh_smooth_type="FACE", use_mesh_modifiers=True)
print("EXPORT", OUT)
