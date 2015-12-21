#include <jni.h>

#include "rsa.h"
#include <android/log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "rsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *predefined_q = "37975227936943673922808872755445627854565536638199";
static const char *predefined_p = "40094690950920881030683735292761468389214899724061";
static const char *predefined_e = "65537";

static BigInt e;
static BigInt n;
static BigInt d;

static char msg[MAX_MSG_LEN];

static unsigned char is_end(const char *str)
{
    int end_mark = 0;
    int i = 0;

    while (str[i] != '\0') {
        end_mark *= SOURCE_BASE;
        end_mark += str[i] - NULL_ANSI;

        if (end_mark > TARGET_BASE) {
            return 0;
        }

        i++;
    }

    return end_mark;
}

static unsigned char calc_interim(int cur_int, char *interim)
{
    int modifier = 0;

    while (cur_int >= TARGET_BASE * modifier) {
        modifier++;
    }

    interim[0] = NULL_ANSI + modifier - 1;

    return (unsigned char) (cur_int - (modifier - 1) * TARGET_BASE);
}

static int compare_nums(BigInt num1, BigInt num2)
{
    int i;

    if (num1.sign > num2.sign) {
        return -1;
    } else if (num1.sign < num2.sign) {
        return 1;
    }

    if ((unsigned char) num1.size > (unsigned char) num2.size) {
        return 1;
    }
    if ((unsigned char) num1.size < (unsigned char) num2.size) {
        return -1;
    }

    for (i = MAX_NUM_SIZE - num1.size; i < MAX_NUM_SIZE; i++) {
        if ((unsigned char) num1.num[i] > (unsigned char) num2.num[i]) {
            return 1;
        }
        if ((unsigned char) num1.num[i] < (unsigned char) num2.num[i]) {
            return -1;
        }
    }

    return 0;
}

BigInt sub_bignum(BigInt num1, BigInt num2)
{
    BigInt res;
    int carry = 0;
    int shift = 0;
    int i;
    int j;

    if (num1.sign != num2.sign) {
        if (!num1.sign && num2.sign) {
            num2.sign = 0;
            res = add_bignum(num1, num2);
            return res;
        }
        num1.sign = 0;
        num2.sign = 0;
        res = add_bignum(num1, num2);
        res.sign = 1;

        return res;

    } else if (num1.sign && num2.sign) {
        num1.sign = 0;
        num2.sign = 0;
        res = sub_bignum(num2, num1);

        return res;
    }

    switch(compare_nums(num1, num2)) {
        case 0:
            clean_bignum(&res, 0);
            return res;
        case -1:
            res = sub_bignum(num2, num1);
            res.sign = 1;
            return res;
    }

    clean_bignum(&res, 0);

    res.size = num1.size;

    for (i = MAX_NUM_SIZE - 1; i >= MAX_NUM_SIZE - res.size; i--) {
        if ((unsigned char) num1.num[i] - (unsigned char) num2.num[i] - carry < 0) {
            res.num[i] = num1.num[i] - num2.num[i] - carry + TARGET_BASE;
            carry = 1;
        } else {
            res.num[i] = num1.num[i] - num2.num[i] - carry;
            carry = 0;
        }
    }

    for (i = MAX_NUM_SIZE - res.size; i < MAX_NUM_SIZE; i++) {
        if (res.num[i] == 0) {
            shift++;
        } else {
            break;
        }
    }

    res.size -= shift;

    return res;
}

BigInt add_bignum(BigInt num1, BigInt num2)
{
    BigInt res;
    int i;
    int j;
    int carry = 0;

    if (num1.sign != num2.sign) {
        if (num1.sign && !num2.sign) {
            num1.sign = 0;
            res = sub_bignum(num2, num1);

            return res;
        }

        num2.sign = 0;
        res = sub_bignum(num1, num2);

        return res;

    } else if (num1.sign && num2.sign) {
        num1.sign = 0;
        num2.sign = 0;
        res = add_bignum(num1, num2);
        res.sign = 1;

        return res;
    }

    clean_bignum(&res, 0);

    if (num1.size >= num2.size) {
        res.size = num1.size;
    } else {
        res.size = num2.size;
    }

    for (i = MAX_NUM_SIZE - 1; i >= MAX_NUM_SIZE - res.size; i--) {
        if ((unsigned char) num1.num[i] + (unsigned char) num2.num[i] + carry
            > TARGET_BASE - 1) {
            res.num[i] = num1.num[i] + num2.num[i] + carry - TARGET_BASE;
            carry = 1;
        } else {
            res.num[i] = num1.num[i] + num2.num[i] + carry;
            carry = 0;
        }
    }

    if (carry) {
        res.num[i] = 1;
        res.size++;
    }

    return res;
}

BigInt add_mod_bignum(BigInt num1, BigInt num2, BigInt mod)
{
    BigInt res;

    res = add_bignum(num1, num2);

    modular(&res, mod);

    return res;
}

void clean_bignum(BigInt *num1, int is_save_sign)
{
    int iter = 0;

    num1->size = 0;

    for (; iter < MAX_NUM_SIZE; iter++) {
        num1->num[iter] = 0;
    }

    if (!is_save_sign) {
        num1->sign = 0;
    }
}

BigInt multmod_bignum(BigInt num1, BigInt num2, BigInt mod)
{
    int size_iter = 0;
    unsigned char bit_iter = 0;
    BigInt res;
    BigInt sub_res;

    clean_bignum(&res, 0);
    clean_bignum(&sub_res, 0);

    for (size_iter = num2.size; size_iter > 0; size_iter--) {
        for (bit_iter = 128; bit_iter >= 1; bit_iter >>= 1) {
            res = add_mod_bignum(res, res, mod);
            if (num2.num[MAX_NUM_SIZE - size_iter] & bit_iter) {
                res = add_mod_bignum(num1, res, mod);
            }
        }
    }

    if (num1.sign != num2.sign) {
        res.sign = 1;
    }

    return res;
}

void modular(BigInt *num1, BigInt mod_num)
{
    while(compare_nums(*num1, mod_num) != -1) {
        *num1 = sub_bignum(*num1, mod_num);
    }
}

static void fill_bignum(BigInt *num, const char *str)
{
    int str_iter = 0;
    int ret_iter = 0;
    int num_iter = 0;
    int i = 0;
    char interim[MAX_NUM_SIZE];
    char current_string[MAX_NUM_SIZE];
    int cur_int = 0;
    unsigned char val = 0;

    clean_bignum(num, 0);

    memset(&current_string, '\0', sizeof(char) * MAX_NUM_SIZE);

    while (str[i] != '\0') {
        current_string[i] = str[i];
        i++;
    }

    if (current_string[0] == '-') {
        num->sign = 1;
        i = 0;
        while (current_string[i] != '\0') {
            current_string[i] = current_string[i + 1];
            i++;
        }
    }

    while(1) {
        memset(&interim, '\0', sizeof(char) * MAX_NUM_SIZE);

        while (current_string[str_iter] != '\0') {
            cur_int *= SOURCE_BASE;
            cur_int += current_string[str_iter] - NULL_ANSI;
            while (cur_int < TARGET_BASE
                   && current_string[str_iter + 1] != '\0') {
                cur_int *= SOURCE_BASE;
                str_iter++;
                cur_int += current_string[str_iter] - NULL_ANSI;

                if (ret_iter) {
                    interim[ret_iter] = '0';
                    ret_iter++;
                }
            }

            val = calc_interim(cur_int, &interim[ret_iter]);
            cur_int = val;
            ret_iter++;
            str_iter++;
        }

        num->num[MAX_NUM_SIZE - num_iter - 1] = val;
        num_iter++;
        cur_int = 0;
        val = 0;
        str_iter = 0;
        ret_iter = 0;
        i = 0;

        memset(&current_string, '\0', sizeof(char) * MAX_NUM_SIZE);

        while (interim[i] != '\0') {
            current_string[i] = interim[i];
            i++;
        }

        if (is_end(&current_string[0]) || current_string[0] == '0') {
            num->num[MAX_NUM_SIZE - num_iter - 1] = is_end(&current_string[0]);
            num->size = num_iter + 1;
            if (num->num[MAX_NUM_SIZE - num_iter - 1] == 0) {
                num->size--;
            }
            return;
        }

    }
}

static int collect_bit_stat(BigInt num1)
{
    int ret = 0;
    unsigned char bit_iter;
    int is_unit_found = 0;

    ret = (num1.size - 1) * 8;

    for (bit_iter = 128; bit_iter >= 1; bit_iter >>= 1) {
        if (num1.num[MAX_NUM_SIZE - num1.size] & bit_iter) {
            is_unit_found = 1;
        }

        if (!is_unit_found) {
            continue;
        }

        ret++;
    }

    return ret;
}

BigInt powmod_bignum(BigInt num1, BigInt num2, BigInt mod_num)
{
    int loop_iter;
    int bit_num = 0;
    int current_num = 0;
    unsigned char bit_iter = 1;

    BigInt res;
    BigInt sub_res = num1;

    clean_bignum(&res, 0);

    res.num[MAX_NUM_SIZE - 1] = 1;
    res.size = 1;

    bit_num = collect_bit_stat(num2);

    for (loop_iter = 0;
         loop_iter < bit_num;
         loop_iter++) {
        current_num = MAX_NUM_SIZE - loop_iter / 8 - 1;
        if (num2.num[current_num] & bit_iter) {
            res = multmod_bignum(res, sub_res, mod_num);
        }

        sub_res = multmod_bignum(sub_res, sub_res, mod_num);

        bit_iter <<= 1;
        if (!bit_iter) {
            bit_iter = 1;
        }
    }

    return res;
}

static void get_str_from_bignum(BigInt num1, char *output)
{
    int i = 0;
    BigInt num_source;
    BigInt res;
    BigInt max_num;
    int power = 0;
    int out_mult = 0;
    int string_iter = 0;

    memset(output, '\0', MAX_NUM_SIZE);

    if (num1.sign) {
        __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "-");
        num1.sign = 0;
    }

    clean_bignum(&res, 0);
    clean_bignum(&num_source, 0);
    clean_bignum(&max_num, 0);

    for (i = 0; i < MAX_NUM_SIZE; i++) {
        max_num.num[i] = 0xFF;
        max_num.size++;
    }

    num_source.size = 1;
    num_source.num[MAX_NUM_SIZE - 1] = SOURCE_BASE;
    res.size = 1;
    res.num[MAX_NUM_SIZE - 1] = 1;

    while (compare_nums(num1, res) != -1) {
        power++;
        res = multmod_bignum(res, num_source, max_num);
    }

    power--;

    while (1) {
        out_mult = 0;
        clean_bignum(&res, 0);
        res.num[MAX_NUM_SIZE - 1] = 1;
        res.size = 1;
        for (i = 0; i < power; i++) {
            res = multmod_bignum(res, num_source, max_num);
        }

        while (compare_nums(num1, res) != -1) {
            num1 = sub_bignum(num1, res);
            out_mult++;
        }

        output[string_iter] = out_mult + NULL_ANSI;
        string_iter++;
        power--;

        if (num1.size == 0) {
            if (power != -1) {
                for (i = 0; i < power + 1; i++) {
                    output[string_iter] = '0';
                    string_iter++;
                }
            }
            break;
        }
    }
}

static void print_decimal_bignum(BigInt num1)
{
    char output[MAX_MSG_LEN];

    get_str_from_bignum(num1, output);

    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "%s\n", output);
}

static void div_bignum_2(BigInt *num1)
{
    int i = 0;

    num1->num[MAX_NUM_SIZE - 1] = (unsigned char) num1->num[MAX_NUM_SIZE - 1] >> 1;

    for (i = MAX_NUM_SIZE - 2; i >= MAX_NUM_SIZE - num1->size; --i) {
        if (num1->num[i] & 0x1) {
            num1->num[i + 1] |= 1 << 7;
        }
        num1->num[i] = (unsigned char) num1->num[i] >> 1;
    }

    if (num1->num[i + 1] == 0 && num1->size) {
        num1->size--;
    }
}

BigInt euclidean(BigInt num1, BigInt num2, BigInt *x, BigInt *y)
{
    int i = 0;
    BigInt X;
    BigInt Y;
    BigInt G;
    BigInt two;
    BigInt max_num;
    BigInt A;
    BigInt B;
    BigInt C;
    BigInt D;

    clean_bignum(&X, 0);
    clean_bignum(&Y, 0);
    clean_bignum(&G, 0);
    clean_bignum(&A, 0);
    clean_bignum(&B, 0);
    clean_bignum(&C, 0);
    clean_bignum(&D, 0);
    clean_bignum(&two, 0);
    clean_bignum(&max_num, 0);

    for (i = 0; i < MAX_NUM_SIZE; i++) {
        max_num.num[i] = 0xFF;
        max_num.size++;
    }

    two.size = 1;
    G.size = 1;
    A.size = 1;
    D.size = 1;
    two.num[MAX_NUM_SIZE - 1] = 2;
    A.num[MAX_NUM_SIZE - 1] = 1;
    D.num[MAX_NUM_SIZE - 1] = 1;
    G.num[MAX_NUM_SIZE - 1] = 1;
    X = num1;
    Y = num2;

    while (!(X.num[MAX_NUM_SIZE - 1] & 0x1)
           &&  !(Y.num[MAX_NUM_SIZE - 1] & 0x1)) {
        div_bignum_2(&X);
        div_bignum_2(&Y);
        G = multmod_bignum(G, two, max_num);
    }

    while(X.size) {
        while (!(X.num[MAX_NUM_SIZE - 1] & 0x1)) {
            div_bignum_2(&X);
            if ((A.num[MAX_NUM_SIZE - 1] & 0x1) || (B.num[MAX_NUM_SIZE - 1] & 0x1)) {
                A = add_bignum(A, num2);
                B = sub_bignum(B, num1);
            }

            div_bignum_2(&A);
            div_bignum_2(&B);
        }

        while (!(Y.num[MAX_NUM_SIZE - 1] & 0x1)) {
            div_bignum_2(&Y);
            if ((C.num[MAX_NUM_SIZE - 1] & 0x1) || (D.num[MAX_NUM_SIZE - 1] & 0x1)) {
                C = add_bignum(C, num2);
                D = sub_bignum(D, num1);
            }

            div_bignum_2(&C);
            div_bignum_2(&D);
        }

        if (compare_nums(X, Y) != -1) {
            X = sub_bignum(X, Y);
            A = sub_bignum(A, C);
            B = sub_bignum(B, D);
        } else {
            Y = sub_bignum(Y, X);
            C = sub_bignum(C, A);
            D = sub_bignum(D, B);
        }
    }

    *x = C;
    *y = D;

    return multmod_bignum(G, Y, max_num);
}

static void choose_d_value(BigInt euler, BigInt *d, BigInt *X, BigInt *Y)
{
    BigInt res;

    clean_bignum(&res, 0);
    clean_bignum(X, 0);
    clean_bignum(d, 0);

    res = euclidean(euler, e, X, d);
    print_decimal_bignum(res);

    while (d->sign) {
        *d = add_bignum(*d, euler);
    }
    while (X->sign) {
        *X = add_bignum(*X, euler);
    }

    *Y = *d;
    print_decimal_bignum(*d);
}

static void convert_to_decimal(char *str)
{
    int len = strlen(str);
    unsigned int i;

    for (i = 0; i < len; i++) {
            str[i] = str[i] + 48;
    }
}

jboolean rsa_encryption(char *text_str)
{
    BigInt max_num;
    BigInt euler;
    BigInt p;
    BigInt q;
    BigInt X;
    BigInt Y;
    BigInt one;
    BigInt text;

    int i = 0;
    char p_str[MAX_NUM_SIZE];
    char q_str[MAX_NUM_SIZE];
    char d_str[MAX_NUM_SIZE];

    memset(p_str, '\0', MAX_NUM_SIZE);
    memset(q_str, '\0', MAX_NUM_SIZE);
    memset(d_str, '\0', MAX_NUM_SIZE);

    clean_bignum(&one, 0);
    clean_bignum(&max_num, 0);
    clean_bignum(&X, 0);
    clean_bignum(&Y, 0);
    clean_bignum(&euler, 0);

    fill_bignum(&p, predefined_p);
    fill_bignum(&q, predefined_q);
    fill_bignum(&e, predefined_e);
    fill_bignum(&text, text_str);

    for (i = 0; i < MAX_NUM_SIZE; i++) {
        max_num.num[i] = 0xFF;
        max_num.size++;
    }

    one.num[MAX_NUM_SIZE - 1] = 1;

    n = multmod_bignum(p, q, max_num);

    p = sub_bignum(p, one);
    q = sub_bignum(q, one);
    euler = multmod_bignum(p, q, max_num);
    p = add_bignum(p, one);
    q = add_bignum(q, one);

    choose_d_value(euler, &d, &X, &Y);

    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "First Bezout number\n");
    print_decimal_bignum(X);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\nSecond Bezout number\n");
    print_decimal_bignum(Y);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\npublic key\n");
    print_decimal_bignum(e);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\nprivate key\n");
    print_decimal_bignum(d);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\nn\n");
    print_decimal_bignum(n);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\nopen text\n");
    print_decimal_bignum(text);

    text = powmod_bignum(text, e, n);

    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\nencrypted text\n");
    print_decimal_bignum(text);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\n");
    get_str_from_bignum(text, msg);

    return true;
}

void rsa_decryption(char *text_str)
{
    BigInt text;
    int i = 0;

    fill_bignum(&text, text_str);

    text = powmod_bignum(text, d, n);

    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "decrypted text\n");
    print_decimal_bignum(text);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "\n");
}

JNIEXPORT jboolean JNICALL
Java_mkorvin_cardemulation_C2JavaWrapLib_check_1pin(JNIEnv *env, jclass type, jbyteArray input) {
    jboolean isCopy;

    char *input_arr = (char*)(*env)->GetByteArrayElements(env, input, &isCopy);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "input is %x %x %x %x\n", input_arr[0], input_arr[1], input_arr[2], input_arr[3]);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "input is %x %x %x %x\n", input_arr[4], input_arr[5], input_arr[6], input_arr[7]);
    convert_to_decimal(input_arr);
    __android_log_print(ANDROID_LOG_DEBUG, "HCEDEMO", "input is %s\n", input_arr);
    jboolean res = rsa_encryption(input_arr);
    rsa_decryption(msg);
    if(isCopy) {
        (*env)->ReleaseByteArrayElements(env, input, input_arr, JNI_ABORT);
    }

    return res;
}
