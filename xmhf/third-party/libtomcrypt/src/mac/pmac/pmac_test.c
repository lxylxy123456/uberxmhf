/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
#include "tomcrypt.h"

/**
   @file pmac_test.c
   PMAC implementation, self-test, by Tom St Denis
*/


#ifdef LTC_PMAC

/**
   Test the LTC_OMAC implementation
   @return CRYPT_OK if successful, CRYPT_NOP if testing has been disabled
*/
int pmac_test(void)
{
#if !defined(LTC_TEST)
    return CRYPT_NOP;
#else
    static const struct {
        int msglen;
        unsigned char key[16], msg[34], tag[16];
    } tests[] = {

   /* PMAC-AES-128-0B */
{
   0,
   /* key */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* msg */
   { 0x00 },
   /* tag */
   { 0x43, 0x99, 0x57, 0x2c, 0xd6, 0xea, 0x53, 0x41,
     0xb8, 0xd3, 0x58, 0x76, 0xa7, 0x09, 0x8a, 0xf7 }
},

   /* PMAC-AES-128-3B */
{
   3,
   /* key */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* msg */
   { 0x00, 0x01, 0x02 },
   /* tag */
   { 0x25, 0x6b, 0xa5, 0x19, 0x3c, 0x1b, 0x99, 0x1b,
     0x4d, 0xf0, 0xc5, 0x1f, 0x38, 0x8a, 0x9e, 0x27 }
},

   /* PMAC-AES-128-16B */
{
   16,
   /* key */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* msg */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* tag */
   { 0xeb, 0xbd, 0x82, 0x2f, 0xa4, 0x58, 0xda, 0xf6,
     0xdf, 0xda, 0xd7, 0xc2, 0x7d, 0xa7, 0x63, 0x38 }
},

   /* PMAC-AES-128-20B */
{
   20,
   /* key */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* msg */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
     0x10, 0x11, 0x12, 0x13 },
   /* tag */
   { 0x04, 0x12, 0xca, 0x15, 0x0b, 0xbf, 0x79, 0x05,
     0x8d, 0x8c, 0x75, 0xa5, 0x8c, 0x99, 0x3f, 0x55 }
},

   /* PMAC-AES-128-32B */
{
   32,
   /* key */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* msg */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f },
   /* tag */
   { 0xe9, 0x7a, 0xc0, 0x4e, 0x9e, 0x5e, 0x33, 0x99,
     0xce, 0x53, 0x55, 0xcd, 0x74, 0x07, 0xbc, 0x75 }
},

   /* PMAC-AES-128-34B */
{
   34,
   /* key */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
   /* msg */
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
     0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
     0x20, 0x21 },
   /* tag */
   { 0x5c, 0xba, 0x7d, 0x5e, 0xb2, 0x4f, 0x7c, 0x86,
     0xcc, 0xc5, 0x46, 0x04, 0xe5, 0x3d, 0x55, 0x12 }
}

};
   int err, x, idx;
   unsigned long len;
   unsigned char outtag[MAXBLOCKSIZE];

    /* AES can be under rijndael or aes... try to find it */
    if ((idx = find_cipher("aes")) == -1) {
       if ((idx = find_cipher("rijndael")) == -1) {
          return CRYPT_NOP;
       }
    }

    for (x = 0; x < (int)(sizeof(tests)/sizeof(tests[0])); x++) {
        len = sizeof(outtag);
        if ((err = pmac_memory(idx, tests[x].key, 16, tests[x].msg, tests[x].msglen, outtag, &len)) != CRYPT_OK) {
           return err;
        }

        if (XMEMCMP(outtag, tests[x].tag, len)) {
#if 0
           unsigned long y;
           printf("\nTAG:\n");
           for (y = 0; y < len; ) {
               printf("0x%02x", outtag[y]);
               if (y < len-1) printf(", ");
               if (!(++y % 8)) printf("\n");
           }
#endif
           return CRYPT_FAIL_TESTVECTOR;
        }
     }
     return CRYPT_OK;
#endif /* LTC_TEST */
}

#endif /* PMAC_MODE */




/* $Source: /cvs/libtom/libtomcrypt/src/mac/pmac/pmac_test.c,v $ */
/* $Revision: 1.8 $ */
/* $Date: 2007/05/12 14:37:41 $ */
