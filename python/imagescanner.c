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

static char imagescanner_doc[] = PyDoc_STR(
    "scan images for barcodes.\n"
    "\n"
    "attaches symbols to image for each decoded result.");

static zbarImageScanner*
imagescanner_new (PyTypeObject *type,
                  PyObject *args,
                  PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbarImageScanner *self = (zbarImageScanner*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zscn = zbar_image_scanner_create();
    if(!self->zscn) {
        Py_DECREF(self);
        return(NULL);
    }

    return(self);
}

static void
imagescanner_dealloc (zbarImageScanner *self)
{
    zbar_image_scanner_destroy(self->zscn);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
imagescanner_set_config (zbarImageScanner *self,
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

    if(zbar_image_scanner_set_config(self->zscn, sym, cfg, val)) {
        PyErr_SetString(PyExc_ValueError, "invalid configuration setting");
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
imagescanner_parse_config (zbarImageScanner *self,
                           PyObject *args,
                           PyObject *kwds)
{
    const char *cfg = NULL;
    static char *kwlist[] = { "config", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &cfg))
        return(NULL);

    if(zbar_image_scanner_parse_config(self->zscn, cfg)) {
        PyErr_Format(PyExc_ValueError, "invalid configuration setting: %s",
                     cfg);
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
imagescanner_enable_cache (zbarImageScanner *self,
                           PyObject *args,
                           PyObject *kwds)
{
    unsigned char enable = 1;
    static char *kwlist[] = { "enable", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                    object_to_bool, &enable))
        return(NULL);

    zbar_image_scanner_enable_cache(self->zscn, enable);
    Py_RETURN_NONE;
}

static PyObject*
imagescanner_scan (zbarImageScanner *self,
                   PyObject *args,
                   PyObject *kwds)
{
    zbarImage *img = NULL;
    static char *kwlist[] = { "image", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                    &zbarImage_Type, &img))
        return(NULL);

    if(zbarImage_validate(img))
        return(NULL);

    int n = zbar_scan_image(self->zscn, img->zimg);
    if(n < 0) {
        PyErr_Format(PyExc_ValueError, "unsupported image format");
        return(NULL);
    }
    return(PyInt_FromLong(n));
}

static PyMethodDef imagescanner_methods[] = {
    { "set_config",   (PyCFunction)imagescanner_set_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "parse_config", (PyCFunction)imagescanner_parse_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "enable_cache",  (PyCFunction)imagescanner_enable_cache,
      METH_VARARGS | METH_KEYWORDS, },
    { "scan",          (PyCFunction)imagescanner_scan,
      METH_VARARGS | METH_KEYWORDS, },
    { NULL, },
};

PyTypeObject zbarImageScanner_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.ImageScanner",
    .tp_doc         = imagescanner_doc,
    .tp_basicsize   = sizeof(zbarImageScanner),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new         = (newfunc)imagescanner_new,
    .tp_dealloc     = (destructor)imagescanner_dealloc,
    .tp_methods     = imagescanner_methods,
};
