#include "zebramodule.h"

static char scanner_doc[] = PyDoc_STR(
    "low level intensity sample stream scanner.  identifies \"bar\" edges"
    "and measures width between them.\n"
    "\n"
    "FIXME.");

static zebraScanner*
scanner_new (PyTypeObject *type,
             PyObject *args,
             PyObject *kwds)
{
    zebraDecoder *decoder = NULL;
    static char *kwlist[] = { "decoder", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", kwlist,
                                    &decoder, zebraDecoder_Type))
        return(NULL);

    zebraScanner *self = (zebraScanner*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    zebra_decoder_t *zdcode = NULL;
    if(decoder) {
        Py_INCREF(decoder);
        self->decoder = decoder;
        zdcode = decoder->zdcode;
    }
    self->zscn = zebra_scanner_create(zdcode);
    if(!self->zscn) {
        Py_DECREF(self);
        return(NULL);
    }

    return(self);
}

static int
scanner_traverse (zebraScanner *self,
                  visitproc visit,
                  void *arg)
{
    Py_VISIT(self->decoder);
    return(0);
}

static int
scanner_clear (zebraScanner *self)
{
    Py_CLEAR(self->decoder);
    return(0);
}

static void
scanner_dealloc (zebraScanner *self)
{
    scanner_clear(self);
    zebra_scanner_destroy(self->zscn);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
scanner_get_width (zebraScanner *self,
                   void *closure)
{
    unsigned int width = zebra_scanner_get_width(self->zscn);
    return(PyInt_FromLong(width));
}

static zebraEnumItem*
scanner_get_color (zebraScanner *self,
                   void *closure)
{
    zebra_color_t zcol = zebra_scanner_get_color(self->zscn);
    assert(zcol == ZEBRA_BAR || zcol == ZEBRA_SPACE);
    zebraEnumItem *color = color_enum[zcol];
    Py_INCREF((PyObject*)color);
    return(color);
}

static PyGetSetDef scanner_getset[] = {
    { "color",    (getter)scanner_get_color, },
    { "width",    (getter)scanner_get_width, },
    { NULL, },
};

static PyObject*
scanner_reset (zebraScanner *self,
               PyObject *args,
               PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebra_scanner_reset(self->zscn);
    Py_RETURN_NONE;
}

static PyObject*
scanner_new_scan (zebraScanner *self,
                  PyObject *args,
                  PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebra_scanner_new_scan(self->zscn);
    Py_RETURN_NONE;
}

static zebraEnumItem*
scanner_scan_y (zebraScanner *self,
                PyObject *args,
                PyObject *kwds)
{
    /* FIXME should accept sequence of values */
    int y = 0;
    static char *kwlist[] = { "y", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &y))
        return(NULL);

    zebra_symbol_type_t sym = zebra_scan_y(self->zscn, y);
    if(PyErr_Occurred())
        /* propagate errors during callback */
        return(NULL);
    if(sym == ZEBRA_NONE) {
        /* hardcode most common case */
        Py_INCREF((PyObject*)symbol_NONE);
        return(symbol_NONE);
    }
    return(zebraSymbol_LookupEnum(sym));
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

PyTypeObject zebraScanner_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Scanner",
    .tp_doc         = scanner_doc,
    .tp_basicsize   = sizeof(zebraScanner),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)scanner_new,
    .tp_traverse    = (traverseproc)scanner_traverse,
    .tp_clear       = (inquiry)scanner_clear,
    .tp_dealloc     = (destructor)scanner_dealloc,
    .tp_getset      = scanner_getset,
    .tp_methods     = scanner_methods,
};
