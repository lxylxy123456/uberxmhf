# 0 "b.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "b.c"



typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct {
 uint16_t a;
} a_t;

typedef struct {
 uint32_t t;
 uint32_t b;
} b_t;

typedef struct {
 uint32_t t;
 a_t c;
} c_t;

void func(uint8_t * dst, const uint8_t * src)
{
 if ((*(uint32_t *) (dst)) == 1) {
  b_t *sb = (b_t *) dst;
  uint8_t *pb = (uint8_t *) & (sb->b);
  for (uint32_t i = 0; i < 4; i++) {
   pb[i] = src[0];
  }
 } else {
  c_t *sc = (c_t *) dst;
  uint8_t *pc = (uint8_t *) & (sc->c.a);
  for (uint32_t i = 0; i < 2; i++) {
   pc[i] = 0;
  }
 }
}
