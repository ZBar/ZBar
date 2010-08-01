#!/usr/bin/env python
from xmlrpclib import ServerProxy
import sys, re

server = ServerProxy("http://www.upcdatabase.com/rpc")
ean_re = re.compile(r'^(UPC-A:|EAN-13:)?(\d{11,13})$', re.M)

def lookup(decode):
    match = ean_re.search(decode)
    if match is None:
        print decode,
        return
    ean = match.group(2)
    if match.group(1) == "UPC-A:":
        ean = "0" + ean;
    elif len(ean) < 12:
        print decode,
        return
    if len(ean) == 12:
        ean = server.calculateCheckDigit(ean + "C")
    print "[" + match.group(1) + ean + "]",
    result = server.lookupEAN(ean)
    if isinstance(result, dict):
        if "found" not in result or not result["found"] or \
               "description" not in result:
            print "not found"
        else:
            print result["description"]
    else:
        print str(result)
    sys.stdout.flush()

if __name__ == "__main__":
    del sys.argv[0]
    if len(sys.argv):
        for decode in sys.argv:
            lookup(decode)
    if not sys.stdin.isatty():
        while 1:
            decode = sys.stdin.readline()
            if not decode:
                break
            lookup(decode)
