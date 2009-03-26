#include "zebramodule.h"

static char decoder_doc[] = PyDoc_STR(
    "low level decode of measured bar/space widths.\n"
    "\n"
    "FIXME.");

static zebraDecoder*
decoder_new (PyTypeObject *type,
             PyObject *args,
             PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebraDecoder *self = (zebraDecoder*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zdcode = zebra_decoder_create();
    zebra_decoder_set_userdata(self->zdcode, self);
    if(!self->zdcode) {
        Py_DECREF(self);
        return(NULL);
    }

    return(self);
}

static int
decoder_traverse (zebraDecoder *self,
                  visitproc visit,
                  void *arg)
{
    Py_VISIT(self->handler);
    Py_VISIT(self->args);
    return(0);
}

static int
decoder_clear (zebraDecoder *self)
{
    zebra_decoder_set_handler(self->zdcode, NULL);
    zebra_decoder_set_userdata(self->zdcode, NULL);
    Py_CLEAR(self->handler);
    Py_CLEAR(self->args);
    return(0);
}

static void
decoder_dealloc (zebraDecoder *self)
{
    decoder_clear(self);
    zebra_decoder_destroy(self->zdcode);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zebraEnumItem*
decoder_get_color (zebraDecoder *self,
                   void *closure)
{
    zebra_color_t zcol = zebra_decoder_get_color(self->zdcode);
    assert(zcol == ZEBRA_BAR || zcol == ZEBRA_SPACE);
    zebraEnumItem *color = color_enum[zcol];
    Py_INCREF((PyObject*)color);
    return(color);
}

static zebraEnumItem*
decoder_get_type (zebraDecoder *self,
                  void *closure)
{
    zebra_symbol_type_t sym = zebra_decoder_get_type(self->zdcode);
    if(sym == ZEBRA_NONE) {
        /* hardcode most common case */
        Py_INCREF((PyObject*)symbol_NONE);
        return(symbol_NONE);
    }
    return(zebraSymbol_LookupEnum(sym));
}

static PyObject*
decoder_get_data (zebraDecoder *self,
                  void *closure)
{
    return(PyString_FromString(zebra_decoder_get_data(self->zdcode)));
}

static PyGetSetDef decoder_getset[] = {
    { "color",    (getter)decoder_get_color, },
    { "type",     (getter)decoder_get_type, },
    { "data",     (getter)decoder_get_data, },
    { NULL, },
};

static PyObject*
decoder_set_config (zebraDecoder *self,
                    PyObject *args,
                    PyObject *kwds)
{
    zebra_symbol_type_t sym = ZEBRA_NONE;
    zebra_config_t cfg = ZEBRA_CFG_ENABLE;
    int val = 1;
    static char *kwlist[] = { "symbology", "config", "value", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist,
                                    &sym, &cfg, &val))
        return(NULL);

    if(zebra_decoder_set_config(self->zdcode, sym, cfg, val)) {
        PyErr_SetString(PyExc_ValueError, "invalid configuration setting");
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
decoder_parse_config (zebraDecoder *self,
                      PyObject *args,
                      PyObject *kwds)
{
    const char *cfg = NULL;
    static char *kwlist[] = { "config", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &cfg))
        return(NULL);

    if(zebra_decoder_parse_config(self->zdcode, cfg)) {
        PyErr_Format(PyExc_ValueError, "invalid configuration setting: %s",
                     cfg);
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
decoder_reset (zebraDecoder *self,
               PyObject *args,
               PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebra_decoder_reset(self->zdcode);
    Py_RETURN_NONE;
}

static PyObject*
decoder_new_scan (zebraDecoder *self,
                  PyObject *args,
                  PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebra_decoder_new_scan(self->zdcode);
    Py_RETURN_NONE;
}

void
decode_handler (zebra_decoder_t *zdcode)
{
    assert(zdcode);
    zebraDecoder *self = zebra_decoder_get_userdata(zdcode);
    assert(self);
    assert(self->zdcode == zdcode);
    assert(self->handler);
    assert(self->args);
    PyObject *junk = PyObject_Call(self->handler, self->args, NULL);
    Py_XDECREF(junk);
}

static PyObject*
decoder_set_handler (zebraDecoder *self,
                     PyObject *args,
                     PyObject *kwds)
{
    PyObject *handler = Py_None;
    PyObject *closure = Py_None;

    static char *kwlist[] = { "handler", "closure", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
                                    &handler, &closure))
        return(NULL);

    if(handler != Py_None && !PyCallable_Check(handler)) {
        PyErr_Format(PyExc_ValueError, "handler %.50s is not callable",
                     handler->ob_type->tp_name);
        return(NULL);
    }
    Py_CLEAR(self->handler);
    Py_CLEAR(self->args);

    if(handler != Py_None) {
        self->args = PyTuple_New(2);
        if(!self->args)
            return(NULL);
        Py_INCREF(self);
        Py_INCREF(closure);
        PyTuple_SET_ITEM(self->args, 0, (PyObject*)self);
        PyTuple_SET_ITEM(self->args, 1, closure);

        Py_INCREF(handler);
        self->handler = handler;

        zebra_decoder_set_handler(self->zdcode, decode_handler);
    }
    else {
        self->handler = self->args = NULL;
        zebra_decoder_set_handler(self->zdcode, NULL);
    }
    Py_RETURN_NONE;
}

static zebraEnumItem*
decoder_decode_width (zebraDecoder *self,
                      PyObject *args,
                      PyObject *kwds)
{
    unsigned int width = 0;
    static char *kwlist[] = { "width", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist, &width))
        return(NULL);

    zebra_symbol_type_t sym = zebra_decode_width(self->zdcode, width);
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

static PyMethodDef decoder_methods[] = {
    { "set_config",   (PyCFunction)decoder_set_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "parse_config", (PyCFunction)decoder_parse_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "reset",        (PyCFunction)decoder_reset,
      METH_VARARGS | METH_KEYWORDS, },
    { "new_scan",     (PyCFunction)decoder_new_scan,
      METH_VARARGS | METH_KEYWORDS, },
    { "set_handler",  (PyCFunction)decoder_set_handler,
      METH_VARARGS | METH_KEYWORDS, },
    { "decode_width", (PyCFunction)decoder_decode_width,
      METH_VARARGS | METH_KEYWORDS, },
    { NULL, },
};

PyTypeObject zebraDecoder_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Decoder",
    .tp_doc         = decoder_doc,
    .tp_basicsize   = sizeof(zebraDecoder),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)decoder_new,
    .tp_traverse    = (traverseproc)decoder_traverse,
    .tp_clear       = (inquiry)decoder_clear,
    .tp_dealloc     = (destructor)decoder_dealloc,
    .tp_getset      = decoder_getset,
    .tp_methods     = decoder_methods,
};
