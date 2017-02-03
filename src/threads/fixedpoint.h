#ifndef FIXED_POINT_H
#define FIXED_POINT_H

typedef int fixed_point_t;
#define BINARY_POINT 14
#define SHIFT (1 << BINARY_POINT)

#define convert_int_to_fp(x) (x * SHIFT)

#define convert_fp_to_int_round_to_zero(x) (x / SHIFT)

#define convert_fp_to_int_round_to_nearest(x) (x >= 0 ? ((x + (SHIFT / 2)) / SHIFT) : ((x - (SHIFT / 2)) / SHIFT))

/* x is fixed and n is int */
#define add_f(x, n) (((x) + ((n) * SHIFT)))
#define sub_f(x, n) (((x) - ((n) * SHIFT)))

/* x and y are fixed */
#define mult_f(x, y) ((((int64_t) x) * (y) / SHIFT))
#define divide_f(x, y) ((((int64_t) x) * SHIFT / (y)))

#endif
