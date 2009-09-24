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

#include <Python.h>
#include <stddef.h>
#include <zbar.h>

#ifndef _ZBARMODULE_H_
#define _ZBARMODULE_H_

typedef struct {
    PyBaseExceptionObject base;
    PyObject *obj;
} zbarException;

extern PyTypeObject zbarException_Type;
extern PyObject *zbar_exc[ZBAR_ERR_NUM];

extern PyObject *zbarErr_Set(PyObject *self);

typedef struct {
    PyIntObject val;            /* integer value is super type */
    PyObject *name;             /* associated string name */
} zbarEnumItem;

extern PyTypeObject zbarEnumItem_Type;

extern zbarEnumItem *zbarEnumItem_New(PyObject *byname,
                                        PyObject *byvalue,
                                        int val,
                                        const char *name);

typedef struct {
    PyObject_HEAD
    PyObject *byname, *byvalue; /* zbarEnumItem content dictionaries */
} zbarEnum;

extern PyTypeObject zbarEnum_Type;

extern zbarEnum *zbarEnum_New(void);
extern int zbarEnum_Add(zbarEnum *self,
                         int val,
                         const char *name);

typedef struct {
    PyObject_HEAD
    zbar_image_t *zimg;
    PyObject *data;
} zbarImage;

extern PyTypeObject zbarImage_Type;

extern zbarImage *zbarImage_FromImage(zbar_image_t *zimg);
extern int zbarImage_validate(zbarImage *image);

typedef struct {
    PyObject_HEAD
    const zbar_symbol_set_t *zsyms;
} zbarSymbolSet;

extern PyTypeObject zbarSymbolSet_Type;

extern zbarSymbolSet*
zbarSymbolSet_FromSymbolSet(const zbar_symbol_set_t *zsyms);

#define zbarSymbolSet_Check(obj) PyObject_TypeCheck(obj, &zbarSymbolSet_Type)

typedef struct {
    PyObject_HEAD
    const zbar_symbol_t *zsym;
    PyObject *data;
    PyObject *loc;
} zbarSymbol;

extern PyTypeObject zbarSymbol_Type;

extern zbarSymbol *zbarSymbol_FromSymbol(const zbar_symbol_t *zsym);
extern zbarEnumItem *zbarSymbol_LookupEnum(zbar_symbol_type_t type);

typedef struct {
    PyObject_HEAD
    const zbar_symbol_t *zsym;
    zbarSymbolSet *syms;
} zbarSymbolIter;

extern PyTypeObject zbarSymbolIter_Type;

extern zbarSymbolIter *zbarSymbolIter_FromSymbolSet(zbarSymbolSet *syms);

typedef struct {
    PyObject_HEAD
    zbar_processor_t *zproc;
    PyObject *handler;
    PyObject *closure;
} zbarProcessor;

extern PyTypeObject zbarProcessor_Type;

#define zbarProcessor_Check(obj) PyObject_TypeCheck(obj, &zbarProcessor_Type)

typedef struct {
    PyObject_HEAD
    zbar_image_scanner_t *zscn;
} zbarImageScanner;

extern PyTypeObject zbarImageScanner_Type;

typedef struct {
    PyObject_HEAD
    zbar_decoder_t *zdcode;
    PyObject *handler;
    PyObject *args;
} zbarDecoder;

extern PyTypeObject zbarDecoder_Type;

typedef struct {
    PyObject_HEAD
    zbar_scanner_t *zscn;
    zbarDecoder *decoder;
} zbarScanner;

extern PyTypeObject zbarScanner_Type;

extern zbarEnumItem *color_enum[2];
extern zbarEnum *config_enum;
extern PyObject *symbol_enum;
extern zbarEnumItem *symbol_NONE;

int object_to_bool(PyObject *obj,
                   int *val);

#endif
