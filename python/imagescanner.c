#include "zebramodule.h"

static char imagescanner_doc[] = PyDoc_STR(
    "scan images for barcodes.\n"
    "\n"
    "attaches symbols to image for each decoded result.");

static zebraImageScanner*
imagescanner_new (PyTypeObject *type,
                  PyObject *args,
                  PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebraImageScanner *self = (zebraImageScanner*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zscn = zebra_image_scanner_create();
    if(!self->zscn) {
        Py_DECREF(self);
        return(NULL);
    }

    return(self);
}

static void
imagescanner_dealloc (zebraImageScanner *self)
{
    zebra_image_scanner_destroy(self->zscn);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
imagescanner_set_config (zebraImageScanner *self,
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

    if(zebra_image_scanner_set_config(self->zscn, sym, cfg, val)) {
        PyErr_SetString(PyExc_ValueError, "invalid configuration setting");
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
imagescanner_parse_config (zebraImageScanner *self,
                           PyObject *args,
                           PyObject *kwds)
{
    const char *cfg = NULL;
    static char *kwlist[] = { "config", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &cfg))
        return(NULL);

    if(zebra_image_scanner_parse_config(self->zscn, cfg)) {
        PyErr_Format(PyExc_ValueError, "invalid configuration setting: %s",
                     cfg);
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
imagescanner_enable_cache (zebraImageScanner *self,
                           PyObject *args,
                           PyObject *kwds)
{
    unsigned char enable = 1;
    static char *kwlist[] = { "enable", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                    object_to_bool, &enable))
        return(NULL);

    zebra_image_scanner_enable_cache(self->zscn, enable);
    Py_RETURN_NONE;
}

static PyObject*
imagescanner_scan (zebraImageScanner *self,
                   PyObject *args,
                   PyObject *kwds)
{
    zebraImage *img = NULL;
    static char *kwlist[] = { "image", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                    &zebraImage_Type, &img))
        return(NULL);

    if(zebraImage_validate(img))
        return(NULL);

    int n = zebra_scan_image(self->zscn, img->zimg);
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

PyTypeObject zebraImageScanner_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.ImageScanner",
    .tp_doc         = imagescanner_doc,
    .tp_basicsize   = sizeof(zebraImageScanner),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new         = (newfunc)imagescanner_new,
    .tp_dealloc     = (destructor)imagescanner_dealloc,
    .tp_methods     = imagescanner_methods,
};
