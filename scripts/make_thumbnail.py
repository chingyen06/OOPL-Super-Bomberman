# -*- coding: utf-8 -*-
# 用遊戲本身的素材合成 1280x720 影片縮圖 (學原版風格，不照搬標題畫面)
import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG = lambda p: os.path.join(ROOT, "Resources", "Image", p)
FONT = os.path.join(ROOT, "Resources", "Font", "GenJyuuGothicX-Bold.ttf")
OUT = os.path.join(ROOT, "docs", "video_thumbnail.png")

W, H = 1280, 720


def load(p):
    return Image.open(p).convert("RGBA")


def fit_w(im, w):
    return im.resize((w, max(1, round(im.height * w / im.width))), Image.LANCZOS)


def fit_h(im, h):
    return im.resize((max(1, round(im.width * h / im.height)), h), Image.LANCZOS)


# 背景：遊戲標題畫面的暖色彩紙底
bg = load(IMG("cover_bg.png")).resize((W, H), Image.LANCZOS)
canvas = bg.copy()

# 角落輕微壓暗，讓主體與文字更跳
vig = Image.new("L", (W, H), 0)
vd = ImageDraw.Draw(vig)
vd.rectangle([0, 0, W, H], fill=40)
vd.ellipse([-260, -200, W + 260, H + 200], fill=0)
vig = vig.filter(ImageFilter.GaussianBlur(120))
canvas = Image.composite(Image.new("RGBA", (W, H), (60, 30, 12, 255)), canvas, vig)

def fit_w_nn(im, w):
    return im.resize((w, max(1, round(im.height * w / im.width))), Image.NEAREST)


# 角色立繪：白色炸彈人 (左下)
char = fit_h(load(IMG("player_right.png")), 386)
cx, cy = 48, H - char.height - 14

# 角色腳下陰影
sh = Image.new("RGBA", (W, H), (0, 0, 0, 0))
ImageDraw.Draw(sh).ellipse(
    [cx + 40, cy + char.height - 26, cx + char.width - 10, cy + char.height + 18],
    fill=(50, 24, 10, 110),
)
canvas = Image.alpha_composite(canvas, sh.filter(ImageFilter.GaussianBlur(10)))
canvas.alpha_composite(char, (cx, cy))

# 皇冠 (防守方標記) 戴在頭上
crown = fit_w(load(IMG("crown.png")), 96)
canvas.alpha_composite(crown, (cx + char.width // 2 - crown.width // 2 - 4, cy - 34))

# 炸彈 (角色右側)
bomb = fit_w(load(IMG("bomb.png")), 158)
canvas.alpha_composite(bomb, (cx + char.width - 30, cy + char.height - bomb.height - 4))

# 砲台敵人 (右側，平衡左邊角色)
turret = fit_w_nn(load(IMG("turret_down.png")), 150)
ty = H - turret.height - 84
canvas.alpha_composite(turret, (W - turret.width - 110, ty))
# 砲台陰影
tsh = Image.new("RGBA", (W, H), (0, 0, 0, 0))
ImageDraw.Draw(tsh).ellipse(
    [W - turret.width - 104, ty + turret.height - 14, W - 120, ty + turret.height + 14],
    fill=(50, 24, 10, 95),
)
canvas = Image.alpha_composite(canvas, tsh.filter(ImageFilter.GaussianBlur(9)))
canvas.alpha_composite(turret, (W - turret.width - 110, ty))

# 官方泡泡 logo (上方置中)
logo = fit_w(load(IMG("logo.png")), 560)
lx, ly = (W - logo.width) // 2, 30
canvas.alpha_composite(logo, (lx, ly))

draw = ImageDraw.Draw(canvas)
f_sub = ImageFont.truetype(FONT, 60)
f_tag = ImageFont.truetype(FONT, 44)
f_info = ImageFont.truetype(FONT, 28)

NAVY = (30, 42, 104)
WHITE = (255, 255, 255)


def centered(text, font, y, fill, stroke, sw, x0=0, x1=W):
    b = draw.textbbox((0, 0), text, font=font, stroke_width=sw)
    tw = b[2] - b[0]
    draw.text((x0 + (x1 - x0 - tw) // 2 - b[0], y), text, font=font,
              fill=fill, stroke_width=sw, stroke_fill=stroke)


# 副標：城堡模式 2D 復刻 (logo 下方置中)
centered("城堡模式  ・  2D 復刻", f_sub, ly + logo.height - 10, NAVY, WHITE, 6)

# 右下角帶子：期末影片報告 + 課程資訊
bx0, by0, bx1, by1 = 712, 588, 1252, 672
draw.rounded_rectangle([bx0, by0, bx1, by1], radius=18, fill=(229, 73, 41, 255))
draw.rounded_rectangle([bx0, by0, bx1, by1], radius=18, outline=(255, 255, 255, 235), width=3)
tagb = draw.textbbox((0, 0), "期末影片報告", font=f_tag)
draw.text((bx0 + 26, by0 + 12), "期末影片報告", font=f_tag, fill=WHITE)
draw.text((bx0 + 26, by0 + 56), "物件導向程式設計實習  ｜  T45  曾靖諺",
          font=f_info, fill=(255, 234, 224))

canvas.convert("RGB").save(OUT, "PNG")
print("saved:", OUT, canvas.size)
print("logo:", logo.size, "char:", char.size)
