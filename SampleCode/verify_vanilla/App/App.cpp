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

#include <cstdio>

#include "sgx_urts.h"
#include "Enclave_u.h"
#include "Tests.h"
#include <unistd.h>
#include <time.h>  
#include <math.h>
//MAX is for the constant for mod operations and amount calculation
#define MAX 100000000UL
#define MAX_WARM_UP 10000
#define OCALLS_TESTS_PER_LOOP 16
#define ENCLAVE_FILE "enclave.signed.so"

extern "C" long sgx_read_ocall_counter();

typedef struct {
  size_t time;
  size_t amount;
} benchmark;

sgx_enclave_id_t eid;
int updated;

benchmark* benchmark_array;
size_t bench_size = 0;

sgx_status_t enclave_init()
{
  sgx_launch_token_t token = { 0 };
  return sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, &token, &updated , &eid, NULL);
}

#define TEST_ECALL(proc) do { int ret = 1; sgx_status_t eret = (proc); if (eret != SGX_SUCCESS || ret) { printf("Test failed with %d, %d in %s (%s:%d)\n", eret, ret, __func__, __FILE__, __LINE__); exit(1); } } while(0)
#define ASSERTME(proc) do { int ret = proc; if (!ret) { printf("Test failed with %d in %s (%s:%d)\n", ret, __func__, __FILE__, __LINE__); exit(1); } } while(0)

void testEcalls()
{
  testEcall0(eid);
  printf("ECALL 0 done\n");
  struct struct_t a = { (uint16_t)TEST_X, (uint32_t)TEST_Y, (uint64_t)TEST_Z };
  union union_t u;
  u.x = TEST_Z;

  TEST_ECALL(testEcall1(eid, &ret, TEST_CHAR, TEST_INT, TEST_FLOAT, TEST_DOUBLE, TEST_SIZE_T, TEST_WCHAR));
  printf("ECALL 1 done\n");
  TEST_ECALL(testEcall2(eid, &ret, a, ENUM_X, ENUM_Y, ENUM_Z, u));
  printf("ECALL 2 done\n");

  int aa[2] = {1, 2}; // [in]     shall be copied to ab but itself left unchanged
  int ab[2] = {3, 4}; // [out]    shall contain aa
  int ac[2] = {5, 6}; // [in,out] shall be index-swapped
  TEST_ECALL(testEcall3(eid, &ret, aa, ab, ac));
  ASSERTME(aa[0] == 1);
  ASSERTME(aa[1] == 2);
  ASSERTME(ab[0] == 1);
  ASSERTME(ab[1] == 2);
  ASSERTME(ac[0] == 6);
  ASSERTME(ac[1] == 5);
  printf("ECALL 3 done\n");
  
  char s1[] = TEST_S1; // [in]
  // const char s2        [in]
  char s3[] = TEST_S3; // [in,out]
  TEST_ECALL(testEcall4(eid, &ret, s1, TEST_S2, s3));
  printf("%s\n", s3);
  ASSERTME(strcmp(s1, TEST_S1) == 0);
  ASSERTME(strcmp(s3, TEST_S4) == 0);
  printf("ECALL 4 done\n");
}

void ocall0() {
}

int ocall1(char a, int b, float c, double d, size_t e, wchar_t f) {
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

int ocall2(struct struct_t a, enum enum_t b, enum enum_t c, enum enum_t d, union union_t u) {
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

int ocall3(int a[2], int b[2], int c[2]) {
  b[0] = a[0];
  b[1] = a[1];
  a[0] = 0;
  a[1] = 0;
  int t = c[1];
  c[1] = c[0];
  c[0] = t;
  return 0;
}

int testOcallNested1(int level, int a[2], int b[2]) {
  printf("testOcallNested1: %04x %02x %02x\n", level, a[0], a[1]);
  ASSERTME(level == 0x0202);
  ASSERTME(a[0] == 0x55);
  ASSERTME(a[1] == 0x66);

  int aa[2] = {0x99, 0xAA};
  int bb[2];
  printf("testOcallNested1 calling testEcallNested2\n");
  TEST_ECALL(testEcallNested2(eid, &ret, 0x0303, aa, bb));
  printf("testOcallNested1 resuming\n");
  /* Shall be unmodified */
  ASSERTME(level == 0x0202);
  ASSERTME(a[0] == 0x55);
  ASSERTME(a[1] == 0x66);
  ASSERTME(aa[0] == 0x99);
  ASSERTME(aa[1] == 0xAA);
  /* Shall be overwritten */
  ASSERTME(bb[0] == 0xBB);
  ASSERTME(bb[1] == 0xCC);
  /* Return to testOcallNested1 */
  b[0] = 0x77;
  b[1] = 0x88;
  return 0;
}

int testOcallNested2(int level, int a[2], int b[2]) {
  printf("testOcallNested2: %04x %02x %02x\n", level, a[0], a[1]);
  ASSERTME(level == 0x0404);
  ASSERTME(a[0] == 0xDD);
  ASSERTME(a[1] == 0xEE);
  /* Return to testEcallNested2 */
  b[0] = 0xFF;
  b[1] = 0x00;
  return 0;
}

int testOcallRecursive(int level, int a[2], int b[2]) {
  printf("testOcallRecursive[%d]: %d %d\n", level, a[0], a[1]);
  ASSERTME(a[0] == level);
  ASSERTME(a[1] == -level);
  int aa[2];
  aa[0] = a[0];
  aa[1] = a[1];
  /* Ecall shall decrement level */
  TEST_ECALL(testEcallRecursive(eid, &ret, level, /*[in]*/a, /*[out]*/b));
  /* A must remain unchanged */
  ASSERTME(aa[0] == a[0]);
  ASSERTME(aa[1] == a[1]);
  if (level > 0) {
    /* B must be swapped A of next iteration */
    ASSERTME(b[0] == a[1]+1);
    ASSERTME(b[1] == a[0]-1);
  }
  b[0] = 100;
  b[1] = 200;
  return 0;
}

void ocall_print_string(const char *str)
{
  printf("%s", str);
}

void testOcalls()
{
  TEST_ECALL(testOcallSimple(eid, &ret, 0));
  printf("Ocall Simple done\n");

  printf("Testing nested Ocalls\n");
  printf("This will hang with SBOX, since nesting is not implemented yet\n");
  int a[2] = {0x11, 0x22};
  int b[2];
  TEST_ECALL(testEcallNested1(eid, &ret, 0x0101, a, b));
  ASSERTME(b[0] == 0x33);
  ASSERTME(b[1] == 0x44);
  printf("Ocall Nested done\n");

  int level = 10;
  int aa[2] = {level, -level};
  int bb[2];
  TEST_ECALL(testEcallRecursive(eid, &ret, level, aa, bb));
  printf("Ocall Recursive done\n");
}

int main(int argc, char** argv)
{
  if (SGX_SUCCESS != enclave_init()) {
    printf("Unable to initialize enclave\n");
    return -1;
  }
  printf("Ready to run tests\n");
  testEcalls();
  testOcalls();
  printf("All tests passed\n");
  sgx_destroy_enclave(eid);
  return 0;
}
