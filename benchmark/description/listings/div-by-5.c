void __llbmc_main(unsigned int x) {
  unsigned int a, b, c, d;
 
  a = x / 5;

  b = (x >> 3) + (x >> 4);
  b += (b >> 4);
  b += (b >> 8);
  b += (b >> 16);
  c = x - 5*b;
  d = b + ((13*c) >> 6);

  __llbmc_assert(a == d);
}
