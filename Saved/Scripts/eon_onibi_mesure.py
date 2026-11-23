# EON — lecture seule : hauteur du modèle SK_Onibi et position des os (cou, tête) en pose de référence, pour caler
# sa taille sur une vraie tête (Vincent, 23/11). Sortie : Saved/Logs/eon_onibi_mesure.txt
import unreal
sk = unreal.load_asset("/Game/EON/Ennemis/Onibi/SK_Onibi")
b = sk.get_bounds()
L = ["bounds origin %s extent %s" % (b.origin, b.box_extent)]
a = unreal.EditorActorSubsystem
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
act = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0))
c = act.skeletal_mesh_component
c.set_skinned_asset_and_update(sk)
for n in c.get_all_socket_names():
    p = c.get_socket_location(n)
    L.append("%s %.1f %.1f %.1f" % (n, p.x, p.y, p.z))
open("D:/_PROJETS/briscard/EON_UE/Saved/Logs/eon_onibi_mesure.txt", "w", encoding="utf-8").write("\n".join(L))
