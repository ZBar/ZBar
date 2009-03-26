#include <Python.h>
#include <stddef.h>
#include <zebra.h>

#ifndef _ZEBRAMODULE_H_
#define _ZEBRAMODULE_H_

typedef struct {
    PyBaseExceptionObject base;
    PyObject *obj;
} zebraException;

extern PyTypeObject zebraException_Type;
extern PyObject *zebra_exc[ZEBRA_ERR_NUM];

extern PyObject *zebraErr_Set(PyObject *self);

typedef struct {
    PyIntObject val;            /* integer value is super type */
    PyObject *name;             /* associated string name */
} zebraEnumItem;

extern PyTypeObject zebraEnumItem_Type;

extern zebraEnumItem *zebraEnumItem_New(PyObject *byname,
                                        PyObject *byvalue,
                                        int val,
                                        const char *name);

typedef struct {
    PyObject_HEAD
    PyObject *byname, *byvalue; /* zebraEnumItem content dictionaries */
} zebraEnum;

extern PyTypeObject zebraEnum_Type;

extern zebraEnum *zebraEnum_New();
extern int zebraEnum_Add(zebraEnum *self,
                         int val,
                         const char *name);

typedef struct {
    PyObject_HEAD
    zebra_image_t *zimg;
    PyObject *data;
} zebraImage;

extern PyTypeObject zebraImage_Type;

extern zebraImage *zebraImage_FromImage(zebra_image_t *zimg);
extern int zebraImage_validate(zebraImage *image);

typedef struct {
    PyObject_HEAD
    const zebra_symbol_t *zsym;
    zebraImage *img;
    PyObject *data;
    PyObject *loc;
} zebraSymbol;

extern PyTypeObject zebraSymbol_Type;

extern zebraSymbol *zebraSymbol_FromSymbol(zebraImage *img,
                                           const zebra_symbol_t *zsym);
extern zebraEnumItem *zebraSymbol_LookupEnum(zebra_symbol_type_t type);

typedef struct {
    PyObject_HEAD
    const zebra_symbol_t *zsym;
    zebraImage *img;
} zebraSymbolIter;

extern PyTypeObject zebraSymbolIter_Type;

typedef struct {
    PyObject_HEAD
    zebra_processor_t *zproc;
    PyObject *handler;
    PyObject *closure;
} zebraProcessor;

extern PyTypeObject zebraProcessor_Type;

#define zebraProcessor_Check(obj) PyObject_TypeCheck(obj, &zebraProcessor_Type)

typedef struct {
    PyObject_HEAD
    zebra_image_scanner_t *zscn;
} zebraImageScanner;

extern PyTypeObject zebraImageScanner_Type;

typedef struct {
    PyObject_HEAD
    zebra_decoder_t *zdcode;
    PyObject *handler;
    PyObject *args;
} zebraDecoder;

extern PyTypeObject zebraDecoder_Type;

typedef struct {
    PyObject_HEAD
    zebra_scanner_t *zscn;
    zebraDecoder *decoder;
} zebraScanner;

extern PyTypeObject zebraScanner_Type;

extern zebraEnumItem *color_enum[2];
extern zebraEnum *config_enum;
extern PyObject *symbol_enum;
extern zebraEnumItem *symbol_NONE;

int object_to_bool(PyObject *obj,
                   int *val);

#endif
