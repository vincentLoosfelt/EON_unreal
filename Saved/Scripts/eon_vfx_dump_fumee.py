# EON — relevé de l'émetteur des lobes de fumée (durée de vie réelle, 24/11).
import unreal, os
s = unreal.load_asset("/Game/EON/VFX/NS_EON_FumeeLobes")
e = unreal.EONVfxLib.get_stateless_emitter(s, 0)
open(os.path.join(unreal.Paths.project_saved_dir(), "vfx_dump_fumee.txt"), "w", encoding="utf-8").write(unreal.EONVfxLib.dump_object(e))
