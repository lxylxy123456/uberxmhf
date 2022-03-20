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
#include <stdarg.h>
/**
  @file hash_memory_multi.c
  Hash (multiple buffers) memory helper, Tom St Denis
*/

/**
  Hash multiple (non-adjacent) blocks of memory at once.
  @param hash   The index of the hash you wish to use
  @param out    [out] Where to store the digest
  @param outlen [in/out] Max size and resulting size of the digest
  @param in     The data you wish to hash
  @param inlen  The length of the data to hash (octets)
  @param ...    tuples of (data,len) pairs to hash, terminated with a (NULL,x) (x=don't care)
  @return CRYPT_OK if successful
*/
int hash_memory_multi(int hash, unsigned char *out, unsigned long *outlen,
                      const unsigned char *in, unsigned long inlen, ...)
{
    hash_state          *md;
    int                  err;
    va_list              args;
    const unsigned char *curptr;
    unsigned long        curlen;

    printf("%s: %d\n", __func__, __LINE__);
    LTC_ARGCHK(in     != NULL);
    LTC_ARGCHK(out    != NULL);
    LTC_ARGCHK(outlen != NULL);

    printf("%s: %d\n", __func__, __LINE__);
    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    printf("%s: %d\n", __func__, __LINE__);
    if (*outlen < hash_descriptor[hash].hashsize) {
       *outlen = hash_descriptor[hash].hashsize;
       return CRYPT_BUFFER_OVERFLOW;
    }

    printf("%s: %d\n", __func__, __LINE__);
    md = XMALLOC(sizeof(hash_state));
    if (md == NULL) {
       return CRYPT_MEM;
    }

    printf("%s: %d\n", __func__, __LINE__);
    if ((err = hash_descriptor[hash].init(md)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    printf("%s: %d\n", __func__, __LINE__);
    va_start(args, inlen);
    curptr = in;
    curlen = inlen;
    printf("%s: %d\n", __func__, __LINE__);
    for (;;) {
       /* process buf */
       printf("%s: %d %p\n", __func__, __LINE__, hash_descriptor[hash].process);
       if ((err = hash_descriptor[hash].process(md, curptr, curlen)) != CRYPT_OK) {
          printf("%s: %d\n", __func__, __LINE__);
          goto LBL_ERR;
       }
       printf("%s: %d\n", __func__, __LINE__);
       /* step to next */
       curptr = va_arg(args, const unsigned char*);
       if (curptr == NULL) {
          break;
       }
       printf("%s: %d\n", __func__, __LINE__);
       curlen = va_arg(args, unsigned long);
    }
    printf("%s: %d\n", __func__, __LINE__);
    err = hash_descriptor[hash].done(md, out);
    *outlen = hash_descriptor[hash].hashsize;
    printf("%s: %d\n", __func__, __LINE__);
LBL_ERR:
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    printf("%s: %d\n", __func__, __LINE__);
    XFREE(md);
    va_end(args);
    printf("%s: %d\n", __func__, __LINE__);
    return err;
}

/* $Source: /cvs/libtom/libtomcrypt/src/hashes/helper/hash_memory_multi.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2006/12/28 01:27:23 $ */
