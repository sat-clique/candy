void __llbmc_main(int x, int n, int z) {
    __llbmc_assert(x != (long)z*n || x % n == 0);
}
