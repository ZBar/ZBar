/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#ifndef _CYCLIC_H_
#define _CYCLIC_H_

#include <string.h>

typedef struct CyclicCharacterTreeNode_s {
    struct CyclicCharacterTreeNode_s* children[3];
    int32_t leafValue;
} CyclicCharacterTreeNode;

static inline void CyclicCharacterTreeNodeReset(CyclicCharacterTreeNode* node) {
    memset(node, 0, sizeof(CyclicCharacterTreeNode));
    node->leafValue = -1;
}

CyclicCharacterTreeNode* CyclicCharacterTreeNodeCreate();

/* Cyclic specific decode state */
typedef struct cyclic_decoder_s {
    CyclicCharacterTreeNode* charTree;
    CyclicCharacterTreeNode*** charSeekers;//One group for each elements-of-character number
    uint8_t maxCharacterLength;
    uint8_t characterPhase;// This means sum of 2 elements - 2
//    uint8_t* s12OfChar;
    uint8_t minS12OfChar;
    uint8_t maxS12OfChar;
    
    unsigned s12;                /* character width */
    
    unsigned direction : 1;     /* scan direction: 0=fwd/space, 1=rev/bar */
    unsigned element : 3;       /* element offset 0-5 */
    int character : 12;         /* character position in symbol */
    unsigned char start;        /* start character */
    unsigned width;             /* last character width */

    unsigned config;
    int configs[NUM_CFGS];      /* int valued configurations */
} cyclic_decoder_t;

/* reset Cyclic specific state */
void cyclic_reset (cyclic_decoder_t *dcodeCyclic);

void cyclic_destroy (cyclic_decoder_t *dcodeCyclic);

/* decode Code 128 symbols */
zbar_symbol_type_t _zbar_decode_cyclic(zbar_decoder_t *dcode);

#endif
