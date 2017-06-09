#define lshrl(a,b) (int64_t)((uint64_t)a >> (uint64_t)b)
#define lshri(a,b) (int32_t)((uint32_t)a >> (uint32_t)b)
#define trim(n) (int32_t)(uint32_t)(uint64_t)n
#define zext(n) (int64_t)(uint64_t)(uint32_t)n

void __llbmc_main(int32_t input) {
    int64_t magic = (int64_t)1374389535;
    char result;
    
    int32_t e = trim(lshrl(magic * ((int64_t)input), 32));
    int64_t a = (e >> 5) + lshri(e, 31);

    result = (char)(lshrl(-zext(input - (100 * a)), 31) & 1);

    if (trim(lshrl(100 * ((int64_t)a), 32)) != 0) {
        __llbmc_assert(result == 0);
    }
}
