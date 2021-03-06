/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifndef MODICONV_H_
#define MODICONV_H_

/* ------------------------------------------------------------------------ */
/* [class defs] */

#define ctxiconv         ((ctxiconv_t*)_ctx->mod[MOD_iconv])
#define kmodiconv        ((kmodiconv_t*)_ctx->modshare[MOD_iconv])
#define IS_defineBytes() (_ctx->modshare[MOD_iconv] != NULL)
#define CT_Bytes         kmodiconv->cBytes
#define TY_Bytes         kmodiconv->cBytes->cid

#define IS_Bytes(O)      ((O)->h.ct == CT_Bytes)

#include <string.h>
#include <langinfo.h>
#include <locale.h>
#ifdef K_USING_ICONV
#include <iconv.h>
typedef iconv_t kiconv_t;
#else
typedef long    kiconv_t;
#endif

typedef kiconv_t (*ficonv_open)(const char *, const char *);
typedef size_t (*ficonv)(kiconv_t, const char **, size_t *, char **, size_t *);
typedef int    (*ficonv_close)(kiconv_t);

typedef struct {
    kmodshare_t h;
    kclass_t     *cBytes;
    kbool_t      (*encode)(const char* from, const char* to, const char* text, size_t len, kwb_t* wb);
    const char*  fmt;
    const char*  locale;
    kiconv_t     (*ficonv_open)(const char *, const char*);
    size_t       (*ficonv)(kiconv_t, const char **, size_t *, char**, size_t *);
    int          (*ficonv_close)(kiconv_t);
} kmodiconv_t;

typedef struct {
    kmodlocal_t h;
} ctxiconv_t;
#endif /* MODICONV_H_ */
