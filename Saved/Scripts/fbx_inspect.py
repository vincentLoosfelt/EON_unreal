# Lecteur FBX binaire minimal : Models, matériaux (connexions OO) et transformations locales (Lcl).
#   py -I fbx_inspect.py fichier.fbx      (ou import : fbx_inspect.scan(path))
import struct, sys, zlib, collections

NUL = bytes([0])
data = b""
W = 8


def _load(path):
    global data, W
    data = open(path, "rb").read()
    W = 8 if struct.unpack_from("<I", data, 23)[0] >= 7500 else 4


def read_prop(o):
    t = chr(data[o]); o += 1
    if t in "YCIFDL":
        fmt = {"Y": "<h", "C": "<?", "I": "<i", "F": "<f", "D": "<d", "L": "<q"}[t]
        return struct.unpack_from(fmt, data, o)[0], o + struct.calcsize(fmt)
    if t in "fdlib":
        n, enc, clen = struct.unpack_from("<III", data, o); o += 12
        raw = data[o:o + clen]; o += clen
        if enc == 1:
            raw = zlib.decompress(raw)
        fmt = {"f": "f", "d": "d", "l": "q", "i": "i", "b": "?"}[t]
        return list(struct.unpack("<%d%s" % (n, fmt), raw)), o
    if t in "SR":
        n = struct.unpack_from("<I", data, o)[0]; o += 4
        return data[o:o + n], o + n
    raise ValueError(t)


def read_node(o):
    if W == 8:
        end, nprops, plen = struct.unpack_from("<QQQ", data, o); o += 24
    else:
        end, nprops, plen = struct.unpack_from("<III", data, o); o += 12
    nlen = data[o]; o += 1
    if end == 0:
        return None, o
    name = data[o:o + nlen].decode(); o += nlen
    props = []
    for _ in range(nprops):
        v, o = read_prop(o); props.append(v)
    kids = []
    while o < end:
        k, o2 = read_node(o)
        if k is None:
            o = o2; break
        kids.append(k); o = o2
    return (name, props, kids), end


def scan(path):
    """Tous les Models : dict(name, root, t, r, s, mats). r en degrés (Euler XYZ du FBX)."""
    _load(path)
    nodes, o = [], 27
    while o < len(data) - 200:
        n, o = read_node(o)
        if n is None:
            break
        nodes.append(n)
    top = {n[0]: n for n in nodes}
    objs = {}
    for n in top["Objects"][2]:
        if n[0] in ("Model", "Material", "Geometry"):
            objs[n[1][0]] = (n[0], n[1][1].split(NUL)[0].decode(errors="replace"), n)
    par = collections.defaultdict(list)
    for c in top["Connections"][2]:
        if c[1][0] == b"OO":
            par[c[1][1]].append(c[1][2])
    out = []
    for oid, (kind, name, n) in objs.items():
        if kind != "Model":
            continue
        parent_models = [p for p in par[oid] if p in objs and objs[p][0] == "Model"]
        props = {"Lcl Translation": [0, 0, 0], "Lcl Rotation": [0, 0, 0], "Lcl Scaling": [1, 1, 1]}
        for p70 in n[2]:
            if p70[0] == "Properties70":
                for p in p70[2]:
                    key = p[1][0].decode()
                    if key in props:
                        props[key] = list(p[1][4:7])
        mats = [objs[c][1] for c, ps in par.items() if oid in ps and c in objs and objs[c][0] == "Material"]
        out.append(dict(name=name, root=not parent_models, t=props["Lcl Translation"],
                        r=props["Lcl Rotation"], s=props["Lcl Scaling"], mats=mats))
    return out


if __name__ == "__main__":
    for m in scan(sys.argv[1]):
        print(m)
