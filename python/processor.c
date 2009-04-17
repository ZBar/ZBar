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

static char processor_doc[] = PyDoc_STR(
    "low level decode of measured bar/space widths.\n"
    "\n"
    "FIXME.");

static zbarProcessor*
processor_new (PyTypeObject *type,
               PyObject *args,
               PyObject *kwds)
{
    static char *kwlist[] = { NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return(NULL);

    zbarProcessor *self = (zbarProcessor*)type->tp_alloc(type, 0);
    if(!self)
        return(NULL);

    self->zproc = zbar_processor_create(0/*FIXME*/);
    zbar_processor_set_userdata(self->zproc, self);
    if(!self->zproc) {
        Py_DECREF(self);
        return(NULL);
    }
    return(self);
}

static int
processor_traverse (zbarProcessor *self,
                    visitproc visit,
                    void *arg)
{
    Py_VISIT(self->handler);
    Py_VISIT(self->closure);
    return(0);
}

static int
processor_clear (zbarProcessor *self)
{
    zbar_processor_set_data_handler(self->zproc, NULL, NULL);
    zbar_processor_set_userdata(self->zproc, NULL);
    Py_CLEAR(self->handler);
    Py_CLEAR(self->closure);
    return(0);
}

static void
processor_dealloc (zbarProcessor *self)
{
    processor_clear(self);
    zbar_processor_destroy(self->zproc);
    ((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

static PyObject*
processor_get_bool (zbarProcessor *self,
                    void *closure)
{
    int val;
    switch((int)closure) {
    case 0:
        val = zbar_processor_is_visible(self->zproc);
        break;
    default:
        assert(0);
        return(NULL);
    }
    if(val < 0)
        return(zbarErr_Set((PyObject*)self));
    return(PyBool_FromLong(val));
}

static int
processor_set_bool (zbarProcessor *self,
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
        rc = zbar_processor_set_visible(self->zproc, val);
        break;
    case 1:
        rc = zbar_processor_set_active(self->zproc, val);
        break;
    default:
        assert(0);
        return(-1);
    }
    if(rc < 0) {
        zbarErr_Set((PyObject*)self);
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
processor_set_config (zbarProcessor *self,
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

    if(zbar_processor_set_config(self->zproc, sym, cfg, val)) {
        PyErr_SetString(PyExc_ValueError, "invalid configuration setting");
        return(NULL);
    }
    Py_RETURN_NONE;
}

static PyObject*
processor_init_ (zbarProcessor *self,
                 PyObject *args,
                 PyObject *kwds)
{
    const char *dev = "";
    int disp = 1;
    static char *kwlist[] = { "video_device", "enable_display", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|zO&", kwlist,
                                    &dev, object_to_bool, &disp))
        return(NULL);

    if(zbar_processor_init(self->zproc, dev, disp))
        return(zbarErr_Set((PyObject*)self));
    Py_RETURN_NONE;
}

static PyObject*
processor_parse_config (zbarProcessor *self,
                        PyObject *args,
                        PyObject *kwds)
{
    const char *cfg = NULL;
    static char *kwlist[] = { "config", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &cfg))
        return(NULL);

    if(zbar_processor_parse_config(self->zproc, cfg)) {
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
processor_user_wait (zbarProcessor *self,
                     PyObject *args,
                     PyObject *kwds)
{
    int timeout = -1;
    static char *kwlist[] = { "timeout", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                    object_to_timeout, &timeout))
        return(NULL);

    int rc = zbar_processor_user_wait(self->zproc, timeout);
    if(rc < 0)
        return(zbarErr_Set((PyObject*)self));
    return(PyInt_FromLong(rc));
}

static PyObject*
processor_process_one (zbarProcessor *self,
                       PyObject *args,
                       PyObject *kwds)
{
    int timeout = -1;
    static char *kwlist[] = { "timeout", NULL };
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
                                    object_to_timeout, &timeout))
        return(NULL);

    int rc = zbar_process_one(self->zproc, timeout);
    if(rc < 0)
        return(zbarErr_Set((PyObject*)self));
    return(PyInt_FromLong(rc));
}

static PyObject*
processor_process_image (zbarProcessor *self,
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

    int n = zbar_process_image(self->zproc, img->zimg);
    if(n < 0)
        return(zbarErr_Set((PyObject*)self));
    return(PyInt_FromLong(n));
}

void
process_handler (zbar_image_t *zimg,
                 const void *userdata)
{
    zbarProcessor *self = (zbarProcessor*)userdata;
    assert(self);
    assert(self->handler);
    assert(self->closure);

    zbarImage *img = zbar_image_get_userdata(zimg);
    if(!img || img->zimg != zimg) {
        img = zbarImage_FromImage(zimg);
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
processor_set_data_handler (zbarProcessor *self,
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

        zbar_processor_set_data_handler(self->zproc, process_handler, self);
    }
    else {
        self->handler = self->closure = NULL;
        zbar_processor_set_data_handler(self->zproc, NULL, self);
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

PyTypeObject zbarProcessor_Type = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "zbar.Processor",
    .tp_doc         = processor_doc,
    .tp_basicsize   = sizeof(zbarProcessor),
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                      Py_TPFLAGS_HAVE_GC,
    .tp_new         = (newfunc)processor_new,
    .tp_traverse    = (traverseproc)processor_traverse,
    .tp_clear       = (inquiry)processor_clear,
    .tp_dealloc     = (destructor)processor_dealloc,
    .tp_getset      = processor_getset,
    .tp_methods     = processor_methods,
};
