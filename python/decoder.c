/*------------------------------------------------------------------------
 *  Copyright 2009-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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

static char decoder_doc[] = PyDoc_STR(
    "low level decode of measured bar/space widths.\n"
    "\n"
    "FIXME.");

static zbarDecoder*
decoder_new (PyTypeObject *type,
             PyObject *args,
             PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbarDecoder *self = (zbarDecoder*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zdcode = zbar_decoder_create();
    zbar_decoder_set_userdata(self->zdcode, self);
    if(!self->zdcode) {
        Py_DECREF(self);
        return(NULL);
    }

    return(self);
}

static int
decoder_traverse (zbarDecoder *self,
                  visitproc visit,
                  void *arg)
{
    Py_VISIT(self->handler);
    Py_VISIT(self->args);
    return(0);
}

static int
decoder_clear (zbarDecoder *self)
{
    zbar_decoder_set_handler(self->zdcode, NULL);
    zbar_decoder_set_userdata(self->zdcode, NULL);
    Py_CLEAR(self->handler);
    Py_CLEAR(self->args);
    return(0);
}

static void
decoder_dealloc (zbarDecoder *self)
{
    decoder_clear(self);
    zbar_decoder_destroy(self->zdcode);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zbarEnumItem*
decoder_get_color (zbarDecoder *self,
                   void *closure)
{
    zbar_color_t zcol = zbar_decoder_get_color(self->zdcode);
    assert(zcol == ZBAR_BAR || zcol == ZBAR_SPACE);
    zbarEnumItem *color = color_enum[zcol];
    Py_INCREF((PyObject*)color);
    return(color);
}

static zbarEnumItem*
decoder_get_type (zbarDecoder *self,
                  void *closure)
{
    zbar_symbol_type_t sym = zbar_decoder_get_type(self->zdcode);
    if(sym == ZBAR_NONE) {
        /* hardcode most common case */
        Py_INCREF((PyObject*)symbol_NONE);
        return(symbol_NONE);
    }
    return(zbarSymbol_LookupEnum(sym));
}

static PyObject*
decoder_get_configs (zbarDecoder *self,
                     void *closure)
{
    unsigned int sym = zbar_decoder_get_type(self->zdcode);
    unsigned int mask = zbar_decoder_get_configs(self->zdcode, sym);
    return(zbarEnum_SetFromMask(config_enum, mask));
}

static PyObject*
decoder_get_modifiers (zbarDecoder *self,
                       void *closure)
{
    unsigned int mask = zbar_decoder_get_modifiers(self->zdcode);
    return(zbarEnum_SetFromMask(modifier_enum, mask));
}

static PyObject*
decoder_get_data (zbarDecoder *self,
                  void *closure)
{
    return(PyString_FromStringAndSize(zbar_decoder_get_data(self->zdcode),
                                      zbar_decoder_get_data_length(self->zdcode)));
}

static PyObject*
decoder_get_direction (zbarDecoder *self,
                       void *closure)
{
    return(PyInt_FromLong(zbar_decoder_get_direction(self->zdcode)));
}

static PyGetSetDef decoder_getset[] = {
    { "color",     (getter)decoder_get_color, },
    { "type",      (getter)decoder_get_type, },
    { "configs",   (getter)decoder_get_configs, },
    { "modifiers", (getter)decoder_get_modifiers, },
    { "data",      (getter)decoder_get_data, },
    { "direction", (getter)decoder_get_direction },
    { NULL, },
};

static PyObject*
decoder_set_config (zbarDecoder *self,
                    PyObject *args,
                    PyObject *kwds)
{
    zbar_symbol_type_t sym = ZBAR_NONE;
    zbar_config_t cfg = ZBAR_CFG_ENABLE;
    int val = 1;
    static char *kwlist[] = { "symbology", "config", "value", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist,
                                    &sym, &cfg, &val))
        return(NULL);

    if(zbar_decoder_set_config(self->zdcode, sym, cfg, val)) {
        PyErr_SetString(PyExc_ValueError, "invalid configuration setting");
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
decoder_get_configs_meth (zbarDecoder *self,
                          PyObject *args,
                          PyObject *kwds)
{
    zbar_symbol_type_t sym = ZBAR_NONE;
    static char *kwlist[] = { "symbology", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &sym))
        return(NULL);

    if(sym == ZBAR_NONE)
        sym = zbar_decoder_get_type(self->zdcode);

    unsigned int mask = zbar_decoder_get_configs(self->zdcode, sym);
    return(zbarEnum_SetFromMask(config_enum, mask));
}

static PyObject*
decoder_parse_config (zbarDecoder *self,
                      PyObject *args,
                      PyObject *kwds)
{
    const char *cfg = NULL;
    static char *kwlist[] = { "config", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &cfg))
        return(NULL);

    if(zbar_decoder_parse_config(self->zdcode, cfg)) {
        PyErr_Format(PyExc_ValueError, "invalid configuration setting: %s",
                     cfg);
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
decoder_reset (zbarDecoder *self,
               PyObject *args,
               PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbar_decoder_reset(self->zdcode);
    Py_RETURN_NONE;
}

static PyObject*
decoder_new_scan (zbarDecoder *self,
                  PyObject *args,
                  PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbar_decoder_new_scan(self->zdcode);
    Py_RETURN_NONE;
}

void
decode_handler (zbar_decoder_t *zdcode)
{
    assert(zdcode);
    zbarDecoder *self = zbar_decoder_get_userdata(zdcode);
    assert(self);
    assert(self->zdcode == zdcode);
    assert(self->handler);
    assert(self->args);
    PyObject *junk = PyObject_Call(self->handler, self->args, NULL);
    Py_XDECREF(junk);
}

static PyObject*
decoder_set_handler (zbarDecoder *self,
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

        zbar_decoder_set_handler(self->zdcode, decode_handler);
    }
    else {
        self->handler = self->args = NULL;
        zbar_decoder_set_handler(self->zdcode, NULL);
    }
    Py_RETURN_NONE;
}

static zbarEnumItem*
decoder_decode_width (zbarDecoder *self,
                      PyObject *args,
                      PyObject *kwds)
{
    unsigned int width = 0;
    static char *kwlist[] = { "width", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist, &width))
        return(NULL);

    zbar_symbol_type_t sym = zbar_decode_width(self->zdcode, width);
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

static PyMethodDef decoder_methods[] = {
    { "set_config",   (PyCFunction)decoder_set_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "get_configs",  (PyCFunction)decoder_get_configs_meth,
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

PyTypeObject zbarDecoder_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.Decoder",
    .tp_doc         = decoder_doc,
    .tp_basicsize   = sizeof(zbarDecoder),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)decoder_new,
    .tp_traverse    = (traverseproc)decoder_traverse,
    .tp_clear       = (inquiry)decoder_clear,
    .tp_dealloc     = (destructor)decoder_dealloc,
    .tp_getset      = decoder_getset,
    .tp_methods     = decoder_methods,
};
