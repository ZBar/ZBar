/*------------------------------------------------------------------------
 *  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include "zbarmodule.h"

static char scanner_doc[] = PyDoc_STR(
    "low level intensity sample stream scanner.  identifies \"bar\" edges"
    "and measures width between them.\n"
    "\n"
    "FIXME.");

static zbarScanner*
scanner_new (PyTypeObject *type,
             PyObject *args,
             PyObject *kwds)
{
    zbarDecoder *decoder = NULL;
    static char *kwlist[] = { "decoder", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", kwlist,
                                    &decoder, zbarDecoder_Type))
        return(NULL);

    zbarScanner *self = (zbarScanner*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    zbar_decoder_t *zdcode = NULL;
    if(decoder) {
        Py_INCREF(decoder);
        self->decoder = decoder;
        zdcode = decoder->zdcode;
    }
    self->zscn = zbar_scanner_create(zdcode);
    if(!self->zscn) {
        Py_DECREF(self);
        return(NULL);
    }

    return(self);
}

static int
scanner_traverse (zbarScanner *self,
                  visitproc visit,
                  void *arg)
{
    Py_VISIT(self->decoder);
    return(0);
}

static int
scanner_clear (zbarScanner *self)
{
    Py_CLEAR(self->decoder);
    return(0);
}

static void
scanner_dealloc (zbarScanner *self)
{
    scanner_clear(self);
    zbar_scanner_destroy(self->zscn);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
scanner_get_width (zbarScanner *self,
                   void *closure)
{
    unsigned int width = zbar_scanner_get_width(self->zscn);
    return(PyInt_FromLong(width));
}

static zbarEnumItem*
scanner_get_color (zbarScanner *self,
                   void *closure)
{
    zbar_color_t zcol = zbar_scanner_get_color(self->zscn);
    assert(zcol == ZBAR_BAR || zcol == ZBAR_SPACE);
    zbarEnumItem *color = color_enum[zcol];
    Py_INCREF((PyObject*)color);
    return(color);
}

static PyGetSetDef scanner_getset[] = {
    { "color",    (getter)scanner_get_color, },
    { "width",    (getter)scanner_get_width, },
    { NULL, },
};

static PyObject*
scanner_reset (zbarScanner *self,
               PyObject *args,
               PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbar_scanner_reset(self->zscn);
    Py_RETURN_NONE;
}

static PyObject*
scanner_new_scan (zbarScanner *self,
                  PyObject *args,
                  PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbar_scanner_new_scan(self->zscn);
    Py_RETURN_NONE;
}

static zbarEnumItem*
scanner_scan_y (zbarScanner *self,
                PyObject *args,
                PyObject *kwds)
{
    /* FIXME should accept sequence of values */
    int y = 0;
    static char *kwlist[] = { "y", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &y))
        return(NULL);

    zbar_symbol_type_t sym = zbar_scan_y(self->zscn, y);
    if(PyErr_Occurred())
        /* propagate errors during callback */
        return(NULL);
    if(sym == ZBAR_NONE) {
        /* hardcode most common case */
        Py_INCREF((PyObject*)symbol_NONE);
        return(symbol_NONE);
    }
    return(zbarSymbol_LookupEnum(sym));
}

static PyMethodDef scanner_methods[] = {
    { "reset",        (PyCFunction)scanner_reset,
      METH_VARARGS | METH_KEYWORDS, },
    { "new_scan",     (PyCFunction)scanner_new_scan,
      METH_VARARGS | METH_KEYWORDS, },
    { "scan_y",       (PyCFunction)scanner_scan_y,
      METH_VARARGS | METH_KEYWORDS, },
    { NULL, },
};

PyTypeObject zbarScanner_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.Scanner",
    .tp_doc         = scanner_doc,
    .tp_basicsize   = sizeof(zbarScanner),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)scanner_new,
    .tp_traverse    = (traverseproc)scanner_traverse,
    .tp_clear       = (inquiry)scanner_clear,
    .tp_dealloc     = (destructor)scanner_dealloc,
    .tp_getset      = scanner_getset,
    .tp_methods     = scanner_methods,
};
