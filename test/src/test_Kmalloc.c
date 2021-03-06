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


#include "konoha2/konoha2.h"
#include "konoha2/gc.h"

int main(int argc, const char *argv[])
{
    klib2_t *lib;
    static kplatform_t plat = {
    	"test", 4096,
    };
    konoha_t konoha = konoha_open((const kplatform_t*)&plat);
    lib = konoha->lib2;
    int i;
    void *malloced[100];
    for (i = 0; i < 100; ++i) {
        malloced[i] = lib->Kmalloc(0, i);
    }
    for (i = 0; i < 100; ++i) {
        lib->Kfree(0, malloced[i], i);
    }
    MODGC_check_malloced_size();
    for (i = 0; i < 100; ++i) {
        malloced[i] = lib->Kzmalloc(0, i);
        int j;
        char *p = malloced[i];
        for (j = 0; j < i; ++j) {
            assert(p[0] == 0);
        }
    }
    for (i = 0; i < 100; ++i) {
        lib->Kfree(0, malloced[i], i);
    }
    konoha_close(konoha);
    MODGC_check_malloced_size();
    return 0;
}
