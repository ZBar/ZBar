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

#define MinRepeatingRequired 3

//#define TestCyclic

//static int g_mallocedNodesCount = 0;

typedef struct {
    const char* name;
    int16_t elementSequence[12];
} CyclicCode;

static CyclicCode Codes[] = {
//    {"DK", {1,1,1,1,2,1,2,1,1,1,1,2}},
//    {"DQ", {1,1,1,1,1,2,1,2,1,1,2,2}},
//    {"DJ", {1,1,1,1,2,1,2,1,2,1,1,2}},
//    {"DX", {1,1,1,1,2,1,1,2,2,1,1,2}},
//    {"D9", {1,1,1,1,2,1,2,2,1,1,1,2}},
//    {"D8", {1,1,1,1,2,2,2,1,1,1,1,2}},
//    {"D7", {1,1,1,1,2,1,1,2,2,2,1,2}},
//    {"D6", {1,1,1,1,2,1,2,2,1,1,2,2}},
//    {"D5", {1,1,1,1,2,2,1,1,2,1,2,2}},
//    {"D4", {1,1,1,1,2,2,1,2,2,1,1,2}},
//    {"D3", {1,1,1,1,2,2,2,2,1,1,1,2}},
//    {"D2", {1,1,1,1,2,1,1,1,2,1,2,2}},
//    {"DA", {1,1,1,1,1,2,2,1,1,1,1,2}},

//    {"SK", {1,1,1,1,2,1,1,1,1,2,2,1}},
//    {"SQ", {1,1,1,1,1,2,1,1,1,1,1,2}},
//    {"SJ", {1,1,1,1,1,1,1,1,1,1,1,2}},
//    {"SX", {1,1,1,1,2,1,1,1,2,2,1,2}},
//    {"S9", {1,1,1,1,2,1,2,1,1,1,2,2}},
//    {"S8", {1,1,1,1,2,2,1,1,1,2,1,2}},
//    {"S7", {1,1,1,1,2,1,1,1,2,2,2,2}},
//    {"S6", {1,1,1,1,2,1,2,1,1,1,1,2}},
//    {"S5", {1,1,1,1,2,1,2,1,2,2,1,2}},
//    {"S4", {1,1,1,1,2,2,2,2,1,2,2,1}},
//    {"S3", {1,1,1,1,2,2,2,1,1,1,2,2}},
//    {"S2", {1,1,1,1,1,2,2,1,2,1,1,2}},
//    {"SA", {1,1,1,1,1,2,1,1,1,2,1,2}},

//    {"HK", {1,1,1,1,2,1,1,1,2,1,1,2}},
//    {"HQ", {1,1,1,1,1,2,1,1,2,2,1,1}},
//    {"HJ", {1,1,1,1,1,2,1,2,2,1,1,2}},
//    {"HX", {1,1,1,1,2,1,1,2,1,1,2,2}},
//    {"H9", {1,1,1,1,2,1,2,1,2,1,1,2}},
//    {"H8", {1,1,1,1,2,1,1,1,1,2,1,2}},
//    {"H7", {1,1,1,1,2,1,1,1,1,1,1,1}},
//    {"H6", {1,1,1,1,2,1,2,1,2,1,2,2}},
//    {"H5", {1,1,1,1,2,1,2,2,2,1,1,2}},
//    {"H4", {1,1,1,1,2,2,1,2,1,1,2,2}},
//    {"H3", {1,1,1,1,2,2,2,1,1,2,1,2}},
//    {"H2", {1,1,1,1,1,2,2,2,1,1,1,1}},
//    {"HA", {1,1,1,1,1,2,1,1,2,1,1,1}},

    {"CK", {1,1,1,1,2,1,1,2,1,1,1,2}},
    {"CQ", {1,1,1,1,1,2,1,1,2,2,1,2}},
    {"CJ", {1,1,1,1,1,1,1,2,1,1,1,2}},
    {"CX", {1,1,1,1,2,1,1,1,1,1,1,1}},
    {"C9", {1,1,1,1,2,1,2,1,2,1,1,2}},
    {"C8", {1,1,1,1,2,2,1,2,1,1,1,2}},
    {"C7", {1,1,1,1,2,1,1,2,2,1,2,2}},
    {"C6", {1,1,1,1,2,1,2,1,2,1,1,1}},
    {"C5", {1,1,1,1,2,2,1,1,1,2,2,2}},
    {"C4", {1,1,1,1,2,2,1,2,1,2,1,2}},
    {"C3", {1,1,1,1,2,2,2,1,2,1,1,2}},
    {"C2", {1,1,1,1,2,1,1,1,1,2,2,2}},
    {"CA", {1,1,1,1,1,2,1,2,1,1,1,2}},
//
//    {"JB", {1,1,1,1,1,2,2,2,2,1,1,2}},
//    {"JS", {1,1,1,1,1,2,2,2,1,2,1,2}},
//    {"AD", {1,1,1,1,2,1,2,1,1,2,2,1}},
};

static int16_t TestPairWidths[] = {
//    0,0,0,0,1,1,0,0,1,1,1,
//    0,0,0,0,1,2,1,1,1,0,1,
//    0,0,0,1,1,1,2,1,1,1,1,

    0,0,0,0,1,1,0,0,0,0,1,2,1,1,1,0,0,0,0,1,1,0,0,1,1,1,
    0,0,0,1,1,1,2,1,1,1,1,
    
//    0,1,1,1,
//    1,1,0,1,
//    1,1,1,1,
};

void CyclicCharacterTreeAdd(CyclicCharacterTreeNode* root, int16_t leafValue, int16_t* path, int length) {
    if (!root) return;
    
    if (0 == length)
    {//printf("#Cyclic# leafValue=%d\n", leafValue);
        root->leafValue = leafValue;
        return;
    }
    
    int16_t c = path[0] + path[1] - 2;
    if (c < 0 || c > 2) return;
    
    CyclicCharacterTreeNode* child = root->children[c];
    if (!child)
    {
        child = CyclicCharacterTreeNodeCreate();
        //printf("#Cyclic# Add child as #%d\n", c);
        root->children[c] = child;
    }
    
    CyclicCharacterTreeAdd(child, leafValue, path + 1, length - 1);
}

CyclicCharacterTreeNode* CyclicCharacterTreeNodeCreate() {
    CyclicCharacterTreeNode* ret = (CyclicCharacterTreeNode*) malloc(sizeof(CyclicCharacterTreeNode));
    //printf("#Cyclic# New node: %d\n", ++g_mallocedNodesCount);
    CyclicCharacterTreeNodeReset(ret);
    return ret;
}

CyclicCharacterTreeNode* CyclicCharacterTreeNodeNext(const CyclicCharacterTreeNode* current, int16_t c) {
    if (!current) return NULL;
    
    if (c < 0 || c > 2) return NULL;
    
    return current->children[c];
}

int16_t cyclic_feed_element(cyclic_decoder_t* decoder, int16_t pairWidth)
{
    int16_t ret = -1;
    if (!decoder) return ret;
#ifdef TestCyclic
    if (pairWidth < 0 || pairWidth > 2) return ret;
#endif
    for (int iS12OfChar = decoder->maxS12OfChar - decoder->minS12OfChar;
         iS12OfChar >= 0; --iS12OfChar)
    {
#ifdef TestCyclic
        int16_t e = pairWidth;///!!!For Test
#else
        int16_t s12OfChar = decoder->minS12OfChar + iS12OfChar;
        int e = decode_e(pairWidth, decoder->s12, s12OfChar);
//        printf("#Barcodes# e=%d. pairWidth=%d, s12=%d, n=%d\n", e, pairWidth, decoder->s12, s12OfChar);
#endif
        CyclicCharacterTreeNode** charSeekers = decoder->charSeekers[iS12OfChar];
        for (int i = decoder->maxCharacterLength - 1; i >= 0; --i)
        {
            if (e < 0 || e > 2)
            {
                charSeekers[i] = NULL;
            }
            else if (!charSeekers[i])
            {
                if (decoder->characterPhase == i)
                {
                    charSeekers[i] = decoder->charTree->children[e];
                }
            }
            else
            {
                charSeekers[i] = charSeekers[i]->children[e];
                if (charSeekers[i] && charSeekers[i]->leafValue > -1)
                {
                    ret = charSeekers[i]->leafValue;
                    printf("#Cyclic# A character found: %s\n", Codes[charSeekers[i]->leafValue].name);
                    charSeekers[i] = NULL;
                }
            }
        }
    }
    
    if (++decoder->characterPhase == decoder->maxCharacterLength)
    {
        decoder->characterPhase = 0;
    }
    
    return ret;
}

void cyclic_destroy (cyclic_decoder_t *decoder)
{//TODO:
//    dcode128->direction = 0;
//    dcode128->element = 0;
//    dcode128->character = -1;
//    dcode128->s6 = 0;
    CyclicCharacterTreeNode* head = CyclicCharacterTreeNodeCreate();
    CyclicCharacterTreeNode* tail = head;
    head->children[0] = decoder->charTree; // children[0] as value, children[1] as next
    while (head)
    {
        for (int i = 0; i < 3; ++i)
        {
            CyclicCharacterTreeNode* parent = head->children[0];
            if (!parent->children[i]) continue;
            
            tail->children[1] = CyclicCharacterTreeNodeCreate();
            tail->children[1]->children[0] = parent->children[i];
            tail = tail->children[1];
        }
        
        free(head->children[0]);
        //printf("#Cyclic# Delete node: %d\n", --g_mallocedNodesCount);
        CyclicCharacterTreeNode* next = head->children[1];
        free(head);
        //printf("#Cyclic# Delete node: %d\n", --g_mallocedNodesCount);
        head = next;
    }
    
    if (decoder->charSeekers)
    {
        for (int i = decoder->maxS12OfChar - decoder->minS12OfChar; i >= 0; --i)
        {
            free(decoder->charSeekers[i]);
        }
        free(decoder->charSeekers);
    }
//    if (decoder->s12OfChar) free(decoder->s12OfChar);
}

void cyclic_reset (cyclic_decoder_t *decoder)
{//TODO:
//    dcode128->direction = 0;
//    dcode128->element = 0;
//    dcode128->character = -1;
//    dcode128->s6 = 0;
    decoder->s12 = 0;
//    CyclicCharacterTreeNode*** charSeekers;//One group for each elements-of-character number
//    int16_t maxCharacterLength;
//    int16_t characterPhase;// This means sum of 2 elements - 2
//    int16_t* s12OfChar;
//    int16_t minS12OfChar;
//    int16_t maxS12OfChar;
    decoder->charTree = CyclicCharacterTreeNodeCreate();
    decoder->maxCharacterLength = 0;
//    decoder->s12OfChar = (int16_t*) malloc(Cyclic12CharactersCount);
    decoder->minS12OfChar = 127;
    decoder->maxS12OfChar = 0;
    for (int i = sizeof(Codes) / sizeof(Codes[0]) - 1; i >= 0; --i)
    {
        int16_t* seq = Codes[i].elementSequence;
        int length = sizeof(Codes[i].elementSequence) / sizeof(Codes[i].elementSequence[0]) - 1;
        if (length > decoder->maxCharacterLength)
        {
            decoder->maxCharacterLength = length;
        }
        
        int16_t s12OfChar = 0;
        for (int j = length; j >= 0; --j)
        {
            s12OfChar += Codes[i].elementSequence[j];
        }
//        decoder->s12OfChar[i] = s12OfChar;
        if (s12OfChar > decoder->maxS12OfChar)
        {
            decoder->maxS12OfChar = s12OfChar;
        }
        if (s12OfChar < decoder->minS12OfChar)
        {
            decoder->minS12OfChar = s12OfChar;
        }
        
        //printf("\n---------------------\n");
        CyclicCharacterTreeAdd(decoder->charTree, i, seq, length);
    }
    //printf("#Cyclic# maxCharacterLength: %d\n", decoder->maxCharacterLength);
    decoder->charSeekers = (CyclicCharacterTreeNode***) malloc(sizeof(CyclicCharacterTreeNode**) * (decoder->maxS12OfChar - decoder->minS12OfChar + 1));
    for (int i = decoder->maxS12OfChar - decoder->minS12OfChar; i >= 0; --i)
    {
        decoder->charSeekers[i] = (CyclicCharacterTreeNode**) malloc(sizeof(CyclicCharacterTreeNode*) * decoder->maxCharacterLength);
        memset(decoder->charSeekers[i], 0, sizeof(CyclicCharacterTreeNode*) * decoder->maxCharacterLength);
    }

    decoder->characterPhase = 0;
    
    decoder->candidate = -1;
    decoder->repeatingCount = 0;
    decoder->nonRepeatingSpan = 0;
#ifdef TestCyclic
    ///!!!For Test:
    for (int i = 0; i < sizeof(TestPairWidths) / sizeof(TestPairWidths[0]); ++i)
    {
        cyclic_feed_element(decoder, TestPairWidths[i]);
    }
    ///!!!:For Test
#endif
}

zbar_symbol_type_t _zbar_decode_cyclic (zbar_decoder_t *dcode)
{
    cyclic_decoder_t* decoder = &dcode->cyclic;
    
    decoder->s12 -= get_width(dcode, 12);
    decoder->s12 += get_width(dcode, 0);
    
    int16_t c = cyclic_feed_element(decoder, pair_width(dcode, 0));
    if (c > -1)
    {
        if (c == decoder->candidate)
        {
            decoder->nonRepeatingSpan = 0;
            if (++decoder->repeatingCount == MinRepeatingRequired)
            {
                release_lock(dcode, ZBAR_CYCLIC);
                
                decoder->candidate = -1;
                decoder->repeatingCount = 0;

                int length = (int)(strlen(Codes[c].name) + 1);
                size_buf(dcode, length);
                memcpy(dcode->buf, Codes[c].name, length);
                dcode->buflen = length;
                
                return(ZBAR_CYCLIC);
            }
        }
        else
        {
            decoder->repeatingCount = 1;
            decoder->candidate = c;
            decoder->nonRepeatingSpan = 0;
            
            acquire_lock(dcode, ZBAR_CYCLIC);
            
            return(ZBAR_PARTIAL);
        }
    }
    else if (decoder->candidate > -1)
    {
        if (++decoder->nonRepeatingSpan > sizeof(Codes[decoder->candidate].elementSequence) / sizeof(Codes[decoder->candidate].elementSequence[0]))
        {
            decoder->candidate = -1;
            decoder->repeatingCount = 0;
            decoder->nonRepeatingSpan = 0;
            
            release_lock(dcode, ZBAR_CYCLIC);
        }
    }
//    int e = decode_e(pair_width(decoder, 0), s, );
    
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
//    db//printf(2, "      code128[%c%02d+%x]:",
//             (dcode128->direction) ? '<' : '>',
//             dcode128->character, dcode128->element);
//
//    c = decode6(dcode);
//    if(dcode128->character < 0) {
//        unsigned qz;
//        db//printf(2, " c=%02x", c);
//        if(c < START_A || c > STOP_REV || c == STOP_FWD) {
//            db//printf(2, " [invalid]\n");
//            return(0);
//        }
//        qz = get_width(dcode, 6);
//        if(qz && qz < (dcode128->s6 * 3) / 4) {
//            db//printf(2, " [invalid qz %d]\n", qz);
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
//        db//printf(2, " dir=%x [valid start]\n", dcode128->direction);
//        return(0);
//    }
//    else if(c < 0 || size_buf(dcode, dcode128->character + 1)) {
//        db//printf(1, (c < 0) ? " [aborted]\n" : " [overflow]\n");
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
//            db//printf(1, " [width var]\n");
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
//            db//printf(2, " [invalid len]\n");
//            sym = ZBAR_NONE;
//        }
//        else
//            db//printf(2, " [valid end]\n");
//        dcode128->character = -1;
//        if(!sym)
//            release_lock(dcode, ZBAR_CODE128);
//        return(sym);
//    }
//
//    db//printf(2, "\n");
    return(0);
}
