# EON — squelette du mannequin UEFN en JSON (nom, parent, transform global de la pose de référence, en cm)
import unreal, json
sk = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
out = {"skeleton": sk.get_editor_property("skeleton").get_path_name(), "bones": []}
m = unreal.SkeletonModifier()
m.set_skeletal_mesh(sk)
for n in m.get_all_bone_names():
    t = m.get_bone_transform(n, True)
    tl, r = t.translation, t.rotation
    out["bones"].append({"name": str(n), "parent": str(m.get_parent_name(n)),
                         "t": [tl.x, tl.y, tl.z], "q": [r.x, r.y, r.z, r.w]})
json.dump(out, open("D:/_PROJETS/briscard/EON_UE/Saved/Jira/uefn_skel.json", "w"), indent=0)
