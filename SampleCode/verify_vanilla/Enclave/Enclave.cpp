/*
 * Copyright 2019 Samuel Weiser <samuel.weiser@iaik.tugraz.at>

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Enclave_t.h"
#include "Tests.h"
#include "sgx_trts.h"
#include <cstring>
#include <stdio.h>
#include <limits.h>

int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

void testEcall0() {
}

int testEcall1(char a, int b, float c, double d, size_t e, wchar_t f) {
  if (a != TEST_CHAR) {
    return 1;
  }
  if (b != TEST_INT) {
    return 2;
  }
  if ((c - TEST_FLOAT) > 0.0f || (c - TEST_FLOAT) < 0.0f) {
    return 3;
  }
  if ((d - TEST_DOUBLE) > 0.0f || (d - TEST_DOUBLE) < 0.0f) {
    return 4;
  }
  if (e != TEST_SIZE_T) {
    return 5;
  }
  if (f != TEST_WCHAR) {
    return 6;
  }
  return 0;
}

int testEcall2(struct struct_t a, enum enum_t b, enum enum_t c, enum enum_t d, union union_t u) {
  if (a.x != (uint16_t)TEST_X) {
    return 1;
  }
  if (a.y != (uint32_t)TEST_Y) {
    return 2;
  }
  if (a.z != (uint64_t)TEST_Z) {
    return 3;
  }
  if (b != ENUM_X) {
    return 4;
  }
  if (c != ENUM_Y) {
    return 5;
  }
  if (d != ENUM_Z) {
    return 6;
  }
  if (u.x != (uint64_t)TEST_Z) {
    return 7;
  }
  if (u.y != (uint64_t)TEST_Z) {
    return 8;
  }
  if (u.z != (uint64_t)TEST_Z) {
    return 9;
  }
  return 0;
}

int testEcall3(int a[2], int b[2], int c[2]) {
  b[0] = a[0];
  b[1] = a[1];
  a[0] = 0;
  a[1] = 0;
  int t = c[1];
  c[1] = c[0];
  c[0] = t;
  return 0;
}

int testEcall4(char* s1, const char* s2, char* s3) {
  printf("%s, %s %s\n", s1, s2, s3);
  if (strcmp(s1, TEST_S1) != 0) {
    return 1;
  }
  if (strcmp(s2, TEST_S2) != 0) {
    return 2;
  }
  if (strcmp(s3, TEST_S3) != 0) {
    return 3;
  }
  strncpy(s3, TEST_S4, strlen(s3));
  return 0;
}

#define TESTME(proc) do { int ret = 1; (proc); if (ret) { return __LINE__; } } while(0)
#define ASSERTME(proc) do { int ret = (proc); if (!ret) { return __LINE__; } } while(0)

int testOcallSimple(char dummy) {
  ocall0();
  printf("OCALL 0 done\n");

  struct struct_t a = { (uint16_t)TEST_X, (uint32_t)TEST_Y, (uint64_t)TEST_Z };
  union union_t u;
  u.x = TEST_Z;

  TESTME(ocall1(&ret, TEST_CHAR, TEST_INT, TEST_FLOAT, TEST_DOUBLE, TEST_SIZE_T, TEST_WCHAR));
  printf("OCALL 1 done\n");
  TESTME(ocall2(&ret, a, ENUM_X, ENUM_Y, ENUM_Z, u));
  printf("OCALL 2 done\n");

  int aa[2] = {1, 2}; // [in]     shall be copied to ab but itself left unchanged
  int ab[2] = {3, 4}; // [out]    shall contain aa
  int ac[2] = {5, 6}; // [in,out] shall be index-swapped
  TESTME(ocall3(&ret, aa, ab, ac));
  ASSERTME(aa[0] == 1);
  ASSERTME(aa[1] == 2);
  printf("%d %d %d %d %d %d\n", aa[0], aa[1], ab[0], ab[1], ac[0], ac[1]);
  ASSERTME(ab[0] == 1);
  ASSERTME(ab[1] == 2);
  ASSERTME(ac[0] == 6);
  ASSERTME(ac[1] == 5);
  printf("OCALL 3 done\n");
  return 0;
}

// The ecall decrements level
int testEcallRecursive(int level, int a[2], int b[2]) {
  printf("testEcallRecursive[%d]: %d %d\n", level, a[0], a[1]);
  ASSERTME(a[0] == level);
  ASSERTME(a[1] == -level);
  if (level > 0) {
    level--;
    a[0] = level;
    a[1] = -level;
    int aa[2];
    aa[0] = a[0];
    aa[1] = a[1];
    TESTME(testOcallRecursive(&ret, level, /*[in]*/a, /*[out]*/b));
    ASSERTME(aa[0] == a[0]);
    ASSERTME(aa[1] == a[1]);
    ASSERTME(b[0] == 100);
    ASSERTME(b[1] == 200);
  }
  // return swapped a via b to caller
  b[0] = a[1];
  b[1] = a[0];
  return 0;
}

int testEcallNested1(int level, int a[2], int b[2]) {
  printf("testEcallNested1: %04x %02x %02x\n", level, a[0], a[1]);
  ASSERTME(level == 0x0101);
  ASSERTME(a[0] == 0x11);
  ASSERTME(a[1] == 0x22);

  int aa[2] = {0x55, 0x66};
  int bb[2];
  printf("testEcallNested1 calling testOcallNested1\n");
  TESTME(testOcallNested1(&ret, 0x0202, aa, bb));
  printf("testEcallNested1 resuming\n");

  /* Shall be unmodified */
  ASSERTME(level == 0x0101);
  ASSERTME(a[0] == 0x11);
  ASSERTME(a[1] == 0x22);
  ASSERTME(aa[0] == 0x55);
  ASSERTME(aa[1] == 0x66);
  /* Shall be overwritten */
  ASSERTME(bb[0] == 0x77);
  ASSERTME(bb[1] == 0x88);
  /* Return to testOcalls */
  b[0] = 0x33;
  b[1] = 0x44;
  return 0;
}

int testEcallNested2(int level, int a[2], int b[2]) {
  printf("testEcallNested2: %04x %02x %02x\n", level, a[0], a[1]);
  ASSERTME(level == 0x0303);
  ASSERTME(a[0] == 0x99);
  ASSERTME(a[1] == 0xAA);

  int aa[2] = {0xDD, 0xEE};
  int bb[2];
  TESTME(testOcallNested2(&ret, 0x0404, aa, bb));

  /* Shall be unmodified */
  ASSERTME(level == 0x0303);
  ASSERTME(a[0] == 0x99);
  ASSERTME(a[1] == 0xAA);
  ASSERTME(aa[0] == 0xDD);
  ASSERTME(aa[1] == 0xEE);
  /* Shall be overwritten */
  ASSERTME(bb[0] == 0xFF);
  ASSERTME(bb[1] == 0x00);
  /* Return to testOcallNested1 */
  b[0] = 0xBB;
  b[1] = 0xCC;
  return 0;
}
