/* SH-CORPUS-11 development-only deterministic zlib 1.3.1 smoke.
 * Authored test fixture, not the historical R2 workload.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int main(void) {
    enum { INPUT_SIZE = 32768 };
    unsigned char input[INPUT_SIZE];
    unsigned char restored[INPUT_SIZE];
    uint32_t seed = UINT32_C(0x13579bdf);
    uLongf compressed_size;
    uLongf restored_size = INPUT_SIZE;
    uLong expected_crc, expected_adler;
    unsigned char *compressed;
    int rc;
    size_t i;

    if (strcmp(zlibVersion(), "1.3.1") != 0) {
        fprintf(stderr, "wrong zlib version: %s\n", zlibVersion());
        return 2;
    }
    for (i = 0; i < INPUT_SIZE; ++i) {
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        input[i] = (unsigned char)((i % 128 < 96) ? ((i / 7) % 23 + 65) : ((seed >> 16) & 0xff));
    }
    expected_crc = crc32(0L, Z_NULL, 0);
    expected_crc = crc32(expected_crc, input, INPUT_SIZE);
    expected_adler = adler32(0L, Z_NULL, 0);
    expected_adler = adler32(expected_adler, input, INPUT_SIZE);
    /* Constants independently computed from the fixed input with Python zlib 1.3.1. */
    if (expected_crc != 0x3b0579c1UL || expected_adler != 0xa9157e7eUL) {
        fprintf(stderr, "reference checksums mismatch\n");
        return 7;
    }
    compressed_size = compressBound(INPUT_SIZE);
    compressed = malloc((size_t)compressed_size);
    if (!compressed) return 3;
    rc = compress2(compressed, &compressed_size, input, INPUT_SIZE, Z_BEST_COMPRESSION);
    if (rc != Z_OK) {
        fprintf(stderr, "compress2 failed: %d\n", rc);
        free(compressed);
        return 4;
    }
    rc = uncompress(restored, &restored_size, compressed, compressed_size);
    free(compressed);
    if (rc != Z_OK || restored_size != INPUT_SIZE || memcmp(input, restored, INPUT_SIZE) != 0) {
        fprintf(stderr, "roundtrip failed: %d\n", rc);
        return 5;
    }
    if (crc32(0L, restored, INPUT_SIZE) != expected_crc ||
        adler32(1L, restored, INPUT_SIZE) != expected_adler) {
        fprintf(stderr, "checksum mismatch\n");
        return 6;
    }
    printf("zlib-new:%u:%lu:%08lx:%08lx\n", INPUT_SIZE,
           (unsigned long)compressed_size,
           (unsigned long)expected_crc,
           (unsigned long)expected_adler);
    return 0;
}
