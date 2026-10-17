# Rebuilds icon.jpg: blue globe on a light background.
import math
from PIL import Image, ImageDraw

S, c, r, w, col = 256, 128, 84, 10, "#2F7DF6"
im = Image.new("RGB", (S, S), "#F4F7FB")
d = ImageDraw.Draw(im)
d.ellipse((c - r, c - r, c + r, c + r), outline=col, width=w)
d.ellipse((c - r // 2, c - r, c + r // 2, c + r), outline=col, width=w)
d.line((c, c - r, c, c + r), fill=col, width=w)
d.line((c - r, c, c + r, c), fill=col, width=w)
for y in (c - r // 2 - 4, c + r // 2 + 4):
    half = int(math.sqrt(r * r - (y - c) ** 2))
    d.line((c - half + 6, y, c + half - 6, y), fill=col, width=w)
im.save("icon.jpg", quality=92)
