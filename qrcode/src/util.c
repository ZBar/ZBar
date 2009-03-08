#include "util.h"

unsigned qr_isqrt(unsigned _val){
  unsigned g;
  unsigned b;
  int      bshift;
  /*Uses the second method from
     http://www.azillionmonkeys.com/qed/sqroot.html
    The main idea is to search for the largest binary digit b such that
     (g+b)*(g+b) <= _val, and add it to the solution g.*/
  g=0;
  b=0x8000;
  for(bshift=16;bshift-->0;){
    unsigned t;
    t=(g<<1)+b<<bshift;
    if(t<=_val){
      g+=b;
      _val-=t;
    }
    b>>=1;
  }
  return g;
}

/*TODO: ihypot
  a>b => g=a+r && add e iff (a+r+e)*(a+r+e) <= a*a+b*b
                            2*(a-b+r+e)*(r+e) <= (b-r-e)*(b-r-e)
                            2*(a/(b-r-e)-1)*(r+e) <= b-r-e
                            2*a*(r+e)/(b-r-e) <= b+r+e
                            2*(a-b+r+e)/(b-r-e) <= b/(r+e)
*/

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
