#ifndef BN_H
#define BN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>
#include "../hash/hash.h"

// Values for Curve25519 and Poly1305
static const uint64_t P25519[]	= {0xFFFFFFEDull, 0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull, 0x7FFFFFFFull};
static const uint64_t N25519[]	= {0x5CF5D3EDull, 0x5812631Aull, 0xA2F79CD6ull, 0x14DEF9DEull, 0ull, 0ull, 0ull, 0x10000000ull};
static const uint64_t P1305[] = {0xfffffffbull, 0xffffffffull, 0xffffffffull, 0xffffffffull, 0x3ull};
static const uint64_t C1305[] = {0x0fffffff, 0x0ffffffc, 0x0ffffffc, 0x0ffffffc};

// Macros for organization
#define BIG_MAX_DIGITS		16
#define BIG_WORDS_SIZE		BIG_MAX_DIGITS * sizeof(dig_t)
#define BIG_BASE 			0x100000000ull
#define BIG_BASE_M			0xFFFFFFFFull
#define BIG_DIGIT_BITS		32
#define BIG_LESS 			0
#define BIG_EQUAL 			1
#define BIG_GREATER 		2
#define EVEN(a)				(!(*(a.value) & 1ull))

// Big Integer structure and init functions
typedef unsigned __int128 	twodig_t;
typedef uint64_t 			dig_t;

typedef struct _big_t{
	dig_t					value[BIG_MAX_DIGITS];
	bool 					sign;
}big_t;

#define big_new(a)			a = calloc(1, sizeof(big_t));

#define big_cpy(a, b)		memcpy(((big_t*)b)->value, ((big_t*)a)->value, BIG_WORDS_SIZE); 	\
							((big_t*)b)->sign = ((big_t*)a)->sign;								

#define big_null(a)			memset(((big_t*)a)->value, 0, BIG_WORDS_SIZE); 						\
							((big_t*)a)->sign = false;

// Logic and arithmetic operations functions
void 	big_mul				(big_t *, big_t *, big_t *);
void	big_mul_kts			(big_t *, big_t *, big_t *);
void	big_sqr				(big_t *, big_t *);
void 	big_sub				(big_t *, big_t *, big_t *);
void 	big_sum				(big_t *, big_t *, big_t *);
void	big_mnt_pow_25519	(big_t *, big_t *, big_t *);
void 	big_mod_inv			(big_t *, big_t *, big_t *);
void 	big_mod				(big_t *, big_t *, big_t *);
void	big_mod_25519		(big_t *, big_t *, big_t *);
void	big_lst				(big_t *, big_t *);
void 	big_rst				(big_t *, big_t *);
void 	big_rst_wrd			(big_t *, int, big_t *);
void 	big_lst_wrd			(big_t *, int, big_t *);
void 	big_and				(big_t *, big_t *, big_t *);
void	big_mod_1305		(big_t *, big_t *, big_t *);
void	big_sum_25519		(big_t *, big_t *, big_t *, big_t *);
void	big_sub_25519		(big_t *, big_t *, big_t *, big_t *);
void	big_mul_25519		(big_t *, big_t *, big_t *, big_t *);

// Utility functions (Compare, primes...)
int 	big_gth				(big_t *, big_t *);
int		big_gth_uns			(big_t *, big_t *);
int 	big_lgd_sym			(big_t *, big_t *);
bool 	big_eql				(big_t *, big_t *);
void 	big_to_hex			(big_t *);
dig_t*	big_to_bin			(big_t *, int *);
void 	bin_to_big			(char *, big_t *);
void	hex_to_big			(char *, big_t *);
int		big_get_len			(big_t *);
void	big_rnd_dig			(big_t *);

#endif