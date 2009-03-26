#include "zebramodule.h"

static char processor_doc[] = PyDoc_STR(
    "low level decode of measured bar/space widths.\n"
    "\n"
    "FIXME.");

static zebraProcessor*
processor_new (PyTypeObject *type,
               PyObject *args,
               PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zebraProcessor *self = (zebraProcessor*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zproc = zebra_processor_create(0/*FIXME*/);
    zebra_processor_set_userdata(self->zproc, self);
    if(!self->zproc) {
        Py_DECREF(self);
        return(NULL);
    }
    return(self);
}

static int
processor_traverse (zebraProcessor *self,
                    visitproc visit,
                    void *arg)
{
    Py_VISIT(self->handler);
    Py_VISIT(self->closure);
    return(0);
}

static int
processor_clear (zebraProcessor *self)
{
    zebra_processor_set_data_handler(self->zproc, NULL, NULL);
    zebra_processor_set_userdata(self->zproc, NULL);
    Py_CLEAR(self->handler);
    Py_CLEAR(self->closure);
    return(0);
}

static void
processor_dealloc (zebraProcessor *self)
{
    processor_clear(self);
    zebra_processor_destroy(self->zproc);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
processor_get_bool (zebraProcessor *self,
                    void *closure)
{
    int val;
    switch((int)closure) {
    case 0:
        val = zebra_processor_is_visible(self->zproc);
        break;
    default:
        assert(0);
        return(NULL);
    }
    if(val < 0)
        return(zebraErr_Set((PyObject*)self));
    return(PyBool_FromLong(val));
}

static int
processor_set_bool (zebraProcessor *self,
                    PyObject *value,
                    void *closure)
{
    if(!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return(-1);
    }
    int rc, val = PyObject_IsTrue(value);
    if(val < 0)
        return(-1);
    switch((int)closure) {
    case 0:
        rc = zebra_processor_set_visible(self->zproc, val);
        break;
    case 1:
        rc = zebra_processor_set_active(self->zproc, val);
        break;
    default:
        assert(0);
        return(-1);
    }
    if(rc < 0) {
        zebraErr_Set((PyObject*)self);
        return(-1);
    }
    return(0);
}

static PyGetSetDef processor_getset[] = {
    { "visible",  (getter)processor_get_bool, (setter)processor_set_bool,
      NULL, (void*)0 },
    { "active",   NULL,                       (setter)processor_set_bool,
      NULL, (void*)1 },
    { NULL, },
};

static PyObject*
processor_set_config (zebraProcessor *self,
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

    if(zebra_processor_set_config(self->zproc, sym, cfg, val)) {
        PyErr_SetString(PyExc_ValueError, "invalid configuration setting");
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
processor_init_ (zebraProcessor *self,
                 PyObject *args,
                 PyObject *kwds)
{
    const char *dev = "";
    int disp = 1;
    static char *kwlist[] = { "video_device", "enable_display", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|zO&", kwlist,
                                    &dev, object_to_bool, &disp))
        return(NULL);

    if(zebra_processor_init(self->zproc, dev, disp))
        return(zebraErr_Set((PyObject*)self));
    Py_RETURN_NONE;
}

static PyObject*
processor_parse_config (zebraProcessor *self,
                        PyObject *args,
                        PyObject *kwds)
{
    const char *cfg = NULL;
    static char *kwlist[] = { "config", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &cfg))
        return(NULL);

    if(zebra_processor_parse_config(self->zproc, cfg)) {
        PyErr_Format(PyExc_ValueError, "invalid configuration setting: %s",
                     cfg);
        return(NULL);
    }
    Py_RETURN_NONE;
}

static int
object_to_timeout (PyObject *obj,
                   int *val)
{
    int tmp;
    if(PyFloat_Check(obj))
        tmp = PyFloat_AS_DOUBLE(obj) * 1000;
    else
        tmp = PyInt_AsLong(obj) * 1000;
    if(tmp < 0 && PyErr_Occurred())
        return(0);
    *val = tmp;
    return(1);
}

static PyObject*
processor_user_wait (zebraProcessor *self,
                     PyObject *args,
                     PyObject *kwds)
{
    int timeout = -1;
    static char *kwlist[] = { "timeout", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                    object_to_timeout, &timeout))
        return(NULL);

    int rc = zebra_processor_user_wait(self->zproc, timeout);
    if(rc < 0)
        return(zebraErr_Set((PyObject*)self));
    return(PyInt_FromLong(rc));
}

static PyObject*
processor_process_one (zebraProcessor *self,
                       PyObject *args,
                       PyObject *kwds)
{
    int timeout = -1;
    static char *kwlist[] = { "timeout", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                    object_to_timeout, &timeout))
        return(NULL);

    int rc = zebra_process_one(self->zproc, timeout);
    if(rc < 0)
        return(zebraErr_Set((PyObject*)self));
    return(PyInt_FromLong(rc));
}

static PyObject*
processor_process_image (zebraProcessor *self,
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

    int n = zebra_process_image(self->zproc, img->zimg);
    if(n < 0)
        return(zebraErr_Set((PyObject*)self));
    return(PyInt_FromLong(n));
}

void
process_handler (zebra_image_t *zimg,
                 const void *userdata)
{
    zebraProcessor *self = (zebraProcessor*)userdata;
    assert(self);
    assert(self->handler);
    assert(self->closure);

    zebraImage *img = zebra_image_get_userdata(zimg);
    if(!img || img->zimg != zimg) {
        img = zebraImage_FromImage(zimg);
        if(!img) {
            PyErr_NoMemory();
            return;
        }
    }
    else
        Py_INCREF(img);

    PyObject *args = PyTuple_New(3);
    Py_INCREF(self);
    Py_INCREF(self->closure);
    PyTuple_SET_ITEM(args, 0, (PyObject*)self);
    PyTuple_SET_ITEM(args, 1, (PyObject*)img);
    PyTuple_SET_ITEM(args, 2, self->closure);

    PyObject *junk = PyObject_Call(self->handler, args, NULL);
    Py_XDECREF(junk);
    Py_DECREF(args);
}

static PyObject*
processor_set_data_handler (zebraProcessor *self,
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
    Py_CLEAR(self->closure);

    if(handler != Py_None) {
        Py_INCREF(handler);
        self->handler = handler;

        Py_INCREF(closure);
        self->closure = closure;

        zebra_processor_set_data_handler(self->zproc, process_handler, self);
    }
    else {
        self->handler = self->closure = NULL;
        zebra_processor_set_data_handler(self->zproc, NULL, self);
    }
    Py_RETURN_NONE;
}

static PyMethodDef processor_methods[] = {
    { "init",             (PyCFunction)processor_init_,
      METH_VARARGS | METH_KEYWORDS, },
    { "set_config",       (PyCFunction)processor_set_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "parse_config",     (PyCFunction)processor_parse_config,
      METH_VARARGS | METH_KEYWORDS, },
    { "user_wait",        (PyCFunction)processor_user_wait,
      METH_VARARGS | METH_KEYWORDS, },
    { "process_one",      (PyCFunction)processor_process_one,
      METH_VARARGS | METH_KEYWORDS, },
    { "process_image",    (PyCFunction)processor_process_image,
      METH_VARARGS | METH_KEYWORDS, },
    { "set_data_handler", (PyCFunction)processor_set_data_handler,
      METH_VARARGS | METH_KEYWORDS, },
    { NULL, },
};

PyTypeObject zebraProcessor_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zebra.Processor",
    .tp_doc         = processor_doc,
    .tp_basicsize   = sizeof(zebraProcessor),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)processor_new,
    .tp_traverse    = (traverseproc)processor_traverse,
    .tp_clear       = (inquiry)processor_clear,
    .tp_dealloc     = (destructor)processor_dealloc,
    .tp_getset      = processor_getset,
    .tp_methods     = processor_methods,
};
