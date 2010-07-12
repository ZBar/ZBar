/*------------------------------------------------------------------------
 *  Copyright 2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#include <zbar.h>

#ifdef DEBUG_DATABAR
# define DEBUG_LEVEL (DEBUG_DATABAR)
#endif
#include "debug.h"
#include "decoder.h"

static const signed char finder_hash[0x20] = {
    0x16, 0x1f, 0x02, 0x00, 0x03, 0x00, 0x06, 0x0b,
    0x1f, 0x0e, 0x17, 0x0c, 0x0b, 0x14, 0x11, 0x0c,
    0x1f, 0x03, 0x13, 0x08, 0x00, 0x0a,   -1, 0x16,
    0x0c, 0x09,   -1, 0x1a, 0x1f, 0x1c, 0x00,   -1,
};

/* DataBar character encoding groups */
struct group_s {
    unsigned short sum;
    unsigned char wmax;
    unsigned char todd;
    unsigned char teven;
} groups[] = {
    /* (17,4) DataBar Expanded character groups */
    {    0, 7,  87,   4 },
    {  348, 5,  52,  20 },
    { 1388, 4,  30,  52 },
    { 2948, 3,  10, 104 },
    { 3988, 1,   1, 204 },

    /* (16,4) DataBar outer character groups */
    {    0, 8, 161,   1 },
    {  161, 6,  80,  10 },
    {  961, 4,  31,  34 },
    { 2015, 3,  10,  70 },
    { 2715, 1,   1, 126 },

    /* (15,4) DataBar inner character groups */
    { 1516, 8,  81,   1 },
    { 1036, 6,  48,  10 },
    {  336, 4,  20,  35 },
    {    0, 2,   4,  84 },
};

/* convert from heterogeneous base {1597,2841}
 * to base 10 character representation
 */
static inline void
databar_postprocess (zbar_decoder_t *dcode,
                     unsigned d[4])
{
    databar_decoder_t *db = &dcode->databar;
    int i;
    unsigned c, chk = 0;
    unsigned char *buf = dcode->buf + 15;
    *--buf = '\0';
    *--buf = '\0';

    dprintf(2, "\n    d={%d,%d,%d,%d}", d[0], d[1], d[2], d[3]);
    unsigned long r = d[0] * 1597 + d[1];
    d[1] = r / 10000;
    r %= 10000;
    r = r * 2841 + d[2];
    d[2] = r / 10000;
    r %= 10000;
    r = r * 1597 + d[3];
    d[3] = r / 10000;
    dprintf(2, " r=%ld", r);

    for(i = 4; --i >= 0; ) {
        c = r % 10;
        chk += c;
        if(i & 1)
            chk += c << 1;
        *--buf = c + '0';
        if(i)
            r /= 10;
    }

    dprintf(2, "\n    d={%d,%d,%d}", d[1], d[2], d[3]);
    r = d[1] * 2841 + d[2];
    d[2] = r / 10000;
    r %= 10000;
    r = r * 1597 + d[3];
    d[3] = r / 10000;
    dprintf(2, " r=%ld", r);

    for(i = 4; --i >= 0; ) {
        c = r % 10;
        chk += c;
        if(i & 1)
            chk += c << 1;
        *--buf = c + '0';
        if(i)
            r /= 10;
    }

    r = d[2] * 1597 + d[3];
    dprintf(2, "\n    d={%d,%d} r=%ld", d[2], d[3], r);

    for(i = 5; --i >= 0; ) {
        c = r % 10;
        chk += c;
        if(!(i & 1))
            chk += c << 1;
        *--buf = c + '0';
        if(i)
            r /= 10;
    }

    /* NB linkage flag not supported */
    if(TEST_CFG(db->config, ZBAR_CFG_EMIT_CHECK)) {
        chk %= 10;
        if(chk)
            chk = 10 - chk;
        dcode->buf[13] = chk + '0';
        dcode->buflen = 14;
    }
    else
        dcode->buflen = 13;

    dprintf(2, "\n    %s", _zbar_decoder_buf_dump(dcode->buf, 16));
}

static inline int
check_width (unsigned wf,
             unsigned wd,
             unsigned n)
{
    unsigned dwf = wf * 3;
    wd *= 14;
    wf *= n;
    return(wf - dwf <= wd && wd <= wf + dwf);
}

static inline void
merge_segment (databar_decoder_t *db,
               databar_segment_t *seg)
{
    unsigned csegs = db->csegs;
    int i;
    for(i = 0; i < csegs; i++) {
        databar_segment_t *s = db->segs + i;
        if(s != seg && s->finder == seg->finder && s->ext == seg->ext &&
           s->color == seg->color && s->side == seg->side &&
           s->data == seg->data && s->check == seg->check &&
           check_width(seg->width, s->width, 14)) {
            /* merge with existing segment */
            unsigned cnt = s->count;
            if(cnt < 0x7f)
                cnt++;
            seg->count = cnt;
            seg->partial &= s->partial;
            seg->width = (3 * seg->width + s->width + 2) / 4;
            s->finder = -1;
            dprintf(2, " dup@%d", i);
        }
        else if(s->finder >= 0) {
            unsigned age = (db->epoch - s->epoch) & 0xff;
            if(age >= 248 || (age >= 128 && s->count < 2))
                s->finder = -1;
        }
    }
}

static inline zbar_symbol_type_t
match_segment (zbar_decoder_t *dcode,
               databar_segment_t *seg)
{
    databar_decoder_t *db = &dcode->databar;
    unsigned csegs = db->csegs, maxage = 0xfff;
    int i0, i1, i2, maxcnt = 0;
    databar_segment_t *smax[3] = { NULL, };

    if(seg->partial && seg->count < 4)
        return(ZBAR_PARTIAL);

    for(i0 = 0; i0 < csegs; i0++) {
        databar_segment_t *s0 = db->segs + i0;
        if(s0 == seg || s0->finder != seg->finder || s0->ext ||
           s0->color != seg->color || s0->side == seg->side ||
           (s0->partial && s0->count < 4) ||
           !check_width(seg->width, s0->width, 14))
            continue;

        for(i1 = 0; i1 < csegs; i1++) {
            databar_segment_t *s1 = db->segs + i1;
            int chkf, chks, chk;
            unsigned age1;
            if(i1 == i0 || s1->finder < 0 || s1->ext ||
               s1->color == seg->color ||
               (s1->partial && s1->count < 4) ||
               !check_width(seg->width, s1->width, 14))
                continue;
            dprintf(2, "\n\t[%d,%d] f=%d(0%xx)/%d(%x%x%x)",
                    i0, i1, seg->finder, seg->color,
                    s1->finder, s1->ext, s1->color, s1->side);

            if(seg->color)
                chkf = seg->finder + s1->finder * 9;
            else
                chkf = s1->finder + seg->finder * 9;
            if(chkf > 72)
                chkf--;
            if(chkf > 8)
                chkf--;

            chks = (seg->check + s0->check + s1->check) % 79;

            if(chkf >= chks)
                chk = chkf - chks;
            else
                chk = 79 + chkf - chks;

            dprintf(2, " chk=(%d,%d) => %d", chkf, chks, chk);
            age1 = ((db->epoch - s0->epoch) & 0xff +
                    (db->epoch - s1->epoch) & 0xff);

            for(i2 = i1 + 1; i2 < csegs; i2++) {
                databar_segment_t *s2 = db->segs + i2;
                unsigned cnt, age2, age;
                if(i2 == i0 || s2->finder != s1->finder || s2->ext ||
                   s2->color != s1->color || s2->side == s1->side ||
                   s2->check != chk ||
                   (s2->partial && s2->count < 4) ||
                   !check_width(seg->width, s2->width, 14))
                    continue;
                age2 = (db->epoch - s2->epoch) & 0xff;
                age = age1 + age2;
                cnt = s0->count + s1->count + s2->count;
                dprintf(2, " [%d] MATCH cnt=%d age=%d", i2, cnt, age);
                if(maxcnt < cnt ||
                   (maxcnt == cnt && maxage > age)) {
                    maxcnt = cnt;
                    maxage = age;
                    smax[0] = s0;
                    smax[1] = s1;
                    smax[2] = s2;
                }
            }
        }
    }

    if(!smax[0])
        return(ZBAR_PARTIAL);

    unsigned d[4];
    d[(seg->color << 1) | seg->side] = seg->data;
    for(i0 = 0; i0 < 3; i0++) {
        d[(smax[i0]->color << 1) | smax[i0]->side] = smax[i0]->data;
        if(!--(smax[i0]->count))
            smax[i0]->finder = -1;
    }
    seg->finder = -1;

    if(acquire_lock(dcode, ZBAR_DATABAR))
        return(ZBAR_PARTIAL);

    databar_postprocess(dcode, d);
    dcode->direction = 1 - 2 * (seg->side ^ seg->color ^ 1);
    return(ZBAR_DATABAR);
}

static inline zbar_symbol_type_t
match_segment_ext (zbar_decoder_t *dcode,
                   databar_segment_t *seg)
{
    /* FIXME TBD */
    seg->finder = -1;
    return(ZBAR_NONE);
}

static inline unsigned
calc_check (unsigned sig0,
            unsigned sig1,
            unsigned side,
            unsigned mod)
{
    unsigned chk = 0;
    int i;
    for(i = 4; --i >= 0; ) {
        chk = (chk * 3 + (sig1 & 0xf) + 1) * 3 + (sig0 & 0xf) + 1;
        sig1 >>= 4;
        sig0 >>= 4;
        if(!(i & 1))
            chk %= mod;
    }
    dprintf(2, " chk=%d", chk);

    if(side)
        chk = (chk * (6561 % mod)) % mod;
    return(chk);
}

static inline int
calc_value4 (unsigned sig,
             unsigned n,
             unsigned wmax,
             unsigned nonarrow)
{
    unsigned v = 0;
    n--;

    unsigned w0 = (sig >> 12) & 0xf;
    if(w0 > 1) {
        if(w0 > wmax)
            return(-1);
        unsigned n0 = n - w0;
        unsigned sk20 = (n - 1) * n * (2 * n - 1);
        unsigned sk21 = n0 * (n0 + 1) * (2 * n0 + 1);
        v = sk20 - sk21 - 3 * (w0 - 1) * (2 * n - w0);

        if(!nonarrow && w0 > 2 && n > 4) {
            unsigned k = (n - 2) * (n - 1) * (2 * n - 3) - sk21;
            k -= 3 * (w0 - 2) * (14 * n - 7 * w0 - 31);
            v -= k;
        }

        if(n - 2 > wmax) {
            unsigned wm20 = 2 * wmax * (wmax + 1);
            unsigned wm21 = (2 * wmax + 1);
            unsigned k = sk20;
            if(n0 > wmax) {
                k -= sk21;
                k += 3 * (w0 - 1) * (wm20 - wm21 * (2 * n - w0));
            }
            else {
                k -= (wmax + 1) * (wmax + 2) * (2 * wmax + 3);
                k += 3 * (n - wmax - 2) * (wm20 - wm21 * (n + wmax + 1));
            }
            k *= 3;
            v -= k;
        }
        v /= 12;
    }
    else
        nonarrow = 1;
    n -= w0;

    unsigned w1 = (sig >> 8) & 0xf;
    if(w1 > 1) {
        if(w1 > wmax)
            return(-1);
        v += (2 * n - w1) * (w1 - 1) / 2;
        if(!nonarrow && w1 > 2 && n > 3)
            v -= (2 * n - w1 - 5) * (w1 - 2) / 2;
        if(n - 1 > wmax) {
            if(n - w1 > wmax)
                v -= (w1 - 1) * (2 * n - w1 - 2 * wmax);
            else
                v -= (n - wmax) * (n - wmax - 1);
        }
    }
    else
        nonarrow = 1;
    n -= w1;

    unsigned w2 = (sig >> 4) & 0xf;
    if(w2 > 1) {
        if(w2 > wmax)
            return(-1);
        v += w2 - 1;
        if(!nonarrow && w2 > 2 && n > 2)
            v -= n - 2;
        if(n > wmax)
            v -= n - wmax;
    }
    else
        nonarrow = 1;

    unsigned w3 = sig & 0xf;
    if(w3 == 1)
        nonarrow = 1;
    else if(w3 > wmax)
        return(-1);

    if(!nonarrow)
        return(-1);

    return(v);
}

static inline zbar_symbol_type_t
decode_char (zbar_decoder_t *dcode,
             databar_segment_t *seg,
             int off,
             int dir)
{
    databar_decoder_t *db = &dcode->databar;
    unsigned s = calc_s(dcode, (dir > 0) ? off : off - 6, 8);
    int n, i, emin[2] = { 0, }, sum = 0;
    unsigned sig0 = 0, sig1 = 0;

    if(seg->ext)
        n = 17;
    else if(seg->side)
        n = 15;
    else
        n = 16;
    emin[1] = -n;

    dprintf(2, "        char: d=%d off=%d n=%d s=%d w=%d sig=",
            dir, off, n, s, seg->width);
    if(s < 13 || !check_width(seg->width, s, n))
        return(ZBAR_NONE);

    for(i = 4; --i >= 0; ) {
        int e = decode_e(pair_width(dcode, off), s, n);
        if(e < 0)
            return(ZBAR_NONE);
        dprintf(2, "%d", e);
        sum = e - sum;
        off += dir;
        sig1 <<= 4;
        if(emin[1] < -sum)
            emin[1] = -sum;
        sig1 += sum;
        if(!i)
            break;

        e = decode_e(pair_width(dcode, off), s, n);
        if(e < 0)
            return(ZBAR_NONE);
        dprintf(2, "%d", e);
        sum = e - sum;
        off += dir;
        sig0 <<= 4;
        if(emin[0] > sum)
            emin[0] = sum;
        sig0 += sum;
    }

    int diff = emin[~n & 1];
    diff = diff + (diff << 4);
    diff = diff + (diff << 8);

    sig0 -= diff;
    sig1 += diff;

    dprintf(2, " emin=%d,%d el=%04x/%04x", emin[0], emin[1], sig0, sig1);

    unsigned sum0 = sig0 + (sig0 >> 8);
    unsigned sum1 = sig1 + (sig1 >> 8);
    sum0 += sum0 >> 4;
    sum1 += sum1 >> 4;
    sum0 &= 0xf;
    sum1 &= 0xf;

    dprintf(2, " sum=%d/%d", sum0, sum1);

    if(sum0 + sum1 + 8 != n) {
        dprintf(2, " [SUM]");
        return(ZBAR_NONE);
    }

    if(((sum0 ^ (n >> 1)) | (sum1 ^ (n >> 1) ^ n)) & 1) {
        dprintf(2, " [ODD]");
        return(ZBAR_NONE);
    }

    i = ((n & 0x3) ^ 1) * 5 + (sum1 >> 1);
    zassert(i < sizeof(groups) / sizeof(*groups), -1,
            "n=%d sum=%d/%d sig=%04x/%04x g=%d",
            n, sum0, sum1, sig0, sig1, i);
    struct group_s *g = groups + i;
    dprintf(2, "\n            g=%d(%d,%d,%d/%d)",
            i, g->sum, g->wmax, g->todd, g->teven);

    int vodd = calc_value4(sig0 + 0x1111, sum0 + 4, g->wmax, ~n & 1);
    dprintf(2, " v=%d", vodd);
    if(vodd < 0 || vodd > g->todd)
        return(ZBAR_NONE);

    int veven = calc_value4(sig1 + 0x1111, sum1 + 4, 9 - g->wmax, n & 1);
    dprintf(2, "/%d", veven);
    if(veven < 0 || veven > g->teven)
        return(ZBAR_NONE);

    int v = g->sum;
    if(n & 2)
        v += vodd + veven * g->todd;
    else
        v += veven + vodd * g->teven;

    dprintf(2, " f=%d(%x%x%x)", seg->finder, seg->ext, seg->color, seg->side);

    unsigned chk = 0;
    if(seg->ext) {
        /* FIXME skip A1 */
        chk = calc_check(sig0, sig1, seg->side, 211);
        if(seg->color)
            chk = (chk * 189) % 211;
    }
    else {
        chk = calc_check(sig0, sig1, seg->side, 79);
        if(seg->color)
            chk = (chk * 16) % 79;
    }
    dprintf(2, " => %d val=%d", chk, v);

    seg->epoch = db->epoch++;
    seg->check = chk;
    seg->data = v;

    merge_segment(db, seg);

    return(ZBAR_PARTIAL);
}

static inline int
alloc_segment (databar_decoder_t *db)
{
    unsigned maxage = 0, csegs = db->csegs;
    int i, old = -1;
    for(i = 0; i < csegs; i++) {
        databar_segment_t *seg = db->segs + i;
        unsigned age;
        if(seg->finder < 0) {
            dprintf(2, " free@%d", i);
            return(i);
        }
        age = (db->epoch - seg->epoch) & 0xff;
        if(age >= 128 && seg->count < 2) {
            seg->finder = -1;
            dprintf(2, " stale@%d (%d - %d = %d)",
                    i, db->epoch, seg->epoch, age);
            return(i);
        }

        /* score based on both age and count */
        if(age > seg->count)
            age = age - seg->count + 1;
        else
            age = 1;

        if(maxage < age) {
            maxage = age;
            old = i;
            dprintf(2, " old@%d(%u)", i, age);
        }
    }

    if(csegs < DATABAR_MAX_SEGMENTS) {
        dprintf(2, " new@%d", i);
        i = csegs;
        csegs *= 2;
        if(csegs > DATABAR_MAX_SEGMENTS)
            csegs = DATABAR_MAX_SEGMENTS;
        if(csegs != db->csegs) {
            databar_segment_t *seg;
            db->segs = realloc(db->segs, csegs * sizeof(*db->segs));
            db->csegs = csegs;
            seg = db->segs + csegs;
            while(--seg, --csegs >= i) {
                seg->finder = -1;
                seg->ext = 0;
                seg->color = 0;
                seg->side = 0;
                seg->partial = 0;
                seg->count = 0;
                seg->epoch = 0;
                seg->check = 0;
            }
            return(i);
        }
    }
    zassert(old >= 0, -1, "\n");

    db->segs[old].finder = -1;
    return(old);
}

static inline zbar_symbol_type_t
decode_finder (zbar_decoder_t *dcode)
{
    databar_decoder_t *db = &dcode->databar;
    databar_segment_t *seg;
    unsigned e0 = pair_width(dcode, 1);
    unsigned e2 = pair_width(dcode, 3);
    unsigned e1, e3, s, finder, dir;
    int sig, iseg;
    dprintf(2, "      databar: e0=%d e2=%d", e0, e2);
    if(e0 < e2) {
        unsigned e = e2 * 4;
        if(e < 15 * e0 || e > 34 * e0)
            return(ZBAR_NONE);
        dir = 0;
        e3 = pair_width(dcode, 4);
    }
    else {
        unsigned e = e0 * 4;
        if(e < 15 * e2 || e > 34 * e2)
            return(ZBAR_NONE);
        dir = 1;
        e2 = e0;
        e3 = pair_width(dcode, 0);
    }
    e1 = pair_width(dcode, 2);

    s = e1 + e3;
    dprintf(2, " e1=%d e3=%d dir=%d s=%d", e1, e3, dir, s);
    if(s < 12)
        return(ZBAR_NONE);

    sig = ((decode_e(e3, s, 14) << 8) | (decode_e(e2, s, 14) << 4) |
           decode_e(e1, s, 14));
    dprintf(2, " sig=%04x", sig & 0xfff);
    if(sig < 0 ||
       ((sig >> 4) & 0xf) < 8 ||
       ((sig >> 4) & 0xf) > 10 ||
       (sig & 0xf) >= 10 ||
       ((sig >> 8) & 0xf) >= 10)
        return(ZBAR_NONE);

    finder = (finder_hash[(sig - (sig >> 5)) & 0x1f] +
              finder_hash[(sig >> 1) & 0x1f]) & 0x1f;
    dprintf(2, " finder=%d", finder);
    if(finder == 0x1f)
        return(ZBAR_NONE);

    iseg = alloc_segment(db);
    if(iseg < 0)
        return(ZBAR_NONE);
    dprintf(2, " i=%d\n", iseg);

    seg = db->segs + iseg;
    seg->finder = (finder >= 9) ? finder - 9 : finder;
    seg->ext = (finder >= 9);
    seg->color = get_color(dcode) ^ dir ^ 1;
    seg->side = dir;
    seg->partial = 0;
    seg->count = 1;
    seg->width = s;

    int rc = decode_char(dcode, seg, 12 - dir, -1);
    if(!rc)
        seg->partial = 1;

    int i = (dcode->idx + 8 + dir) & 0xf;
    zassert(db->chars[i] == -1, ZBAR_NONE, "\n");
    db->chars[i] = iseg;
    return(rc);
}

zbar_symbol_type_t
_zbar_decode_databar (zbar_decoder_t *dcode)
{
    databar_decoder_t *db = &dcode->databar;
    databar_segment_t *seg, *pair;
    zbar_symbol_type_t sym;
    int iseg, i = dcode->idx & 0xf;
    sym = decode_finder(dcode);
    dprintf(2, "\n");

    iseg = db->chars[i];
    if(iseg < 0)
        return(sym);

    db->chars[i] = -1;
    seg = db->segs + iseg;
    dprintf(2, "        databar: i=%d part=%d", iseg, seg->partial);

    if(seg->partial) {
        pair = NULL;
        seg->side = !seg->side;
    }
    else {
        int jseg = alloc_segment(db);
        pair = db->segs + iseg;
        seg = db->segs + jseg;
        seg->finder = pair->finder;
        seg->ext = pair->ext;
        seg->color = pair->color;
        seg->side = !pair->side;
        seg->partial = 0;
        seg->count = 1;
        seg->width = pair->width;
    }
    dprintf(2, "\n");

    sym = decode_char(dcode, seg, 1, 1);
    if(sym) {
        if(seg->ext)
            sym = match_segment_ext(dcode, seg);
        else
            sym = match_segment(dcode, seg);
    }
    else {
        seg->finder = -1;
        if(pair)
            pair->partial = 1;
    }

    dprintf(2, "\n");
    return(sym);
}
