#!/usr/bin/python
from sys import argv
import zebra
import Image

if len(argv) < 2: exit(1)

# create a reader
scanner = zebra.ImageScanner()

# configure the reader
scanner.parse_config('enable')

# obtain image data
pil = Image.open(argv[1]).convert('L')
width, height = pil.size
raw = pil.tostring()

# wrap image data
image = zebra.Image(width, height, 'Y800', raw)

# scan the image for barcodes
scanner.scan(image)

# extract results
for symbol in image:
    # do something useful with results
    print 'decoded', symbol.type, 'symbol', '"%s"' % symbol.data

# clean up
del(image)
