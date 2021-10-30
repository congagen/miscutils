from PIL import Image
import PIL.ImageOps

import os, sys
import random


i_path = "input/raw/"
o_path = "input/prepped/"
dirs = os.listdir(i_path)

FRAME_COUNT = 1000000
INVERT_PROB = 50
ROTATE_PROB = 50
DOWNSAM_PROB = 50

def main():
    count = 0

    while count < FRAME_COUNT:
        for item in dirs:
            count += 1
            out_f_name = str(random.randrange(count + 1, count + 100000000000000)) + str(count)
            if os.path.isfile(i_path+item) and str(item)[0] != ".":
                print("Item: "+ i_path+item)
                print(count)
                im = Image.open(i_path+item)
                im = im.convert('RGB')
                #f, e = os.path.splitext(path+item)

                if random.randint(0,100) > 50:
                    im = PIL.ImageOps.invert(im)

                if random.randint(0, 100) > 75:
                    deg = random.choice([90, 180, 270])
                    im = im.rotate(deg)

                if random.randint(0, 100) > 30:
                    im = im.resize((random.randint(10, 200), random.randint(10, 200)), Image.ANTIALIAS)

                imResize = im.resize((1920, 1080), Image.ANTIALIAS)
                imResize.save(o_path + str(out_f_name) + '.jpg', 'JPEG', quality=90)


def duplicate(frame_count):
    pass


main()
