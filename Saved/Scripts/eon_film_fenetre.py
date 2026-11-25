# EON — filme la fenêtre du jeu (UnrealEditor -game) en images JPG, pour examiner les anims image par image.
# Attend la fenêtre (titre contenant "EON"), puis capture DUREE secondes à FPS images/s, à moitié de la taille.
# Lecture par PrintWindow (PW_RENDERFULLCONTENT) : le contenu de la fenêtre du jeu, même recouverte par une autre.
#   python eon_film_fenetre.py <dossier_sortie> [duree=50] [fps=15] [attente_debut=0]
import ctypes, ctypes.wintypes as W, os, sys, time
from PIL import Image
out = sys.argv[1]; duree = float(sys.argv[2]) if len(sys.argv) > 2 else 50.0
fps = float(sys.argv[3]) if len(sys.argv) > 3 else 15.0; attente = float(sys.argv[4]) if len(sys.argv) > 4 else 0.0
os.makedirs(out, exist_ok=True)
u = ctypes.windll.user32
try: ctypes.windll.shcore.SetProcessDpiAwareness(2)
except Exception: pass
k32 = ctypes.windll.kernel32
k32.OpenProcess.restype = ctypes.c_void_p; k32.CloseHandle.argtypes = [ctypes.c_void_p]
k32.QueryFullProcessImageNameW.argtypes = [ctypes.c_void_p, W.DWORD, ctypes.c_wchar_p, ctypes.POINTER(W.DWORD)]
def exe(h):   # nom de l'exécutable de la fenêtre : seul le jeu (UnrealEditor -game) compte, pas une visionneuse « EON_… »
    pid = W.DWORD(); u.GetWindowThreadProcessId(h, ctypes.byref(pid))
    hp = k32.OpenProcess(0x1000, False, pid.value)
    if not hp: return ""
    b = ctypes.create_unicode_buffer(520); n = W.DWORD(520)
    ok = k32.QueryFullProcessImageNameW(hp, 0, b, ctypes.byref(n)); k32.CloseHandle(hp)
    return b.value.replace("\\", "/").split("/")[-1] if ok else ""
PID = int(os.environ.get("EON_PID", "0"))   # facultatif : ne filmer que la fenêtre de ce processus (autre jeu EON ouvert)
def pid_de(h):
    pid = W.DWORD(); u.GetWindowThreadProcessId(h, ctypes.byref(pid)); return pid.value
def trouve():
    res = []
    @ctypes.WINFUNCTYPE(ctypes.c_bool, W.HWND, W.LPARAM)
    def cb(h, _):
        n = u.GetWindowTextLengthW(h)
        if n and u.IsWindowVisible(h):
            b = ctypes.create_unicode_buffer(n + 1); u.GetWindowTextW(h, b, n + 1)
            r = W.RECT(); u.GetClientRect(h, ctypes.byref(r))
            if "EON" in b.value and "Unreal Editor" not in b.value and r.right > 800 and exe(h).lower().startswith("unrealeditor") and (not PID or pid_de(h) == PID): res.append(h)
        return True
    u.EnumWindows(cb, 0)
    return res[0] if res else None
t0 = time.time(); h = None
while time.time() - t0 < 240 and not h:
    h = trouve(); time.sleep(0.5)
if not h: print("fenêtre introuvable"); sys.exit(1)
time.sleep(attente)
for _ in range(20):   # fenêtre réduite (-48000) : la restaurer sans voler le focus
    if not u.IsIconic(h): break
    u.ShowWindow(h, 4); time.sleep(0.5)
r = W.RECT(); u.GetClientRect(h, ctypes.byref(r))
p = W.POINT(0, 0); u.ClientToScreen(h, ctypes.byref(p))
box = (p.x, p.y, p.x + r.right, p.y + r.bottom)
print("fenêtre", box)
g = ctypes.windll.gdi32
VP = ctypes.c_void_p   # poignées 64 bits : sans restype, ctypes les tronque en int
u.GetDC.restype = VP; u.GetDC.argtypes = [VP]; u.ReleaseDC.argtypes = [VP, VP]; u.PrintWindow.argtypes = [VP, VP, ctypes.c_uint]
g.CreateCompatibleDC.restype = VP; g.CreateCompatibleDC.argtypes = [VP]
g.CreateCompatibleBitmap.restype = VP; g.CreateCompatibleBitmap.argtypes = [VP, ctypes.c_int, ctypes.c_int]
g.SelectObject.restype = VP; g.SelectObject.argtypes = [VP, VP]; g.DeleteObject.argtypes = [VP]; g.DeleteDC.argtypes = [VP]
g.GetDIBits.argtypes = [VP, VP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
class BIH(ctypes.Structure):
    _fields_ = [("biSize", W.DWORD), ("biWidth", W.LONG), ("biHeight", W.LONG), ("biPlanes", W.WORD), ("biBitCount", W.WORD),
                ("biCompression", W.DWORD), ("biSizeImage", W.DWORD), ("biXPelsPerMeter", W.LONG), ("biYPelsPerMeter", W.LONG),
                ("biClrUsed", W.DWORD), ("biClrImportant", W.DWORD)]
def grab(h, w, hh):
    hdc = u.GetDC(h); mdc = g.CreateCompatibleDC(hdc); bmp = g.CreateCompatibleBitmap(hdc, w, hh); g.SelectObject(mdc, bmp)
    u.PrintWindow(h, mdc, 3)   # PW_CLIENTONLY | PW_RENDERFULLCONTENT
    bi = BIH(); bi.biSize = ctypes.sizeof(BIH); bi.biWidth = w; bi.biHeight = -hh; bi.biPlanes = 1; bi.biBitCount = 32
    buf = ctypes.create_string_buffer(w * hh * 4)
    g.GetDIBits(mdc, bmp, 0, hh, buf, ctypes.byref(bi), 0)
    g.DeleteObject(bmp); g.DeleteDC(mdc); u.ReleaseDC(h, hdc)
    return Image.frombuffer("RGB", (w, hh), buf, "raw", "BGRX", 0, 1)
k = 0; t1 = time.time()
while time.time() - t1 < duree and u.IsWindow(h):
    t = time.time()
    im = grab(h, r.right, r.bottom)
    im = im.resize((im.width // 2, im.height // 2))
    im.save(os.path.join(out, "f%04d_%05.2f.jpg" % (k, t - t1)), quality=88)
    k += 1
    time.sleep(max(0.0, 1.0 / fps - (time.time() - t)))
print("images", k)
