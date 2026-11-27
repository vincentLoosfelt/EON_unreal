import importlib, json, os, unreal
import eon_assets
importlib.reload(eon_assets)
eon_assets._exported.clear()
eon_assets.generate_theme()
th = json.load(open(os.path.join(eon_assets.source_dir(), "Salle.eontheme"), encoding="utf-8"))
f = th.get("facade") or {}
unreal.log("EONTHEME facade min %s max %s pieces %s" % (f.get("min_cm"), f.get("max_cm"), [(p.get("slot"), p.get("couleur"), p.get("texture"), p.get("emission"), len(p["t"]) // 3) for p in f.get("pieces", [])]))
