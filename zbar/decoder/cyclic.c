/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <config.h>
#include <string.h>     /* memmove */

#include <zbar.h>

#ifdef DEBUG_CYCLIC
# define DEBUG_LEVEL (DEBUG_CYCLIC)
#endif
#include "debug.h"
#include "decoder.h"

#define Cyclic12CharactersCount 5

static uint8_t ElementWidthSequences[Cyclic12CharactersCount][12] = {
    {1,1,1,1,1,2,1,1,1,2,1,2},//黑1
    {1,1,1,1,1,2,2,1,2,1,1,2},//黑2
    {1,1,1,1,2,1,2,2,1,2,1,2},//黑5
};

static uint8_t CharacterCodes[Cyclic12CharactersCount] = {
    1,//黑1
    2,//黑2
    5,//黑5
};

void CyclicCharacterTreeAdd(CyclicCharacterTreeNode* root, int32_t leafValue, uint8_t* path, int length) {
    if (!root) return;
    
    if (0 == length)
    {
        root->leafValue = leafValue;
        return;
    }
    
    uint8_t c = path[0] + path[1] - 2;
    if (c < 0 || c > 2) return;
    
    CyclicCharacterTreeNode* child = root->children[c];
    if (!child)
    {
        child = CyclicCharacterTreeNodeCreate();
        root->children[c] = child;
    }
    
    CyclicCharacterTreeAdd(child, leafValue, path + 1, length - 1);
}

void cyclic_destroy (cyclic_decoder_t *dcodeCyclic)
{//TODO:
//    dcode128->direction = 0;
//    dcode128->element = 0;
//    dcode128->character = -1;
//    dcode128->s6 = 0;
    CyclicCharacterTreeNode* head = CyclicCharacterTreeNodeCreate();
    CyclicCharacterTreeNode* tail = head;
    head->children[0] = dcodeCyclic->charTree; // children[0] as value, children[1] as next
    while (head)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (!head->children[i]) continue;
            
            tail->children[1] = CyclicCharacterTreeNodeCreate();
            tail->children[1]->children[0] = head->children[i];
            tail = tail->children[1];
        }
        
        free(head->children[0]);
        
        CyclicCharacterTreeNode* next = head->children[1];
        free(head);
        head = next;
    }
}

void cyclic_reset (cyclic_decoder_t *dcodeCyclic)
{//TODO:
//    dcode128->direction = 0;
//    dcode128->element = 0;
//    dcode128->character = -1;
//    dcode128->s6 = 0;
    dcodeCyclic->charTree = CyclicCharacterTreeNodeCreate();
    for (int i = 0; i < Cyclic12CharactersCount; ++i)
    {
        uint8_t* seq = ElementWidthSequences[i];
        int length = sizeof(ElementWidthSequences[i]) / sizeof(ElementWidthSequences[i][0]) - 1;
        CyclicCharacterTreeAdd(dcodeCyclic->charTree, CharacterCodes[i], seq, length);
    }
}

zbar_symbol_type_t _zbar_decode_cyclic (zbar_decoder_t *dcode)
{
    cyclic_decoder_t* dcodeCyclic = &dcode->cyclic;
    //TODO:
//    signed char c;
//
//    /* update latest character width */
//    dcode128->s6 -= get_width(dcode, 6);
//    dcode128->s6 += get_width(dcode, 0);
//
//    if((dcode128->character < 0)
//       ? get_color(dcode) != ZBAR_SPACE
//       : (/* process every 6th element of active symbol */
//          ++dcode128->element != 6 ||
//          /* decode color based on direction */
//          get_color(dcode) != dcode128->direction))
//        return(0);
//    dcode128->element = 0;
//
//    dbprintf(2, "      code128[%c%02d+%x]:",
//             (dcode128->direction) ? '<' : '>',
//             dcode128->character, dcode128->element);
//
//    c = decode6(dcode);
//    if(dcode128->character < 0) {
//        unsigned qz;
//        dbprintf(2, " c=%02x", c);
//        if(c < START_A || c > STOP_REV || c == STOP_FWD) {
//            dbprintf(2, " [invalid]\n");
//            return(0);
//        }
//        qz = get_width(dcode, 6);
//        if(qz && qz < (dcode128->s6 * 3) / 4) {
//            dbprintf(2, " [invalid qz %d]\n", qz);
//            return(0);
//        }
//        /* decoded valid start/stop */
//        /* initialize state */
//        dcode128->character = 1;
//        if(c == STOP_REV) {
//            dcode128->direction = ZBAR_BAR;
//            dcode128->element = 7;
//        }
//        else
//            dcode128->direction = ZBAR_SPACE;
//        dcode128->start = c;
//        dcode128->width = dcode128->s6;
//        dbprintf(2, " dir=%x [valid start]\n", dcode128->direction);
//        return(0);
//    }
//    else if(c < 0 || size_buf(dcode, dcode128->character + 1)) {
//        dbprintf(1, (c < 0) ? " [aborted]\n" : " [overflow]\n");
//        if(dcode128->character > 1)
//            release_lock(dcode, ZBAR_CODE128);
//        dcode128->character = -1;
//        return(0);
//    }
//    else {
//        unsigned dw;
//        if(dcode128->width > dcode128->s6)
//            dw = dcode128->width - dcode128->s6;
//        else
//            dw = dcode128->s6 - dcode128->width;
//        dw *= 4;
//        if(dw > dcode128->width) {
//            dbprintf(1, " [width var]\n");
//            if(dcode128->character > 1)
//                release_lock(dcode, ZBAR_CODE128);
//            dcode128->character = -1;
//            return(0);
//        }
//    }
//    dcode128->width = dcode128->s6;
//
//    zassert(dcode->buf_alloc > dcode128->character, 0,
//            "alloc=%x idx=%x c=%02x %s\n",
//            dcode->buf_alloc, dcode128->character, c,
//            _zbar_decoder_buf_dump(dcode->buf, dcode->buf_alloc));
//
//    if(dcode128->character == 1) {
//        /* lock shared resources */
//        if(acquire_lock(dcode, ZBAR_CODE128)) {
//            dcode128->character = -1;
//            return(0);
//        }
//        dcode->buf[0] = dcode128->start;
//    }
//
//    dcode->buf[dcode128->character++] = c;
//
//    if(dcode128->character > 2 &&
//       ((dcode128->direction)
//        ? c >= START_A && c <= START_C
//        : c == STOP_FWD)) {
//        /* FIXME STOP_FWD should check extra bar (and QZ!) */
//        zbar_symbol_type_t sym = ZBAR_CODE128;
//        if(validate_checksum(dcode) || postprocess(dcode))
//            sym = ZBAR_NONE;
//        else if(dcode128->character < CFG(*dcode128, ZBAR_CFG_MIN_LEN) ||
//                (CFG(*dcode128, ZBAR_CFG_MAX_LEN) > 0 &&
//                 dcode128->character > CFG(*dcode128, ZBAR_CFG_MAX_LEN))) {
//            dbprintf(2, " [invalid len]\n");
//            sym = ZBAR_NONE;
//        }
//        else
//            dbprintf(2, " [valid end]\n");
//        dcode128->character = -1;
//        if(!sym)
//            release_lock(dcode, ZBAR_CODE128);
//        return(sym);
//    }
//
//    dbprintf(2, "\n");
    return(0);
}
