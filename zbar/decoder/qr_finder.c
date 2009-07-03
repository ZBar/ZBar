#include <config.h>
#include <assert.h>

#include <zbar.h>
#include "decoder.h"

#ifdef DEBUG_QR
# define DEBUG_LEVEL (DEBUG_QR)
#endif
#include "debug.h"


zbar_symbol_type_t _zbar_find_qr (zbar_decoder_t *dcode)
{
    qr_finder_t *qrf = &dcode->qrf;

    /* update latest finder pattern width */
    qrf->s5 -= get_width(dcode, 6);
    qrf->s5 += get_width(dcode, 1);
    unsigned s = qrf->s5;

    if(get_color(dcode) != ZBAR_SPACE || s < 7)
        return(0);

    dprintf(2, "    qrf: s=%d", s);

    int ei = decode_e(pair_width(dcode, 1), s, 7);
    dprintf(2, " %d", ei);
    if(ei)
        return(0);

    ei = decode_e(pair_width(dcode, 2), s, 7);
    dprintf(2, "%d", ei);
    if(ei != 2)
        return(0);

    ei = decode_e(pair_width(dcode, 3), s, 7);
    dprintf(2, "%d", ei);
    if(ei != 2)
        return(0);

    ei = decode_e(pair_width(dcode, 4), s, 7);
    dprintf(2, "%d", ei);
    if(ei)
        return(0);

    int tqz = get_width(dcode, 5);
    if(tqz) {
        tqz = (tqz + 1) * 7 / s;
        dprintf(2, "%d", tqz);
        if(tqz < 1)
            return(0);
    }
    else
        tqz = 4;

    int lqz = get_width(dcode, 0);
    if(lqz) {
        lqz = (lqz + 1) * 7 / s;
        dprintf(2, "%d", lqz);
        if(lqz < 0)
            return(0);
    }
    else
        lqz = 4;

    /* one border must be full QZ */
    if(tqz < 3 && lqz < 3) {
        dprintf(2, " [invalid qz]\n");
        return(0);
    }

    /* valid QR finder symbol */
    dprintf(2, " [valid]\n");

    /* FIXME TBD: mark positions needed by decoder */
    assert(0);

    return(ZBAR_QR);
}
