# EON — encode un dossier d'images JPG/PNG (ordre alphabétique) en mp4 H.264 avec le ffmpeg intégré à Blender.
#   blender -b --factory-startup --python blender_images_mp4.py -- <dossier> <sortie.mp4> [fps=15] [premiere=0] [derniere=-1]
import bpy, os, sys
a = sys.argv[sys.argv.index("--") + 1:]
src, out = a[0], a[1]
fps = int(a[2]) if len(a) > 2 else 15
fs = sorted(f for f in os.listdir(src) if f.lower().endswith((".jpg", ".png")))
i0 = int(a[3]) if len(a) > 3 else 0
i1 = int(a[4]) if len(a) > 4 and int(a[4]) >= 0 else len(fs)
fs = fs[i0:i1]
sc = bpy.context.scene
se = sc.sequence_editor_create()
coll = se.strips if hasattr(se, "strips") else se.sequences
st = coll.new_image("img", os.path.join(src, fs[0]), 1, 1)
for f in fs[1:]:
    st.elements.append(f)
img = bpy.data.images.load(os.path.join(src, fs[0]))
w, h = img.size
sc.render.resolution_x, sc.render.resolution_y, sc.render.resolution_percentage = w - w % 2, h - h % 2, 100
sc.render.fps = fps
sc.frame_start, sc.frame_end = 1, len(fs)
sc.render.image_settings.file_format = "FFMPEG"
sc.render.ffmpeg.format = "MPEG4"
sc.render.ffmpeg.codec = "H264"
sc.render.ffmpeg.constant_rate_factor = "HIGH"
sc.render.filepath = out
bpy.ops.render.render(animation=True)
print("@@MP4", out, len(fs), "images", w, h, fps)
