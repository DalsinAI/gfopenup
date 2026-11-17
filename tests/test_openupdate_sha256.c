/* Host test of src/openupdate_sha256.h against FIPS 180-4's own vectors and a million letters.
 *   cc -std=c99 -Wall -Werror -Isrc tests/test_openupdate_sha256.c -o t && ./t
 * tests/test_build_pack.py builds and runs it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "openupdate_sha256.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("FAIL %s\n", msg); } } while (0)

static const char *sha(const char *text, long chunk)
{
    static char out[65];
    struct oud_sha s;
    long len = (long)strlen(text), at;
    oud_sha_start(&s);
    for (at = 0; at < len; at += chunk)
        oud_sha_add(&s, (const unsigned char *)text + at, len - at < chunk ? len - at : chunk);
    oud_sha_end(&s, out);
    return out;
}

int main(void)
{
    static char million[1000001];
    static const char two[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    CHECK(!strcmp(sha("", 1), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), "nothing");
    CHECK(!strcmp(sha("abc", 64), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), "abc");
    CHECK(!strcmp(sha(two, 64), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"), "two blocks");
    CHECK(!strcmp(sha(two, 7), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"), "the same in pieces of 7");
    memset(million, 'a', 1000000);
    CHECK(!strcmp(sha(million, 4096), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"), "a million letters");
    {   /* 55, 56 and 64 bytes: the padding's edges */
        char t[65];
        memset(t, 'x', 64); t[55] = 0;
        CHECK(!strcmp(sha(t, 64), "d5e285683cd4efc02d021a5c62014694958901005d6f71e89e0989fac77e4072"), "55 bytes");
        t[55] = 'x'; t[56] = 0;
        CHECK(!strcmp(sha(t, 3), "04c26261370ee7541549d16dee320c723e3fd14671e66a099afe0a377c16888e"), "56 bytes, in pieces of 3");
        t[56] = 'x'; memset(t, 'x', 64); t[64] = 0;
        CHECK(!strcmp(sha(t, 5), "7ce100971f64e7001e8fe5a51973ecdfe1ced42befe7ee8d5fd6219506b5393c"), "64 bytes, in pieces of 5");
    }
    CHECK(oud_sha_same("BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD", sha("abc", 3)), "upper and lower case are the same");
    CHECK(!oud_sha_same("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ae", sha("abc", 3)), "one letter off");
    CHECK(!oud_sha_same("ba7816bf", sha("abc", 3)), "a short one");
    printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
