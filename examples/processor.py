#!/usr/bin/python
from sys import argv
import zbar

# create a Processor
proc = zbar.Processor()

# configure the Processor
proc.parse_config('enable')

# initialize the Processor
device = '/dev/video0'
if len(argv) > 1:
    device = argv[1]
proc.init(device)

# setup a callback
def my_handler(proc, image, closure):
    # extract results
    for symbol in image:
        if not symbol.count:
            # do something useful with results
            print 'decoded', symbol.type, 'symbol', '"%s"' % symbol.data

proc.set_data_handler(my_handler)

# enable the preview window
proc.visible = True

# initiate scanning
proc.active = True
try:
    proc.user_wait()
except zbar.WindowClosed, e:
    pass
