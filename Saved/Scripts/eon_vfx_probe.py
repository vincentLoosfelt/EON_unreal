import unreal, os
L=unreal.EONVfxLib
out=[]
for n in ["NS_EON_Gerbe","NS_EON_GerbeDir","NS_EON_Anneau"]:
    s=unreal.load_asset("/Game/EON/VFX/"+n)
    e=L.get_stateless_emitter(s,0)
    d=L.dump_object(e)
    out.append("==== "+n)
    for l in d.split("\n"):
        if any(k in l for k in ["Mode=Binding","bModuleEnabled = True","Material =","FacingMode","Alignment =","ConeAngle","SpawnInfos","VelocityType"]):
            out.append(l[:200])
open(os.path.join(unreal.Paths.project_saved_dir(),"vfx_probe.txt"),"w",encoding="utf-8").write("\n".join(out))
