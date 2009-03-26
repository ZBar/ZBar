#include "zebramodule.h"

static char symboliter_doc[] = PyDoc_STR(
    "symbol iterator.\n"
    "\n"
    "iterates over decode results attached to an image.");

static int
symboliter_traverse (zebraSymbolIter *self,
                     visitproc visit,
                     void *arg)
{
    Py_VISIT(self->img);
    return(0);
}

static int
symboliter_clear (zebraSymbolIter *self)
{
    Py_CLEAR(self->img);
    return(0);
}

static void
symboliter_dealloc (zebraSymbolIter *self)
{
    symboliter_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zebraSymbolIter*
symboliter_iter (zebraSymbolIter *self)
{
    Py_INCREF(self);
    return(self);
}

static zebraSymbol*
symboliter_iternext (zebraSymbolIter *self)
{
    if(!self->zsym)
        self->zsym = zebra_image_first_symbol(self->img->zimg);
    else
        self->zsym = zebra_symbol_next(self->zsym);
    if(!self->zsym)
        return(NULL);
    return(zebraSymbol_FromSymbol(self->img, self->zsym));
}

PyTypeObject zebraSymbolIter_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.SymbolIter",
    .tp_doc         = symboliter_doc,
    .tp_basicsize   = sizeof(zebraSymbolIter),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_traverse    = (traverseproc)symboliter_traverse,
    .tp_clear       = (inquiry)symboliter_clear,
    .tp_dealloc     = (destructor)symboliter_dealloc,
    .tp_iter        = (getiterfunc)symboliter_iter,
    .tp_iternext    = (iternextfunc)symboliter_iternext,
};

static zebraSymbolIter*
zebraSymbolIter_New (zebraImage *img)
{
    zebraSymbolIter *self;
    self = PyObject_GC_New(zebraSymbolIter, &zebraSymbolIter_Type);
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

static zebraImage*
image_new (PyTypeObject *type,
           PyObject *args,
           PyObject *kwds)
{
    zebraImage *self = (zebraImage*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zimg = zebra_image_create();
    if(!self->zimg) {
        Py_DECREF(self);
        return(NULL);
    }
    zebra_image_set_userdata(self->zimg, self);
    return(self);
}

static int
image_traverse (zebraImage *self,
                visitproc visit,
                void *arg)
{
    Py_VISIT(self->data);
    return(0);
}

static int
image_clear (zebraImage *self)
{
    zebra_image_t *zimg = self->zimg;
    self->zimg = NULL;
    if(zimg) {
        assert(zebra_image_get_userdata(zimg) == self);
        if(self->data) {
            /* attach data directly to zebra image */
            zebra_image_set_userdata(zimg, self->data);
            self->data = NULL;
        }
        else
            zebra_image_set_userdata(zimg, NULL);
        zebra_image_destroy(zimg);
    }
    return(0);
}

static void
image_dealloc (zebraImage *self)
{
    image_clear(self);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static zebraSymbolIter*
image_iter (zebraImage *self)
{
    return(zebraSymbolIter_New(self));
}

static PyObject*
image_get_format (zebraImage *self,
                  void *closure)
{
    unsigned long format = zebra_image_get_format(self->zimg);
    return(PyString_FromStringAndSize((char*)&format, 4));
}

static int
image_set_format (zebraImage *self,
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
    zebra_image_set_format(self->zimg,*((unsigned long*)format));
    return(0);
}

static PyObject*
image_get_size (zebraImage *self,
                void *closure)
{
    unsigned int width = zebra_image_get_width(self->zimg);
    unsigned int height = zebra_image_get_height(self->zimg);
    return(PyTuple_Pack(2, PyInt_FromLong(width), PyInt_FromLong(height)));
}

static int
image_set_size (zebraImage *self,
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

    zebra_image_set_size(self->zimg, width, height);
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
image_get_int (zebraImage *self,
               void *closure)
{
    unsigned int val = -1;
    switch((int)closure) {
    case 0:
        val = zebra_image_get_width(self->zimg); break;
    case 1:
        val = zebra_image_get_height(self->zimg); break;
    case 2:
        val = zebra_image_get_sequence(self->zimg); break;
    default:
        assert(0);
    }
    return(PyInt_FromLong(val));
}

static int
image_set_int (zebraImage *self,
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
        tmp = zebra_image_get_height(self->zimg);
        zebra_image_set_size(self->zimg, val, tmp);
        break;
    case 1:
        tmp = zebra_image_get_width(self->zimg);
        zebra_image_set_size(self->zimg, tmp, val);
        break;
    case 2:
        zebra_image_set_sequence(self->zimg, val);
    default:
        assert(0);
    }
    return(0);
}

static PyObject*
image_get_data (zebraImage *self,
                void *closure)
{
    assert(zebra_image_get_userdata(self->zimg) == self);
    if(self->data) {
        Py_INCREF(self->data);
        return(self->data);
    }

    const char *data = zebra_image_get_data(self->zimg);
    unsigned long datalen = zebra_image_get_data_length(self->zimg);
    if(!data || !datalen) {
        Py_INCREF(Py_None);
        return(Py_None);
    }

    self->data = PyBuffer_FromMemory((void*)data, datalen);
    Py_INCREF(self->data);
    return(self->data);
}

void
image_cleanup (zebra_image_t *zimg)
{
    PyObject *data = zebra_image_get_userdata(zimg);
    zebra_image_set_userdata(zimg, NULL);
    if(!data)
        return;  /* FIXME internal error */
    if(PyObject_TypeCheck(data, &zebraImage_Type)) {
        zebraImage *self = (zebraImage*)data;
        assert(self->zimg == zimg);
        Py_CLEAR(self->data);
    }
    else
        Py_DECREF(data);
}

static int
image_set_data (zebraImage *self,
                PyObject *value,
                void *closure)
{
    if(!value) {
        zebra_image_free_data(self->zimg);
        return(0);
    }
    char *data;
    Py_ssize_t datalen;
    if(PyString_AsStringAndSize(value, &data, &datalen))
        return(-1);

    Py_INCREF(value);
    zebra_image_set_data(self->zimg, data, datalen, image_cleanup);
    assert(!self->data);
    self->data = value;
    zebra_image_set_userdata(self->zimg, self);
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
image_init (zebraImage *self,
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
        zebra_image_set_size(self->zimg, width, height);
    if(format && image_set_format(self, format, NULL))
        return(-1);
    if(data && image_set_data(self, data, NULL))
        return(-1);
    return(0);
}

static zebraImage*
image_convert (zebraImage *self,
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

    zebraImage *img = PyObject_GC_New(zebraImage, &zebraImage_Type);
    if(!img)
        return(NULL);
    img->data = NULL;
    if(width > 0 && height > 0)
        img->zimg =
            zebra_image_convert_resize(self->zimg,
                                       *((unsigned long*)format),
                                       width, height);
    else
        img->zimg = zebra_image_convert(self->zimg, *((unsigned long*)format));

    if(!img->zimg) {
        /* FIXME propagate exception */
        Py_DECREF(img);
        return(NULL);
    }
    zebra_image_set_userdata(img->zimg, img);

    return(img);
}

static PyMethodDef image_methods[] = {
    { "convert",  (PyCFunction)image_convert, METH_VARARGS | METH_KEYWORDS, },
    { NULL, },
};

PyTypeObject zebraImage_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Image",
    .tp_doc         = image_doc,
    .tp_basicsize   = sizeof(zebraImage),
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

zebraImage*
zebraImage_FromImage (zebra_image_t *zimg)
{
    zebraImage *self = PyObject_GC_New(zebraImage, &zebraImage_Type);
    if(!self)
        return(NULL);
    zebra_image_ref(zimg, 1);
    zebra_image_set_userdata(zimg, self);
    self->zimg = zimg;
    self->data = NULL;
    return(self);
}

int
zebraImage_validate (zebraImage *img)
{
    if(!zebra_image_get_width(img->zimg) ||
       !zebra_image_get_height(img->zimg) ||
       !zebra_image_get_data(img->zimg) ||
       !zebra_image_get_data_length(img->zimg)) {
        PyErr_Format(PyExc_ValueError, "image size and data must be defined");
        return(-1);
    }
    return(0);
}
