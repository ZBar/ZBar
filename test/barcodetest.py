#!/usr/bin/env python

import sys, re, unittest as UT, xml.etree.ElementTree as ET
from os import path, getcwd
from errno import EISDIR, EINVAL, EACCES
from StringIO import StringIO
from subprocess import Popen, PIPE
from urllib2 import urlopen, HTTPError
from urlparse import urljoin, urlunparse
from traceback import format_exception

debug = False

# program to run - None means we still need to search for it
zbarimg = None
# arguments to said program
zbarimg_args = [ '-q', '--xml' ]


# namespace support
try:
    register_namespace = ET.register_namespace
except AttributeError:
    def register_namespace(prefix, uri):
        ET._namespace_map[uri] = prefix

# barcode results
BC = 'http://zbar.sourceforge.net/2008/barcode'
register_namespace('bc', BC)

# testcase extensions
TS = 'http://zbar.sourceforge.net/2009/test-spec'
register_namespace('test', TS)


# printing support
def fixtag(node):
    return str(node.tag).split('}', 1)[-1]

def toxml(node):
    s = StringIO()
    ET.ElementTree(node).write(s)
    return s.getvalue()

def hexdump(data):
    print data
    for i, c in enumerate(data):
        if i & 0x7 == 0:
            print '[%04x]' % i,
        print ' %04x' % ord(c),
        if i & 0x7 == 0x7:
            print
    if len(c) & 0x7 != 0x7:
        print '\n'


# search for a file in the distribution
def distdir_search(subdir, base, suffixes=('',)):
    # start with current dir,
    # then try script invocation path
    rundir = path.dirname(sys.argv[0])
    search = [ '', rundir ]

    # finally, attempt to follow VPATH if present
    try:
        import re
        makefile = open('Makefile')
        for line in makefile:
            if re.match('^VPATH\s*=', line):
                vpath = line.split('=', 1)[1].strip()
                if vpath and vpath != rundir:
                    search.append(vpath)
                break
    except:
        pass

    # poke around for subdir
    subdirs = tuple((subdir, path.join('..', subdir), '..', ''))

    for prefix in search:
        for subdir in subdirs:
            for suffix in suffixes:
                file = path.realpath(path.join(prefix, subdir, base + suffix))
                if path.isfile(file):
                    return(file)
    return None


def find_zbarimg():
    """search for zbarimg program to run.
    """
    global zbarimg
    # look in dist dir first
    zbarimg = distdir_search('zbarimg', 'zbarimg', ('', '.exe'))
    if not zbarimg:
        # fall back to PATH
        zbarimg = 'zbarimg'
        if debug:
            print 'using zbarimg from PATH'
    elif debug:
        print 'using:', zbarimg


def run_zbarimg(images):
    """invoke zbarimg for the specified files.

    return results as an ET.Element
    """
    args = [ zbarimg ]
    args.extend(zbarimg_args)
    args.extend(images)
    if debug:
        print 'running:', ' '.join(args)

    # FIXME should be able to pipe (feed) parser straight from output
    child = Popen(args, stdout = PIPE, stderr = PIPE)
    (xml, err) = child.communicate()

    rc = child.returncode
    if debug:
        print 'zbarimg returned', rc

    # FIXME trim usage from error msg
    assert rc in (0, 4), \
           'zbarimg returned error status (%d)\n' % rc + err

    result = ET.XML(xml)
    assert result.tag == ET.QName(BC, 'barcodes')
    return result


class TestCase(UT.TestCase):
    """single barcode source test.

    must have source attribute set to an ET.Element representation of a
    bc:source tag before test is run.
    """
    def shortDescription(self):
        return self.source.get('href')

    def setUp(self):
        if not zbarimg:
            find_zbarimg()

    def runTest(self):
        expect = self.source
        assert expect is not None
        assert expect.tag == ET.QName(BC, 'source')
        actual = run_zbarimg((expect.get('href'),))

        self.assertEqual(len(actual), 1)

        try:
            compare_sources(expect, actual[0])
        except AssertionError:
            if expect.get(str(ET.QName(TS, 'exception'))) != 'TODO':
                raise
            # ignore


class BuiltinTestCase(TestCase):
    def __init__(self, methodName='runTest'):
        TestCase.__init__(self, methodName)

        href = distdir_search('examples', 'barcode.png')
        if not href:
            href = 'http://zbar.sf.net/test/barcode.png'

        self.source = src = ET.Element(ET.QName(BC, 'source'), href=href)
        sym = ET.SubElement(src, ET.QName(BC, 'symbol'), type='EAN-13')
        data = ET.SubElement(sym, ET.QName(BC, 'data'))
        data.text = '9876543210128'


def compare_maps(expect, actual, compare_func):
    errors = []
    notes = []
    for key, iact in actual.iteritems():
        iexp = expect.pop(key, None)
        if iexp is None:
            errors.append('bonus unexpected result:\n' + toxml(iact))
            continue

        try:
            compare_func(iexp, iact)
        except:
            errors.append(''.join(format_exception(*sys.exc_info())))

        if iexp.get(str(ET.QName(TS, 'exception'))) == 'TODO':
            notes.append('TODO unexpected result:\n' + toxml(iact))

    for key, iexp in expect.iteritems():
        if iexp.get(str(ET.QName(TS, 'exception'))) == 'TODO':
            notes.append('TODO missing expected result:\n' + toxml(iexp))
        else:
            errors.append('missing expected result:\n' + toxml(iexp))

    if len(notes) == 1:
        print >>sys.stderr, '(TODO)',
    elif notes:
        print >>sys.stderr, '(%d TODO)' % len(notes),
    assert not errors, '\n'.join(errors)


def compare_sources(expect, actual):
    assert actual.tag == ET.QName(BC, 'source')
    assert actual.get('href').endswith(expect.get('href')), \
           'source href mismatch: %s != %s' % (acthref, exphref)

    # FIXME process/trim test:* contents

    def map_source(src):
        if not len(src) or src[0].tag != ET.QName(BC, 'index'):
            # insert artificial hierarchy
            syms = src[:]
            del src[:]
            idx = ET.SubElement(src, ET.QName(BC, 'index'), num='0')
            idx[:] = syms
            exc = src.get(str(ET.QName(TS, 'exception')))
            if exc is not None:
                idx.set(str(ET.QName(TS, 'exception')), exc)
            return { '0': idx }
        elif len(src):
            assert src[0].tag != ET.QName(BC, 'symbol'), \
                   'invalid source element: ' + \
                   'expecting "index" or "symbol", got "%s"' % fixtag(src[0])

        srcmap = { }
        for idx in src:
            srcmap[idx.get('num')] = idx
        return srcmap

    compare_maps(map_source(expect), map_source(actual), compare_indices)


def compare_indices(expect, actual):
    assert actual.tag == ET.QName(BC, 'index')
    assert actual.get('num') == expect.get('num')

    # FIXME process/trim test:* contents

    def map_index(idx):
        idxmap = { }
        for sym in idx:
            assert sym.tag == ET.QName(BC, 'symbol')
            typ = sym.get('type')
            assert typ is not None
            data = sym.find(str(ET.QName(BC, 'data'))).text
            idxmap[typ, data] = sym

        return idxmap

    try:
        compare_maps(map_index(expect), map_index(actual), compare_symbols)
    except AssertionError:
        if expect.get(str(ET.QName(TS, 'exception'))) != 'TODO':
            raise


def compare_symbols(expect, actual):
    pass


# override unittest.TestLoader to populate tests from xml description
class TestLoader:
    suiteClass = UT.TestSuite

    def __init__(self):
        self.cwd = urlunparse(('file', '', getcwd() + '/', '', '', ''))
        if debug:
            print 'cwd =', self.cwd

    def loadTestsFromModule(self, module):
        return self.suiteClass([BuiltinTestCase()])

    def loadTestsFromNames(self, names, module=None):
        suites = [ self.loadTestsFromName(name, module) for name in names ]
        return self.suiteClass(suites)

    def loadTestsFromURL(self, url=None, file=None):
        if debug:
            print 'loading url:', url

        target = None
        if not file:
            if not url:
                return self.suiteClass([BuiltinTestCase()])

            content = None
            url = urljoin(self.cwd, url)
            # FIXME grok fragment

            try:
                if debug:
                    print 'trying:', url
                file = urlopen(url)
                content = file.info().get('Content-Type')
            except HTTPError, e:
                # possible remote directory
                pass
            except IOError, e:
                if e.errno not in (EISDIR, EINVAL, EACCES):
                    raise
                # could be local directory

            if (not file or
                content == 'text/html' or
                (isinstance(file, HTTPError) and file.code != 200)):
                # could be directory, try opening index
                try:
                    tmp = urljoin(url + '/', 'index.xml')
                    if debug:
                        print 'trying index:', tmp
                    file = urlopen(tmp)
                    content = file.info().get('Content-Type')
                    url = tmp
                except IOError:
                    raise IOError('no test index found at: %s' % url)

            if debug:
                print '\tContent-Type:', content

            if content not in ('application/xml', 'text/xml'):
                # assume url is image to test, try containing index
                # FIXME should be able to keep searching up
                try:
                    target = url.rsplit('/', 1)[1]
                    index = urljoin(url, 'index.xml')
                    if debug:
                        print 'trying index:', index
                    file = urlopen(index)
                    content = file.info().get('Content-Type')
                    if debug:
                        print '\tContent-Type:', content
                    assert content in ('application/xml', 'text/xml')
                    url = index
                except IOError:
                    raise IOError('no index found for: %s' % url)

        index = ET.ElementTree(file=file).getroot()
        assert index.tag == ET.QName(BC, 'barcodes')

        suite = self.suiteClass()
        for src in index:
            # FIXME trim any meta info
            href = src.get('href')
            if target and target != href:
                continue
            if src.tag == ET.QName(BC, 'source'):
                test = TestCase()
                src.set('href', urljoin(url, href))
                test.source = src
                suite.addTest(test)
            elif src.tag == ET.QName(TS, 'index'):
                suite.addTest(self.loadTestsFromURL(urljoin(url, href)))
            else:
                raise AssertionError('malformed test index') # FIXME detail

        assert suite.countTestCases(), 'empty test index: %s' % url
        return suite

    def loadTestsFromName(self, name=None, module=None):
        return self.loadTestsFromURL(name)

    def unsupported(self, *args, **kwargs):
        raise TypeError("unsupported TestLoader API")

    loadTestsFromTestCase = unsupported
    getTestCaseNames = unsupported


if __name__ == '__main__':
    if '-d' in sys.argv:
        debug = True
        sys.argv.remove('-d')

    UT.main(module=None, testLoader=TestLoader())
