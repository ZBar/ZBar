#if !defined(_qrcode_util_H)
# define _qrcode_util_H (1)

#define QR_FIX_POINT (21)
#define QR_FIX_MUL (_a,_b) \
  (((long long)(_a))*((long long)(_b))>>QR_FIX_POINT)
#define QR_FIX_ROTATEX (_x,_y,_c,_s) \
  (((long long)(_x))*((long long)(_c))- \
   ((long long)(_y))*((long long)(_s))>>QR_FIX_POINT)
#define QR_FIX_ROTATEY (_x,_y,_c,_s) \
  (((long long)(_x))*((long long)(_s))+ \
   ((long long)(_y))*((long long)(_c))>>QR_FIX_POINT)
#define QR_MAXI(_a,_b)      ((_a)-((_a)-(_b)&-((_b)>(_a))))
#define QR_MINI(_a,_b)      ((_a)+((_b)-(_a)&-((_b)<(_a))))
#define QR_SIGNI(_x)        (((_x)>0)-((_x)<0))
#define QR_SIGNMASK(_x)     (-((_x)<0))
#define QR_COPYSIGNI(_a,_b) ((_a)&~QR_SIGNMASK(_b)|-(_a)&QR_SIGNMASK(_b))
#define QR_CLAMPI(_a,_b,_c) (QR_MAXI(_a,QR_MINI(_b,_c)))
#define QR_CLAMP255(_x)     ((unsigned char)((((_x)<0)-1)&((_x)|-((_x)>255))))

unsigned qr_isqrt(unsigned _val);

#endif
