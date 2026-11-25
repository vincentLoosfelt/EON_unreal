# EON — relevé des modules d'un émetteur Lightweight (préparation de la fumée toon, 24/11).
import unreal, os
s = unreal.load_asset("/Niagara/DefaultAssets/Templates/Systems/DirectionalBurstLightweight")
e = unreal.EONVfxLib.get_stateless_emitter(s, 0)
open(os.path.join(unreal.Paths.project_saved_dir(), "vfx_dump.txt"), "w", encoding="utf-8").write(unreal.EONVfxLib.dump_object(e))
