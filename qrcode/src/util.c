#include "util.h"

unsigned qr_isqrt(unsigned _val){
  unsigned temp;
  unsigned g;
  unsigned b;
  int      bshft;
  /*Uses the second approximation method from
     http://www.azillionmonkeys.com/qed/sqroot.html*/
  g=0;
  b=0x8000;
  bshft=15;
  do{
    temp=(g<<1)+b<<bshft--;
    if(_val>=temp){
      g+=b;
      _val-=temp;
    }
    b>>=1;
  }
  while(b>0);
  return g;
}

#if defined(__GNUC__)
# include <features.h>
# if __GNUC_PREREQ(3,4)
#  include <limits.h>
#  if INT_MAX>=2147483647
#   define QR_CLZ0 sizeof(unsigned)*CHAR_BIT
#   define QR_CLZ(_x) (__builtin_clz(_x))
#  elif LONG_MAX>=2147483647L
#   define QR_CLZ0 sizeof(unsigned long)*CHAR_BIT
#   define QR_CLZ(_x) (__builtin_clzl(_x))
#  endif
# endif
#endif

int qr_ilog(unsigned _v){
#if defined(QR_CLZ)
/*Note that __builtin_clz is not defined when _x==0, according to the gcc
   documentation (and that of the x86 BSR instruction that implements it), so
   we have to special-case it.*/
  return QR_CLZ0-QR_CLZ(_v)&-!!_v;
#else
  int ret;
  int m;
  m=!!(_v&0xFFFF0000)<<4;
  _v>>=m;
  ret=m;
  m=!!(_v&0xFF00)<<3;
  _v>>=m;
  ret|=m;
  m=!!(_v&0xF0)<<2;
  _v>>=m;
  ret|=m;
  m=!!(_v&0xC)<<1;
  _v>>=m;
  ret|=m;
  ret|=!!(_v&0x2);
  return ret;
#endif
}

#if defined(QR_TEST_SQRT)
#include <math.h>
#include <stdio.h>

/*Exhaustively test the integer square root function.*/
int main(void){
  unsigned u;
  u=0;
  do{
    unsigned r;
    unsigned s;
    r=qr_isqrt(u);
    s=(int)sqrt(u);
    if(r!=s)printf("%u: %u!=%u\n",u,r,s);
    u++;
  }
  while(u);
  return 0;
}
#endif
