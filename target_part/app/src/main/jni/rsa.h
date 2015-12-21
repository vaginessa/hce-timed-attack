#ifndef CARD_EMULATION_RSA_H
#define CARD_EMULATION_RSA_H

enum {
    MAX_NUM_SIZE = 1000,
    TARGET_BASE = 256,
    SOURCE_BASE = 10,
    NULL_ANSI = 48,
    MAX_MSG_LEN = 100000,
};

typedef struct {
    int size;
    int sign;
    char num[MAX_NUM_SIZE];
} BigInt;

extern BigInt add_mod_bignum(BigInt a, BigInt b, BigInt m);
extern BigInt sub_bignum(BigInt a, BigInt b);
extern BigInt add_bignum(BigInt a, BigInt b);
extern BigInt multmod_bignum(BigInt a, BigInt b, BigInt m);
extern void clean_bignum(BigInt *a, int is_save_sign);
extern void modular(BigInt *a, BigInt m);
extern void print_bignum_target(BigInt a);
extern BigInt powmod_bignum(BigInt a, BigInt b, BigInt m);
extern BigInt euclidean(BigInt a, BigInt b, BigInt *x, BigInt *y);
extern jboolean rsa_encryption(char *text_str);
extern void rsa_decryption(char *text_str);
extern void elgamal(void);

#endif //CARD_EMULATION_RSA_H