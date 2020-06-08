#!/usr/bin/env python3

from pysstv.sstv import SSTV
from pysstv.color import PD120
from PIL import Image
import glob
import qrcode
import uuid
import os
import sys
import random

# Image size array constants
W = 0
H = 1

# PD120, 640x480, 126s

class sstv_event(object):
    def __init__(self, sstv_mode, in_image_path_glob, out_image_path):
        self.sstv_mode = sstv_mode
        self.in_image_path_glob = in_image_path_glob
        self.out_image_path = out_image_path
        self.validate_image()
        self.validate_path(self.out_image_path)

    def validate_image(self):
        _imagefiles = glob.glob(self.in_image_path_glob)
        random.shuffle(_imagefiles)
        for _imagefile in _imagefiles:
            try:
                img = Image.open(_imagefile)
                self.imagefile = _imagefile
                return
            except Exception as e:
                print(e)
        sys.exit(1)

    def validate_path(self, p):
        if not os.path.exists(p):
            os.makedirs(p)

    def load_size_image(self, image_file):
        img = Image.open(image_file)
        if img.size[W] * img.size[H] != self.sstv_mode.WIDTH * self.sstv_mode.HEIGHT:
            img = img.resize((self.sstv_mode.WIDTH, self.sstv_mode.HEIGHT), Image.ANTIALIAS)
        return img

    def create_qr_code(self, data):
        qr = qrcode.QRCode(
            version=2,
            error_correction=qrcode.constants.ERROR_CORRECT_H,  # max redundancy
            box_size=4,  # min for clear contrast in decoded pictures
            border=2)

        qr.add_data(data)
        qr.make(fit=True)
        return qr.make_image()

    def overlay_qr_codes(self, main_img, uid):
        qr_img = self.create_qr_code(uid)
        qr_count = main_img.size[H] // qr_img.size[H]
        spacer = (main_img.size[H] - (qr_img.size[H] * qr_count)) // qr_count

        qr_box = Image.new("RGB", (qr_img.size[W], main_img.size[H]), (255, 255, 255))
        main_img.paste(qr_box, (main_img.size[W] - qr_img.size[W], 0))

        for i in range(qr_count):
            offset = i * (qr_img.size[H] + spacer)
            offset += spacer // 2
            main_img.paste(qr_img, (main_img.size[W] - qr_img.size[W], offset))
            qr_img = qr_img.transpose(Image.ROTATE_90)  # rotate to mitigate interference

    def execute(self):
        print("Processing %s .." % (self.imagefile))

        # create unique ID for the image
        uid = str(uuid.uuid4())[:8]

        # load and resize the image if needed
        main_img = self.load_size_image(self.imagefile)

        # repeat QR code down right side of image
        self.overlay_qr_codes(main_img, uid)

        # save reference image to disk
        tx_file = os.path.join(self.out_image_path, "%s.png" % uid)
        main_img.save(tx_file, "PNG")

        # convert image to sstv data
        sstv = self.sstv_mode(main_img, 12000, 16)
        sstv.vox_enabled = True

        # modulate sstv to audio and write to file
        audio_file = os.path.join(self.out_image_path, "%s.wav" % uid)
        sstv.write_wav(audio_file)

        print("Finished %s" % (self.imagefile))

ev = sstv_event(sstv_mode=PD120,
                in_image_path_glob="sstv_input/*.png",
                out_image_path="sstv_encoded/")
ev.execute()
