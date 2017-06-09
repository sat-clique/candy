void fermat(int32_t x, int32_t y, int32_t z) {
	__llbmc_assume(x > 0 && y > 0);

	__llbmc_assume(!__llbmc_ovfl_mul_int32_t(x, x));
	__llbmc_assume(!__llbmc_ovfl_mul_int32_t(x*x, x));
	__llbmc_assume(!__llbmc_ovfl_mul_int32_t(y, y));
	__llbmc_assume(!__llbmc_ovfl_mul_int32_t(y*y, y));
	__llbmc_assume(!__llbmc_ovfl_mul_int32_t(z, z));
	__llbmc_assume(!__llbmc_ovfl_mul_int32_t(z*z, z));
	__llbmc_assume(!__llbmc_ovfl_add_int32_t(x*x*x, y*y*y));

	__llbmc_assert(x*x*x + y*y*y != z*z*z);
}