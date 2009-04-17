#!/usr/bin/python
import sys, os
import unittest as ut
import zbar

# FIXME this needs to be conditional
# would be even better to auto-select PIL or ImageMagick (or...)
import Image

data = None
size = (0, 0)

def load_image():
    image = Image.open(os.path.join(sys.path[0], "barcode.png")).convert('L')
    return(image.tostring(), image.size)

# FIXME this could be integrated w/fixture creation
(data, size) = load_image()

encoded_widths = \
    '9 111 212241113121211311141132 11111 311213121312121332111132 111 9'

VIDEO_DEVICE = None
if 'VIDEO_DEVICE' in os.environ:
    VIDEO_DEVICE = os.environ['VIDEO_DEVICE']

class TestZBarFunctions(ut.TestCase):
    def test_version(self):
        ver = zbar.version()
        self.assert_(isinstance(ver, tuple))
        self.assertEqual(len(ver), 2)
        for v in ver:
            self.assert_(isinstance(v, int))

    def test_verbosity(self):
        zbar.increase_verbosity()
        zbar.set_verbosity(16)

    def test_exceptions(self):
        self.assert_(isinstance(zbar.Exception, type))
        self.assert_(issubclass(zbar.InternalError, zbar.Exception))
        self.assert_(issubclass(zbar.UnsupportedError, zbar.Exception))
        self.assert_(issubclass(zbar.InvalidRequestError, zbar.Exception))
        self.assert_(issubclass(zbar.SystemError, zbar.Exception))
        self.assert_(issubclass(zbar.LockingError, zbar.Exception))
        self.assert_(issubclass(zbar.BusyError, zbar.Exception))
        self.assert_(issubclass(zbar.X11DisplayError, zbar.Exception))
        self.assert_(issubclass(zbar.X11ProtocolError, zbar.Exception))
        self.assert_(issubclass(zbar.WindowClosed, zbar.Exception))

class TestScanner(ut.TestCase):
    def setUp(self):
        self.scn = zbar.Scanner()

    def tearDown(self):
        del(self.scn)

    def test_type(self):
        self.assert_(isinstance(self.scn, zbar.Scanner))
        self.assert_(callable(self.scn.reset))
        self.assert_(callable(self.scn.new_scan))
        self.assert_(callable(self.scn.scan_y))

        def set_color(color):
            self.scn.color = color
        self.assertRaises(AttributeError, set_color, zbar.BAR)

        def set_width(width):
            self.scn.width = width
        self.assertRaises(AttributeError, set_width, 1)

    # FIXME more scanner tests

class TestDecoder(ut.TestCase):
    def setUp(self):
        self.dcode = zbar.Decoder()

    def tearDown(self):
        del(self.dcode)

    def test_type(self):
        self.assert_(isinstance(self.dcode, zbar.Decoder))
        self.assert_(callable(self.dcode.set_config))
        self.assert_(callable(self.dcode.parse_config))
        self.assert_(callable(self.dcode.reset))
        self.assert_(callable(self.dcode.new_scan))
        self.assert_(callable(self.dcode.set_handler))
        self.assert_(callable(self.dcode.decode_width))

        def set_type(typ):
            self.dcode.type = typ
        self.assertRaises(AttributeError, set_type, zbar.Symbol.CODE128)

        def set_color(color):
            self.dcode.color = color
        self.assertRaises(AttributeError, set_color, zbar.BAR)

        def set_data(data):
            self.dcode.data = data
        self.assertRaises(AttributeError, set_data, 'yomama')

    def test_width(self):
        sym = self.dcode.decode_width(5)
        self.assert_(sym is zbar.Symbol.NONE)
        self.assert_(not sym)
        self.assertEqual(str(sym), 'NONE')

        typ = self.dcode.type
        self.assert_(sym is typ)

    def test_reset(self):
        self.assert_(self.dcode.color is zbar.SPACE)
        sym = self.dcode.decode_width(1)
        self.assert_(self.dcode.color is zbar.BAR)
        self.dcode.reset()
        self.assert_(self.dcode.color is zbar.SPACE)

    def test_decode(self):
        inline_sym = [ -1 ]
        def handler(dcode, closure):
            self.assert_(dcode is self.dcode)
            if dcode.type > zbar.Symbol.PARTIAL:
                inline_sym[0] = dcode.type
            closure[0] += 1

        explicit_closure = [ 0 ]
        self.dcode.set_handler(handler, explicit_closure)

        for (i, width) in enumerate(encoded_widths):
            if width == ' ': continue
            sym = self.dcode.decode_width(int(width))
            if i < len(encoded_widths) - 1:
                self.assert_(sym is zbar.Symbol.NONE or
                             sym is zbar.Symbol.PARTIAL)
            else:
                self.assert_(sym is zbar.Symbol.EAN13)

        self.assertEqual(self.dcode.data, '6268964977804')
        self.assert_(self.dcode.color is zbar.BAR)
        self.assert_(sym is zbar.Symbol.EAN13)
        self.assert_(inline_sym[0] is zbar.Symbol.EAN13)
        self.assertEqual(explicit_closure, [ 2 ])

    # FIXME test exception during callback

class TestImage(ut.TestCase):
    def setUp(self):
        self.image = zbar.Image(123, 456, 'Y800')

    def tearDown(self):
        del(self.image)

    def test_type(self):
        self.assert_(isinstance(self.image, zbar.Image))
        self.assert_(callable(self.image.convert))

    def test_new(self):
        self.assertEqual(self.image.format, 'Y800')
        self.assertEqual(self.image.size, (123, 456))

        image = zbar.Image()
        self.assert_(isinstance(image, zbar.Image))
        self.assertEqual(image.format, '\0\0\0\0')
        self.assertEqual(image.size, (0, 0))

    def test_format(self):
        def set_format(fmt):
            self.image.format = fmt
        self.assertRaises(ValueError, set_format, 10)
        self.assertEqual(self.image.format, 'Y800')
        self.image.format = 'gOOb'
        self.assertEqual(self.image.format, 'gOOb')
        self.assertRaises(ValueError, set_format, 'yomama')
        self.assertEqual(self.image.format, 'gOOb')
        self.assertRaises(ValueError, set_format, 'JPG')
        self.assertEqual(self.image.format, 'gOOb')

    def test_size(self):
        def set_size(sz):
            self.image.size = sz
        self.assertRaises(ValueError, set_size, (1,))
        self.assertRaises(ValueError, set_size, 1)
        self.image.size = (12, 6)
        self.assertRaises(ValueError, set_size, (1, 2, 3))
        self.assertEqual(self.image.size, (12, 6))
        self.assertRaises(ValueError, set_size, "foo")
        self.assertEqual(self.image.size, (12, 6))
        self.assertEqual(self.image.width, 12)
        self.assertEqual(self.image.height, 6)
        self.image.width = 81
        self.assertEqual(self.image.size, (81, 6))
        self.image.height = 64
        self.assertEqual(self.image.size, (81, 64))
        self.assertEqual(self.image.width, 81)
        self.assertEqual(self.image.height, 64)

class TestImageScanner(ut.TestCase):
    def setUp(self):
        self.scn = zbar.ImageScanner()

    def tearDown(self):
        del(self.scn)

    def test_type(self):
        self.assert_(isinstance(self.scn, zbar.ImageScanner))
        self.assert_(callable(self.scn.set_config))
        self.assert_(callable(self.scn.parse_config))
        self.assert_(callable(self.scn.enable_cache))
        self.assert_(callable(self.scn.scan))

    def test_set_config(self):
        self.scn.set_config()
        self.assertRaises(ValueError, self.scn.set_config, -1)
        self.assertRaises(TypeError, self.scn.set_config, "yomama")
        self.scn.set_config()

    def test_parse_config(self):
        self.scn.parse_config("disable")
        self.assertRaises(ValueError, self.scn.set_config, -1)
        self.scn.set_config()

class TestImageScan(ut.TestCase):
    def setUp(self):
        self.scn = zbar.ImageScanner()
        self.image = zbar.Image(size[0], size[1], 'Y800', data)

    def tearDown(self):
        del(self.image)
        del(self.scn)

    def test_scan(self):
        n = self.scn.scan(self.image)
        self.assertEqual(n, 1)

        i = iter(self.image)
        self.assert_(isinstance(i, zbar.SymbolIter))
        sym = i.next()
        self.assertRaises(StopIteration, i.next)

        # this is the only way to obtain a Symbol,
        # so test Symbol here
        self.assert_(isinstance(sym, zbar.Symbol))
        self.assert_(sym.type is zbar.Symbol.EAN13)
        self.assert_(sym.type is sym.EAN13)
        self.assertEqual(str(sym.type), 'EAN13')
        self.assertEqual(sym.count, 0)

        data = sym.data
        self.assertEqual(data, '9876543210128')

        loc = sym.location
        self.assertEqual(len(loc), 4)
        self.assert_(isinstance(loc, tuple))
        for pt in loc:
            self.assert_(isinstance(pt, tuple))
            self.assertEqual(len(pt), 2)
            # FIXME test values (API currently in flux)

        self.assert_(data is sym.data)
        self.assert_(loc is sym.location)

    def test_scan_again(self):
        self.test_scan()

class TestProcessor(ut.TestCase):
    def setUp(self):
        self.proc = zbar.Processor()

    def tearDown(self):
        del(self.proc)

    def test_type(self):
        self.assert_(isinstance(self.proc, zbar.Processor))
        self.assert_(callable(self.proc.init))
        self.assert_(callable(self.proc.set_config))
        self.assert_(callable(self.proc.parse_config))
        self.assert_(callable(self.proc.set_data_handler))
        self.assert_(callable(self.proc.user_wait))
        self.assert_(callable(self.proc.process_one))
        self.assert_(callable(self.proc.process_image))

    def test_set_config(self):
        self.proc.set_config()
        self.assertRaises(ValueError, self.proc.set_config, -1)
        self.assertRaises(TypeError, self.proc.set_config, "yomama")
        self.proc.set_config()

    def test_parse_config(self):
        self.proc.parse_config("disable")
        self.assertRaises(ValueError, self.proc.set_config, -1)
        self.proc.set_config()

    def test_processing(self):
        self.proc.init(VIDEO_DEVICE)
        self.assert_(self.proc.visible is False)
        self.proc.visible = 1
        self.assert_(self.proc.visible is True)
        self.assertEqual(self.proc.user_wait(1.1), 0)

        self.image = zbar.Image(size[0], size[1], 'Y800', data)

        count = [ 0 ]
        def data_handler(proc, image, closure):
            self.assert_(proc is self.proc)
            self.assert_(image is self.image)
            self.assertEqual(count[0], 0)
            count[0] += 1

            symiter = iter(image)
            self.assert_(isinstance(symiter, zbar.SymbolIter))

            symbols = tuple(image)
            self.assertEqual(len(symbols), 1)
            for symbol in symbols:
                self.assert_(isinstance(symbol, zbar.Symbol))
                self.assert_(symbol.type is zbar.Symbol.EAN13)
                self.assertEqual(symbol.data, '9876543210128')
            closure[0] += 1

        explicit_closure = [ 0 ]
        self.proc.set_data_handler(data_handler, explicit_closure)

        rc = self.proc.process_image(self.image)
        self.assertEqual(rc, 0)

        self.assertEqual(self.proc.user_wait(.9), 0)

        self.assertEqual(explicit_closure, [ 1 ])
        self.proc.set_data_handler()

if __name__ == '__main__':
    ut.main()
