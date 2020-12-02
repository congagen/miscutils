import os
import sys
from PIL import Image


app_ico_sizes = (57, 72, 114, 144)
web_clip_sizes = (57, 72, 114, 144)


iphone_icons = {
    "Iphone-Notification-20x2": 40,
    "Iphone-Notification-20x3": 60,

    "Iphone_Spotlight-29x1": 29,
    "Iphone_Spotlight-29x2": 58,
    "Iphone_Settings-29x3": 87,

    "Iphone_Spotlight-40x2": 80,
    "Iphone_Spotlight-40x3": 120,

    "Iphone_App_iOS_5.6_57x1": 57,
    "Iphone_App_iOS_5.6_57x2": 114,

    "Iphone_App_iOS_7-10_60x2": 120,
    "Iphone_App_iOS_7-10_60x3": 180,

    "iTunesArtwork" : 512,
    "iTunesArtwork@2x" : 1024,

    "Icon-60@2x" : 120,
    "Icon-60@3x" : 180,

    "Icon-76" : 76,

    "Iphone_App-60x2": 60,
    "Iphone_App-60x3": 120,

    "Icon-Small-40" : 40,
    "Icon-Small-40@2x" : 80,
    "Icon-Small-40@3x" : 120,

    "Icon-Small" : 29,
    "Icon-Small@2x" : 58,
    "Icon-Small@3x" : 87
}

ipad_icons = {
    "iTunesArtwork" : 512,
    "iTunesArtwork@2x" : 1024,

    "Icon-167" : 167,

    "Icon-76" : 76,
    "Icon-76@2x" : 152,

    "iPad-Icon-50" : 50,
    "iPad-Icon-50@2x" : 100,

    "iPad-Icon-72" : 72,
    "iPad-Icon-72x2" : 144,

    "Ipad_Notifications-20x1": 20,
    "Ipad_Notifications-20x2": 40,

    "Ipad_Settings-29x1": 29,
    "Ipad_Settings-29x2": 58,

    "Icon-Small-40" : 40,
    "Icon-Small-40@2x" : 80,
    "Icon-Small" : 40,
    "Icon-Small@2x" : 80,

    "Ipad_Pro_App-83.5x2" : 167
}


universal_icons = {
    "iTunesArtwork" : 512,
    "iTunesArtwork@2x" : 1024,

    "Icon-60@2x" : 120,
    "Icon-60@3x" : 180,

    "Icon-76" : 76,
    "Icon-76@2x" : 152,

    "Icon-Small-40" : 40,
    "Icon-Small-40@2x" : 80,
    "Icon-Small-40@3x" : 120,

    "Icon-Small" : 29,
    "Icon-Small@2x" : 58,
    "Icon-Small@3x" : 87
}


all_forms = {
    "iphone_icons" : iphone_icons,
    "ipad_icons" : ipad_icons,
    "universal_icons" : universal_icons
}


# ------------------------------------------------------------------------------

def scale(format_dict, output_path, ipath):
    Image.MAX_IMAGE_PIXELS = 10000000000000000000


    img = Image.open(ipath)

    for k, v in format_dict.items():
        file_o_path = output_path + '/' + str(k) + ".png"
        resizedImage = img.resize((v, v), Image.ANTIALIAS)
        resizedImage.save(file_o_path)

# ------------------------------------------------------------------------------

def main(input_path):
    for k, v in all_forms.items():
        foldername = str(k)
        os.mkdir(foldername)

        scale(v, foldername, input_path)


if __name__ == "__main__":
    main(str(sys.argv[1]))
