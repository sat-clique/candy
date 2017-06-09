extern int t[N]; // size parameter N set by define

static int avg_l(int *a, int n) {
    long s = 0, i;

    for (i = 0; i < n; i++)
        s += a[i];
    if (s < 0 && s % n != 0) {
        // avoid round-towards-zero
        return (s / n) - 1;
    } else {
        return s / n;
    }
}

static int avg_i(int *a, int n) {
    int i, p = 0, s = 0;

    for (i = 0; i < n; i++) {
        s += a[i] / n;
        p += a[i] % n;
        if (p >= n) { p -= n; s++; }
        else if (p < 0) { p += n; s--; }
    }

    return s;
}

void __llbmc_main(void) {
    int a_l, a_i;

    a_l = avg_l(t, N);
    a_i = avg_i(t, N);

    __llbmc_assert(a_l == a_i);
}