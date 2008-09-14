#if !defined(_qrcode_util_H)
# define _qrcode_util_H (1)

#define QR_MAXI(_a,_b)      ((_a)-((_a)-(_b)&-((_b)>(_a))))
#define QR_MINI(_a,_b)      ((_a)+((_b)-(_a)&-((_b)<(_a))))
#define QR_SIGNI(_x)        (((_x)>0)-((_x)<0))
#define QR_SIGNMASK(_x)     (-((_x)<0))
/*Divides a signed integer by a positive value with exact rounding.
  Both values must be able to be doubled without overflow.*/
#define QR_DIVROUND(_x,_y) \
 ((((_x)<<1)+(_y)-((_y)<<1&QR_SIGNMASK(_x)))/((_y)<<1))
/*Divides a signed integer by a positive value with rounding (exact only when
   the divisor is even), but less chance of potential overflow.*/
#define QR_HDIVROUND(_x,_y) (((_x)+((_y)>>1)-((_y)&QR_SIGNMASK(_x)))/(_y))
/*Unlike copysign(), simply inverts the sign of _a if _b is negative.
  Call with abs(_a) to emulate the standard copysign() behavior.*/
#define QR_COPYSIGNI(_a,_b) ((_a)&~QR_SIGNMASK(_b)|-(_a)&QR_SIGNMASK(_b))
#define QR_CLAMPI(_a,_b,_c) (QR_MAXI(_a,QR_MINI(_b,_c)))
#define QR_CLAMP255(_x)     ((unsigned char)((((_x)<0)-1)&((_x)|-((_x)>255))))
/*Swaps two integers _a and _b if _a>_b.*/
#define QR_SORT2I(_a,_b) \
  do{ \
    int t__; \
    t__=QR_MINI(_a,_b)^(_a); \
    (_a)^=t__; \
    (_b)^=t__; \
  } \
  while(0)
#define QR_ILOG0(_v) (!!((_v)&0x2))
#define QR_ILOG1(_v) (((_v)&0xC)?2+QR_ILOG0((_v)>>2):QR_ILOG0(_v))
#define QR_ILOG2(_v) (((_v)&0xF0)?4+QR_ILOG1((_v)>>4):QR_ILOG1(_v))
#define QR_ILOG3(_v) (((_v)&0xFF00)?8+QR_ILOG2((_v)>>8):QR_ILOG2(_v))
#define QR_ILOG4(_v) (((_v)&0xFFFF0000)?16+QR_ILOG3((_v)>>16):QR_ILOG3(_v))
/*Computes the integer logarithm of an (unsigned, 32-bit) constant.*/
#define QR_ILOG(_v) ((int)QR_ILOG4((unsigned)(_v)))

/*Multiplies 32-bit numbers _a and _b, adds _r, and takes bits [_s,_s+31] of
   the result.*/
#define QR_FIXMUL(_a,_b,_r,_s) ((int)((_a)*(long long)(_b)+(_r)>>(_s)))

unsigned qr_isqrt(unsigned _val);
int qr_ilog(unsigned _val);

#endif
