from PIL import Image
from PIL import ImageOps
import time
import os

SESSION_ID = str(int(time.time()))
SESSION_TMP_PATH = "tmp/" + SESSION_ID
SESSION_OUT_PATH = "output/" + SESSION_ID


def extract_frames(path):
    num_key_frames = 8
    with Image.open(path) as im:
        for i in range(num_key_frames):
            im.seek(im.n_frames // num_key_frames * i)
            im.save(SESSION_TMP_PATH+"/"+'{}.png'.format(i))


def slice_img(im, mid_w, mid_h):
    width  = int(mid_w*2)
    height = int(mid_h*2)

    tl = im.copy().crop((0, 0, mid_w, mid_h)).convert('RGBA')
    tr = im.copy().crop((mid_w, 0, width, mid_h)).convert('RGBA')
    bl = im.copy().crop((0, mid_h, mid_w, height)).convert('RGBA')
    br = im.copy().crop((mid_w, mid_h, width, height)).convert('RGBA')

    return [tl,tr,bl,br]


def mirror_frames(frame_paths, portion=0, x=True, y=True, flip_x=False, flip_y=False, resize=0):
    count = 0
    frames = []
    width= 0
    height = 0

    filename = str(time.time())

    for f in os.listdir(SESSION_TMP_PATH):
        img_path = SESSION_TMP_PATH+"/"+f

        with Image.open(img_path) as im:
            base_w, base_h = im.size
            print(base_w, base_h)

            if base_w % 2 != 0:
                im = im.crop((0, 0, base_w-1, base_h))

            if base_h % 2 != 0:
                im = im.crop((0, 0, base_w, base_h-1))

            #left, top, right, bottom)
            width, height = im.size
            mid_w = width  - int(width * 0.5)
            mid_h = height - int(height * 0.5)

            quarts = slice_img(im, mid_w, mid_h)
            selected = quarts[portion].convert('RGBA')

            composite = Image.new("RGBA", (width, height))

            composite.paste(selected)
            selected = ImageOps.mirror(selected)
            composite.paste(selected, (mid_w, 0))

            selected = ImageOps.flip(selected)
            selected = ImageOps.mirror(selected)

            composite.paste(selected, (0, mid_h))
            selected = ImageOps.mirror(selected)
            composite.paste(selected, (mid_w, mid_h))

            if resize > 0:
                composite = composite.resize((resize, resize))
            #composite.save(SESSION_TMP_PATH+"/"+str(count)+"_comp.png")
            frames.append(composite)

            # Horiso: im.mirror()
            # Vertic: im.flip()

        count += 1

    #gifimg = Image.new("RGBA", (width, height), (255, 255, 255, 0))
    gifimg = frames[0]
    gifimg.convert('RGBA')
    o_path = SESSION_OUT_PATH + "/" + filename + ".gif"
    gifimg.save(o_path, save_all=True, append_images=frames[1:], duration=100, loop=0)


def main():
    os.mkdir(SESSION_TMP_PATH)
    os.mkdir(SESSION_OUT_PATH)

    for f in os.listdir("input"):
        print(f)
        if ".gif" in f:
            for i in range(4):
                extract_frames("input/"+f)
                mirror_frames(SESSION_TMP_PATH, portion=i, resize=100)


if __name__ == '__main__':
    main()
