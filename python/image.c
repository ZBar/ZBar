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

static char symboliter_doc[] = PyDoc_STR(
    "symbol iterator.\n"
    "\n"
    "iterates over decode results attached to an image.");

static int
symboliter_traverse (zbarSymbolIter *self,
                     visitproc visit,
                     void *arg)
{
    Py_VISIT(self->img);
    return(0);
}

static int
symboliter_clear (zbarSymbolIter *self)
{
    Py_CLEAR(self->img);
    return(0);
}

static void
symboliter_dealloc (zbarSymbolIter *self)
{
    symboliter_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zbarSymbolIter*
symboliter_iter (zbarSymbolIter *self)
{
    Py_INCREF(self);
    return(self);
}

static zbarSymbol*
symboliter_iternext (zbarSymbolIter *self)
{
    if(!self->zsym)
        self->zsym = zbar_image_first_symbol(self->img->zimg);
    else
        self->zsym = zbar_symbol_next(self->zsym);
    if(!self->zsym)
        return(NULL);
    return(zbarSymbol_FromSymbol(self->img, self->zsym));
}

PyTypeObject zbarSymbolIter_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.SymbolIter",
    .tp_doc         = symboliter_doc,
    .tp_basicsize   = sizeof(zbarSymbolIter),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_traverse    = (traverseproc)symboliter_traverse,
    .tp_clear       = (inquiry)symboliter_clear,
    .tp_dealloc     = (destructor)symboliter_dealloc,
    .tp_iter        = (getiterfunc)symboliter_iter,
    .tp_iternext    = (iternextfunc)symboliter_iternext,
};

static zbarSymbolIter*
zbarSymbolIter_New (zbarImage *img)
{
    zbarSymbolIter *self;
    self = PyObject_GC_New(zbarSymbolIter, &zbarSymbolIter_Type);
    if(!self)
        return(NULL);

    Py_INCREF(img);
    self->img = img;
    self->zsym = NULL;
    return(self);
}


static char image_doc[] = PyDoc_STR(
    "image object.\n"
    "\n"
    "stores image data samples along with associated format and size metadata.");

static zbarImage*
image_new (PyTypeObject *type,
           PyObject *args,
           PyObject *kwds)
{
    zbarImage *self = (zbarImage*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zimg = zbar_image_create();
    if(!self->zimg) {
        Py_DECREF(self);
        return(NULL);
    }
    zbar_image_set_userdata(self->zimg, self);
    return(self);
}

static int
image_traverse (zbarImage *self,
                visitproc visit,
                void *arg)
{
    Py_VISIT(self->data);
    return(0);
}

static int
image_clear (zbarImage *self)
{
    zbar_image_t *zimg = self->zimg;
    self->zimg = NULL;
    if(zimg) {
        assert(zbar_image_get_userdata(zimg) == self);
        if(self->data) {
            /* attach data directly to zbar image */
            zbar_image_set_userdata(zimg, self->data);
            self->data = NULL;
        }
        else
            zbar_image_set_userdata(zimg, NULL);
        zbar_image_destroy(zimg);
    }
    return(0);
}

static void
image_dealloc (zbarImage *self)
{
    image_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zbarSymbolIter*
image_iter (zbarImage *self)
{
    return(zbarSymbolIter_New(self));
}

static PyObject*
image_get_format (zbarImage *self,
                  void *closure)
{
    unsigned long format = zbar_image_get_format(self->zimg);
    return(PyString_FromStringAndSize((char*)&format, 4));
}

static int
image_set_format (zbarImage *self,
                  PyObject *value,
                  void *closure)
{
    if(!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete format attribute");
        return(-1);
    }
    char *format;
    Py_ssize_t len;
    if(PyString_AsStringAndSize(value, &format, &len) ||
       !format || len != 4) {
        PyErr_Format(PyExc_ValueError,
                     "format '%.50s' is not a valid four character code",
                     format);
        return(-1);
    }
    zbar_image_set_format(self->zimg,*((unsigned long*)format));
    return(0);
}

static PyObject*
image_get_size (zbarImage *self,
                void *closure)
{
    unsigned int width = zbar_image_get_width(self->zimg);
    unsigned int height = zbar_image_get_height(self->zimg);
    return(PyTuple_Pack(2, PyInt_FromLong(width), PyInt_FromLong(height)));
}

static int
image_set_size (zbarImage *self,
                PyObject *value,
                void *closure)
{
    if(!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete size attribute");
        return(-1);
    }
    int rc = -1;
    PyObject *wobj = NULL, *hobj = NULL;
    if(!PySequence_Check(value) ||
       PySequence_Size(value) != 2)
        goto error;
    wobj = PySequence_GetItem(value, 0);
    hobj = PySequence_GetItem(value, 1);
    if(!wobj || !hobj)
        goto error;

    int width = PyInt_AsSsize_t(wobj);
    if(width == -1 && PyErr_Occurred())
        goto error;

    int height = PyInt_AsSsize_t(hobj);
    if(height == -1 && PyErr_Occurred())
        goto error;

    zbar_image_set_size(self->zimg, width, height);
    rc = 0;

error:
    Py_XDECREF(wobj);
    Py_XDECREF(hobj);
    if(rc)
        PyErr_SetString(PyExc_ValueError,
                        "size must be a sequence of two ints");
    return(rc);
}

static PyObject*
image_get_int (zbarImage *self,
               void *closure)
{
    unsigned int val = -1;
    switch((int)closure) {
    case 0:
        val = zbar_image_get_width(self->zimg); break;
    case 1:
        val = zbar_image_get_height(self->zimg); break;
    case 2:
        val = zbar_image_get_sequence(self->zimg); break;
    default:
        assert(0);
    }
    return(PyInt_FromLong(val));
}

static int
image_set_int (zbarImage *self,
               PyObject *value,
               void *closure)
{
    unsigned int tmp, val = PyInt_AsSsize_t(value);
    if(val == -1 && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError, "expecting an integer");
        return(-1);
    }
    switch((int)closure) {
    case 0:
        tmp = zbar_image_get_height(self->zimg);
        zbar_image_set_size(self->zimg, val, tmp);
        break;
    case 1:
        tmp = zbar_image_get_width(self->zimg);
        zbar_image_set_size(self->zimg, tmp, val);
        break;
    case 2:
        zbar_image_set_sequence(self->zimg, val);
    default:
        assert(0);
    }
    return(0);
}

static PyObject*
image_get_data (zbarImage *self,
                void *closure)
{
    assert(zbar_image_get_userdata(self->zimg) == self);
    if(self->data) {
        Py_INCREF(self->data);
        return(self->data);
    }

    const char *data = zbar_image_get_data(self->zimg);
    unsigned long datalen = zbar_image_get_data_length(self->zimg);
    if(!data || !datalen) {
        Py_INCREF(Py_None);
        return(Py_None);
    }

    self->data = PyBuffer_FromMemory((void*)data, datalen);
    Py_INCREF(self->data);
    return(self->data);
}

void
image_cleanup (zbar_image_t *zimg)
{
    PyObject *data = zbar_image_get_userdata(zimg);
    zbar_image_set_userdata(zimg, NULL);
    if(!data)
        return;  /* FIXME internal error */
    if(PyObject_TypeCheck(data, &zbarImage_Type)) {
        zbarImage *self = (zbarImage*)data;
        assert(self->zimg == zimg);
        Py_CLEAR(self->data);
    }
    else
        Py_DECREF(data);
}

static int
image_set_data (zbarImage *self,
                PyObject *value,
                void *closure)
{
    if(!value) {
        zbar_image_free_data(self->zimg);
        return(0);
    }
    char *data;
    Py_ssize_t datalen;
    if(PyString_AsStringAndSize(value, &data, &datalen))
        return(-1);

    Py_INCREF(value);
    zbar_image_set_data(self->zimg, data, datalen, image_cleanup);
    assert(!self->data);
    self->data = value;
    zbar_image_set_userdata(self->zimg, self);
    return(0);
}

static PyGetSetDef image_getset[] = {
    { "format",   (getter)image_get_format, (setter)image_set_format, },
    { "size",     (getter)image_get_size,   (setter)image_set_size, },
    { "width",    (getter)image_get_int,    (setter)image_set_int,
      NULL, (void*)0 },
    { "height",   (getter)image_get_int,    (setter)image_set_int,
      NULL, (void*)1 },
    { "sequence", (getter)image_get_int,    (setter)image_set_int,
      NULL, (void*)2 },
    { "data",     (getter)image_get_data,   (setter)image_set_data, },
    { NULL, },
};

static int
image_init (zbarImage *self,
            PyObject *args,
            PyObject *kwds)
{
    int width = -1, height = -1;
    PyObject  *format = NULL, *data = NULL;
    static char *kwlist[] = { "width", "height", "format", "data", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|iiOO", kwlist,
                                    &width, &height, &format, &data))
        return(-1);

    if(width > 0 && height > 0)
        zbar_image_set_size(self->zimg, width, height);
    if(format && image_set_format(self, format, NULL))
        return(-1);
    if(data && image_set_data(self, data, NULL))
        return(-1);
    return(0);
}

static zbarImage*
image_convert (zbarImage *self,
               PyObject *args,
               PyObject *kwds)
{
    const char *format = NULL;
    int width = -1, height = -1;
    static char *kwlist[] = { "format", "width", "height", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|ii", kwlist,
                                    &format, &width, &height))
        return(NULL);
    assert(format);

    if(strlen(format) != 4) {
        PyErr_Format(PyExc_ValueError,
                     "format '%.50s' is not a valid four character code",
                     format);
        return(NULL);
    }

    zbarImage *img = PyObject_GC_New(zbarImage, &zbarImage_Type);
    if(!img)
        return(NULL);
    img->data = NULL;
    if(width > 0 && height > 0)
        img->zimg =
            zbar_image_convert_resize(self->zimg,
                                       *((unsigned long*)format),
                                       width, height);
    else
        img->zimg = zbar_image_convert(self->zimg, *((unsigned long*)format));

    if(!img->zimg) {
        /* FIXME propagate exception */
        Py_DECREF(img);
        return(NULL);
    }
    zbar_image_set_userdata(img->zimg, img);

    return(img);
}

static PyMethodDef image_methods[] = {
    { "convert",  (PyCFunction)image_convert, METH_VARARGS | METH_KEYWORDS, },
    { NULL, },
};

PyTypeObject zbarImage_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.Image",
    .tp_doc         = image_doc,
    .tp_basicsize   = sizeof(zbarImage),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)image_new,
    .tp_init        = (initproc)image_init,
    .tp_traverse    = (traverseproc)image_traverse,
    .tp_clear       = (inquiry)image_clear,
    .tp_dealloc     = (destructor)image_dealloc,
    .tp_getset      = image_getset,
    .tp_methods     = image_methods,
    .tp_iter        = (getiterfunc)image_iter,
};

zbarImage*
zbarImage_FromImage (zbar_image_t *zimg)
{
    zbarImage *self = PyObject_GC_New(zbarImage, &zbarImage_Type);
    if(!self)
        return(NULL);
    zbar_image_ref(zimg, 1);
    zbar_image_set_userdata(zimg, self);
    self->zimg = zimg;
    self->data = NULL;
    return(self);
}

int
zbarImage_validate (zbarImage *img)
{
    if(!zbar_image_get_width(img->zimg) ||
       !zbar_image_get_height(img->zimg) ||
       !zbar_image_get_data(img->zimg) ||
       !zbar_image_get_data_length(img->zimg)) {
        PyErr_Format(PyExc_ValueError, "image size and data must be defined");
        return(-1);
    }
    return(0);
}
