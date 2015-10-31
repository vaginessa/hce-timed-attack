#include <jni.h>

#include "rsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *rsa_input = "E:\\rsa_input.txt";
static const char *elgamal_input = "E:\\elgamal_input.txt";

// Функция для вычисления, меньше ли текущая строка, чем основание
// целевой системы счисления (256 по умолчанию)
static unsigned char is_end(const char *str)
{
    int end_mark = 0;
    int i = 0;

    // Пока текущий символ - не символ конца строки
    while (str[i] != '\0') {
        // увеличить значение в 10 раз
        end_mark *= SOURCE_BASE;
        // считать очередной символ из строки
        end_mark += str[i] - NULL_ANSI;

        // Если считанное число уже больше 256, то вернуть 0
        if (end_mark > TARGET_BASE) {
            return 0;
        }

        i++;
    }

    // Иначе вернуть это число
    return end_mark;
}

// Посчитать частное и остаток от деления на 256 (по умолчанию)
static unsigned char calc_interim(int cur_int, char *interim)
{
    int modifier = 0;

    // Пока делимое >= 256 * modifier
    while (cur_int >= TARGET_BASE * modifier) {
        // увеличить modifier
        modifier++;
    }

    // Записать в частное modifier - 1
    interim[0] = NULL_ANSI + modifier - 1;

    // Вернуть разницу между делимым и (modifier - 1)*256
    return (unsigned char) (cur_int - (modifier - 1) * TARGET_BASE);
}

// сравнение двух больших чисел
// возвращаемый результат -1 - больше второе число, 1 - первое,
// 0 - числа равны
static int compare_nums(BigInt num1, BigInt num2)
{
    int i;

    // если знаки разные - положительное больше отрицательного
    if (num1.sign > num2.sign) {
        return -1;
    } else if (num1.sign < num2.sign) {
        return 1;
    }

    // если размерность у чисел разная - число с большей размерностью больше
    if ((unsigned char) num1.size > (unsigned char) num2.size) {
        return 1;
    }
    if ((unsigned char) num1.size < (unsigned char) num2.size) {
        return -1;
    }

    // иначе сравнивать все разряды, начиная со старшего - первый не совпавший
    // по значению даст верный ответ
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

// вычитание без модуля
BigInt sub_bignum(BigInt num1, BigInt num2)
{
    BigInt res;
    int carry = 0;
    int shift = 0;
    int i;
    int j;

    if (num1.sign != num2.sign) {
        /* a - (-b) == a + b */
        if (!num1.sign && num2.sign) {
            num2.sign = 0;
            res = add_bignum(num1, num2);
            return res;
        }
        /* -a - b == - (a + b) */
        num1.sign = 0;
        num2.sign = 0;
        res = add_bignum(num1, num2);
        res.sign = 1;
        return res;

    } else if (num1.sign && num2.sign) {
        /* -a - (-b) == b - a */
        num1.sign = 0;
        num2.sign = 0;
        res = sub_bignum(num2, num1);
        return res;
    }

    /* сравнить числа */
    switch(compare_nums(num1, num2)) {
        case 0:
            /* если числа равны - результат 0 */
            clean_bignum(&res, 0);
            return res;
        case -1:
            // если второе число больше первого - поменять аргументы функции
            // местами и указать знак результата минус
            res = sub_bignum(num2, num1);
            res.sign = 1;
            return res;
    }

    clean_bignum(&res, 0);

    // проинициализировать разрядную размерность результата
    // как размер первого аргумента (большего)
    res.size = num1.size;

    // пройтись по всем разрядам результата
    for (i = MAX_NUM_SIZE - 1; i >= MAX_NUM_SIZE - res.size; i--) {
        if ((unsigned char) num1.num[i] - (unsigned char) num2.num[i] - carry < 0) {
            // если разряд второго числа меньше разряда второго (с учетом переноса)
            // то перенести со старшего разряда и поставить флаг переноса
            res.num[i] = num1.num[i] - num2.num[i] - carry + TARGET_BASE;
            carry = 1;
        } else {
            // иначе - просто вычесть из одного разряда другой и перенос
            res.num[i] = num1.num[i] - num2.num[i] - carry;
            carry = 0;
        }
    }

    // посчитать количество нулевых старших разрядов (которые могли
    // образоваться в результате вычитания)
    for (i = MAX_NUM_SIZE - res.size; i < MAX_NUM_SIZE; i++) {
        if (res.num[i] == 0) {
            shift++;
        } else {
            break;
        }
    }

    //уменьшить размерность числа на вычисленное на предыдущем шаге значение
    res.size -= shift;

    return res;
}

//сложение без модуля
BigInt add_bignum(BigInt num1, BigInt num2)
{
    BigInt res;
    int i;
    int j;
    int carry = 0;

    if (num1.sign != num2.sign) {
        /* -a + b == b - a */
        if (num1.sign && !num2.sign) {
            num1.sign = 0;
            res = sub_bignum(num2, num1);
            return res;
        }

        /*  a + (-b) == a - b */
        num2.sign = 0;
        res = sub_bignum(num1, num2);
        return res;

    } else if (num1.sign && num2.sign) {
        /* -a + (-b) == - (a + b) */
        num1.sign = 0;
        num2.sign = 0;
        res = add_bignum(num1, num2);
        res.sign = 1;
        return res;
    }

    clean_bignum(&res, 0);

    // проинициализировать размерность результата размерностью
    // большего числа
    if (num1.size >= num2.size) {
        res.size = num1.size;
    } else {
        res.size = num2.size;
    }

    // пройтись по всем разрядам результата
    for (i = MAX_NUM_SIZE - 1; i >= MAX_NUM_SIZE - res.size; i--) {
        if ((unsigned char) num1.num[i] + (unsigned char) num2.num[i] + carry
            > TARGET_BASE - 1) {
            // если результат сложения будет больше максимального числа,
            // которое способно вместиться в разряд (256 по умолчанию)
            // то уменьшить результат сложения на 256 и указать перенос
            // на старший разряд
            res.num[i] = num1.num[i] + num2.num[i] + carry - TARGET_BASE;
            carry = 1;
        } else {
            // иначе - просто сложить
            res.num[i] = num1.num[i] + num2.num[i] + carry;
            carry = 0;
        }
    }

    if (carry) {
        // если в последнем разряде был перенос - увеличить размерность числа
        // и указать старшему разярду значение 1
        res.num[i] = 1;
        res.size++;
    }

    return res;
}

// суммирование по модулю
BigInt add_mod_bignum(BigInt num1, BigInt num2, BigInt mod)
{
    BigInt res;

    // сложить без взятия модуля
    res = add_bignum(num1, num2);
    // взять модуль
    modular(&res, mod);

    return res;
}

// обнулить число, второй аргумент - сохранить ли знак обнуляемого числа
// is_save_sign == 1 - знак сохранится, иначе - нет
void clean_bignum(BigInt *num1, int is_save_sign)
{
    int iter = 0;

    // обнулить размерность
    num1->size = 0;

    // все разряды
    for (; iter < MAX_NUM_SIZE; iter++) {
        num1->num[iter] = 0;
    }

    // проверить нужно ли сохранять знак, если нет - указать его положительным
    if (!is_save_sign) {
        num1->sign = 0;
    }
}

// умножение по модулю
BigInt multmod_bignum(BigInt num1, BigInt num2, BigInt mod)
{
    int size_iter = 0;
    unsigned char bit_iter = 0;
    BigInt res;
    BigInt sub_res;

    // обнулить числа
    clean_bignum(&res, 0);
    clean_bignum(&sub_res, 0);

    //пройтись по всем разрядам (начиная со старшего) второго числа
    for (size_iter = num2.size; size_iter > 0; size_iter--) {
        // пройтись по всем битам в текущем разряде (начиная со старшего)
        for (bit_iter = 128; bit_iter >= 1; bit_iter >>= 1) {
            // увеличить результат вдвое по модулю (с помощью сложения числа
            // самого с собой)
            res = add_mod_bignum(res, res, mod);
            if (num2.num[MAX_NUM_SIZE - size_iter] & bit_iter) {
                // если текущий бит - 1, то увеличить результат на первое число
                // по модулю
                res = add_mod_bignum(num1, res, mod);
            }
        }
    }

    // если знаки разные - знак результата отрицателен, иначе - положителен
    if (num1.sign != num2.sign) {
        res.sign = 1;
    }

    return res;
}

// взятие модуля от числа
void modular(BigInt *num1, BigInt mod_num)
{
    // пока модуль меньше числа
    while(compare_nums(*num1, mod_num) != -1) {
        // вычитать от числа модуль
        *num1 = sub_bignum(*num1, mod_num);
    }
}

// перевод строки, в которой записано десятичное число
// в формат BigInt (число по основанию 256)
// Алгоритм - рекурсивное деление числа на основание, с записью
// остатка от деления в результат
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

    // обнулить результат
    clean_bignum(num, 0);

    // обнулить строку с текущим десятичным числом
    memset(&current_string, '\0', sizeof(char) * MAX_NUM_SIZE);

    // скопировать входную строку в текущую
    while (str[i] != '\0') {
        current_string[i] = str[i];
        i++;
    }

    // если первый символ во входной строке знак минус, то
    // указать знак результата минус и отбросить символ минуса из
    // текущей строки
    if (current_string[0] == '-') {
        num->sign = 1;
        i = 0;
        while (current_string[i] != '\0') {
            current_string[i] = current_string[i + 1];
            i++;
        }
    }

    while(1) {
        // обнулить частное
        memset(&interim, '\0', sizeof(char) * MAX_NUM_SIZE);

        // цикл - пока текущий символ не равен символу окончания строки
        while (current_string[str_iter] != '\0') {
            // умножить текущее число на 10
            cur_int *= SOURCE_BASE;
            // прибавить очередное число из текущей строки
            cur_int += current_string[str_iter] - NULL_ANSI;
            // цикл - пока текущее число меньше 256 и следующий символ
            // не конец текущей строки
            while (cur_int < TARGET_BASE
                   && current_string[str_iter + 1] != '\0') {
                // умножить текущее число на 10
                cur_int *= SOURCE_BASE;
                // увеличить итератор текущего символа в строке
                str_iter++;
                // считать очередное число из строки
                cur_int += current_string[str_iter] - NULL_ANSI;

                //если это не первое число в частном
                if (ret_iter) {
                    // добавить в частное 0
                    interim[ret_iter] = '0';
                    // увеличить итератор частного
                    ret_iter++;
                }
            }

            // функция записывает очередной символ в частное
            // и возвращает остаток от разности
            val = calc_interim(cur_int, &interim[ret_iter]);
            // текущим числом становится остаток от деления
            cur_int = val;
            ret_iter++;
            str_iter++;
        }

        // после того, как остался последний остаток
        // он записывается в первый нулевой разряд (начиная с младших)
        num->num[MAX_NUM_SIZE - num_iter - 1] = val;
        // увеличить итераторы, обнулить временные значения
        num_iter++;
        cur_int = 0;
        val = 0;
        str_iter = 0;
        ret_iter = 0;
        i = 0;

        memset(&current_string, '\0', sizeof(char) * MAX_NUM_SIZE);

        // предыдущее частное становится новым делимым
        while (interim[i] != '\0') {
            current_string[i] = interim[i];
            i++;
        }

        // вычислить не находится ли в текущей строке число,
        // меньше основания? Если да, то записать это число в очередной
        // разряд и увеличить размерность числа
        if (is_end(&current_string[0]) || current_string[0] == '0') {
            num->num[MAX_NUM_SIZE - num_iter - 1] = is_end(&current_string[0]);
            num->size = num_iter + 1;
            // если число оказалось 0, то уменьшить размерность обратно
            if (num->num[MAX_NUM_SIZE - num_iter - 1] == 0) {
                num->size--;
            }
            return;
        }

    }
}

// Посчитать количество битов в числе
static int collect_bit_stat(BigInt num1)
{
    int ret = 0;
    unsigned char bit_iter;
    int is_unit_found = 0;

    // В одном разряде числа 8 битов, соответственно можно сразу сказать,
    // что во всех разярдах, кроме старшего количество бит = 8
    ret = (num1.size - 1) * 8;

    for (bit_iter = 128; bit_iter >= 1; bit_iter >>= 1) {
        // с помощью битовой маски найти первый ненулевой бит (начиная со старших)
        if (num1.num[MAX_NUM_SIZE - num1.size] & bit_iter) {
            is_unit_found = 1;
        }
        // если ненулевой бит не был найден - продолжить его искать
        if (!is_unit_found) {
            continue;
        }
        // иначе - начать сбор статистики
        ret++;
    }

    return ret;
}

// Возвести число в степень по модулю
BigInt powmod_bignum(BigInt num1, BigInt num2, BigInt mod_num)
{
    int loop_iter;
    int bit_num = 0;
    int current_num = 0;
    unsigned char bit_iter = 1;

    BigInt res;
    // проинициализировать временный результат входным числом
    BigInt sub_res = num1;

    // обнулить число
    clean_bignum(&res, 0);

    // проинициализировать число единицей
    res.num[MAX_NUM_SIZE - 1] = 1;
    res.size = 1;

    // посчитать количество битов в степени
    bit_num = collect_bit_stat(num2);

    // пройтись по всем битам степени
    for (loop_iter = 0;
         loop_iter < bit_num;
         loop_iter++) {
        // посчитать номер текущего разряда на основе номера текущего бита
        current_num = MAX_NUM_SIZE - loop_iter / 8 - 1;
        if (num2.num[current_num] & bit_iter) {
            // если бит равен 1, то умножить число по модулю на временный
            // результат
            res = multmod_bignum(res, sub_res, mod_num);
        }
        // возвести временный результат во вторую степень (с помощью умножения
        // на самого себя)
        sub_res = multmod_bignum(sub_res, sub_res, mod_num);

        // рассматривать следующий бит в разряде
        bit_iter <<= 1;
        // если битовая маска вышла за пределы разрядной сетки unsigned char
        // то вернуть её в значение 1
        if (!bit_iter) {
            bit_iter = 1;
        }
    }

    return res;
}

// вывести на экран число в десятичном виде
static void print_decimal_bignum(BigInt num1)
{
    int i = 0;
    BigInt num_source;
    BigInt res;
    BigInt max_num;
    int power = 0;
    int out_mult = 0;
    int string_iter = 0;

    char output[MAX_NUM_SIZE * 256];

    memset(output, '\0', MAX_NUM_SIZE);

    // если число отрицательное, напечатать знак минуса и модифицировать знак
    // числа, изменив его на положительный
    if (num1.sign) {
        printf("-");
        num1.sign = 0;
    }

    // обнулить результат
    clean_bignum(&res, 0);
    clean_bignum(&num_source, 0);
    clean_bignum(&max_num, 0);

    // проинициализировать максимально возможное число (для отсутсвия операции
    // взятия модуля
    for (i = 0; i < MAX_NUM_SIZE; i++) {
        max_num.num[i] = 0xFF;
        max_num.size++;
    }

    // num_source - специальное число, равное 10 в десятичном виде
    num_source.size = 1;
    num_source.num[MAX_NUM_SIZE - 1] = SOURCE_BASE;
    // проинициализировать res единицей
    res.size = 1;
    res.num[MAX_NUM_SIZE - 1] = 1;

    // вычислить самую маленькую степень 10, которой нет в числе
    while (compare_nums(num1, res) != -1) {
        // если число меньше res, то умножить res на 10
        power++;
        // увеличить счетчик степеней
        res = multmod_bignum(res, num_source, max_num);
    }

    // уменьшить счетчик степени, чтобы она указывала на самую большую степень
    // 10, которая есть в числе
    power--;

    while (1) {
        out_mult = 0;
        clean_bignum(&res, 0);
        // проинициализировать res 1
        res.num[MAX_NUM_SIZE - 1] = 1;
        res.size = 1;
        for (i = 0; i < power; i++) {
            // с помощью умножения на 10 посчитать максимальную степень
            // 10, которое есть в числе
            res = multmod_bignum(res, num_source, max_num);
        }

        // вычитать эту степень из числа до тех пор, пока число не станет
        // меньше её
        while (compare_nums(num1, res) != -1) {
            num1 = sub_bignum(num1, res);
            // увеличить текущее число
            out_mult++;
        }

        // записать текущее число в строку в старший разряд
        output[string_iter] = out_mult + NULL_ANSI;
        string_iter++;
        // уменьшить максимальную степень 10, которая есть в числе
        power--;

        // если число равно 0
        if (num1.size == 0) {
            if (power != -1) {
                // но не все степени 10 пройдены
                for (i = 0; i < power + 1; i++) {
                    // записать в строку столько нулей, сколько степеней
                    // ещё не обработано (число оказалось кратно одной из
                    // степеней десятки)
                    output[string_iter] = '0';
                    string_iter++;
                }
            }
            // остановить цикл
            break;
        }
    }

    // вывести число в десятичном виде на экран
    printf("%s\n", output);
}

// функция для деления числа на 2 с помощью разрядных сдвигов
static void div_bignum_2(BigInt *num1)
{
    int i = 0;

    // младший разряд побитово сдвигается вправо на 1 бит
    num1->num[MAX_NUM_SIZE - 1] = (unsigned char) num1->num[MAX_NUM_SIZE - 1] >> 1;

    // пройтись по всем разрядам, кроме младшего (начиная с младших)
    for (i = MAX_NUM_SIZE - 2; i >= MAX_NUM_SIZE - num1->size; --i)	{
        // если младший бит = 1, то установить старший бит предыдущего разряда в 1
        if (num1->num[i] & 0x1) {
            num1->num[i + 1] |= 1 << 7;
        }
        // побитово сдвинуть текущий разряд вправо на 1 бит
        num1->num[i] = (unsigned char) num1->num[i] >> 1;
    }

    // если старший разряд оказался равен 0, а число в результате сдвига не
    // обнулилось, то уменьшить размер числа на 1
    if (num1->num[i + 1] == 0 && num1->size) {
        num1->size--;
    }
}

// Реализация расширеного бинарного алгоритма Евклида
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

    // обнуление используемых чисел
    clean_bignum(&X, 0);
    clean_bignum(&Y, 0);
    clean_bignum(&G, 0);
    clean_bignum(&A, 0);
    clean_bignum(&B, 0);
    clean_bignum(&C, 0);
    clean_bignum(&D, 0);
    clean_bignum(&two, 0);
    clean_bignum(&max_num, 0);

    // проинициализировать максимально возможное число (используется для
    // отсутствия операции взятия модуля в модульных функциях)
    for (i = 0; i < MAX_NUM_SIZE; i++) {
        max_num.num[i] = 0xFF;
        max_num.size++;
    }

    // проинициализировать используемые числа
    two.size = 1;
    G.size = 1;
    A.size = 1;
    D.size = 1;
    two.num[MAX_NUM_SIZE - 1] = 2;
    A.num[MAX_NUM_SIZE - 1] = 1;
    D.num[MAX_NUM_SIZE - 1] = 1;
    G.num[MAX_NUM_SIZE - 1] = 1;

    X = num1; // принициализировать u первым числом
    Y = num2; // проинициализировать v вторым числом

    // Пока оба числа четные, делить их на 2, а G умножать на 2
    while (!(X.num[MAX_NUM_SIZE - 1] & 0x1)
           &&  !(Y.num[MAX_NUM_SIZE - 1] & 0x1)) {
        div_bignum_2(&X);
        div_bignum_2(&Y);
        G = multmod_bignum(G, two, max_num);
    }

    // Пока u != 0
    while(X.size) {
        // пока u четное
        while (!(X.num[MAX_NUM_SIZE - 1] & 0x1)) {
            // делить его вдвое
            div_bignum_2(&X);
            // если одно из чисел нечетное, то A = A + второе число,
            // B = B - первое число
            if ((A.num[MAX_NUM_SIZE - 1] & 0x1) || (B.num[MAX_NUM_SIZE - 1] & 0x1)) {
                A = add_bignum(A, num2);
                B = sub_bignum(B, num1);
            }

            // поделить А и B на два
            div_bignum_2(&A);
            div_bignum_2(&B);
        }

        // аналогично для v, но модифицировать C и D
        while (!(Y.num[MAX_NUM_SIZE - 1] & 0x1)) {
            div_bignum_2(&Y);
            if ((C.num[MAX_NUM_SIZE - 1] & 0x1) || (D.num[MAX_NUM_SIZE - 1] & 0x1)) {
                C = add_bignum(C, num2);
                D = sub_bignum(D, num1);
            }

            div_bignum_2(&C);
            div_bignum_2(&D);
        }

        // если u >= v, то
        if (compare_nums(X, Y) != -1) {
            // u = u -v
            X = sub_bignum(X, Y);
            // A = A - C
            A = sub_bignum(A, C);
            // B = B - D
            B = sub_bignum(B, D);
        } else { // иначе
            // v = v - u
            Y = sub_bignum(Y, X);
            // C = C - A
            C = sub_bignum(C, A);
            // D = D - B
            D = sub_bignum(D, B);
        }
    }

    // Соотношение Безу - числа C и D
    *x = C;
    *y = D;

    // результат алгоритма - G*v
    return multmod_bignum(G, Y, max_num);
}

// выбрать е при имеющемся d и значении функции Эйлера
static void choose_e_value(BigInt euler, BigInt *e, BigInt d, BigInt *X, BigInt *Y)
{
    BigInt res;

    // обнулить используемые числа
    clean_bignum(&res, 0);
    clean_bignum(X, 0);
    clean_bignum(e, 0);

    // посчитать НОД(значение функции Эйлера, d), а также соотношение Безу для них
    res = euclidean(euler, d, X, e);
    // второй множитель в соотношении Безу - e
    *Y = *e;
    while (e->sign) {
        // если e отрицательное, то взять модуль e по значении функции Эйлера
        *e = add_bignum(*e, euler);
    }
}

// реализация алгоритма шифрования RSA
void rsa_encryption(BigInt *d, BigInt *n, BigInt *text)
{
    BigInt max_num;
    BigInt euler;
    BigInt e;
    BigInt p;
    BigInt q;
    BigInt X;
    BigInt Y;

    FILE *input;

    int i = 0;
    char p_str[MAX_NUM_SIZE];
    char q_str[MAX_NUM_SIZE];
    char d_str[MAX_NUM_SIZE];
    char text_str[MAX_NUM_SIZE];

    // обнулить используемые переменные
    memset(p_str, '\0', MAX_NUM_SIZE);
    memset(q_str, '\0', MAX_NUM_SIZE);
    memset(d_str, '\0', MAX_NUM_SIZE);
    memset(text_str, '\0', MAX_NUM_SIZE);

    clean_bignum(&max_num, 0);
    clean_bignum(n, 0);
    clean_bignum(text, 0);
    clean_bignum(d, 0);
    clean_bignum(&e, 0);
    clean_bignum(&X, 0);
    clean_bignum(&Y, 0);
    clean_bignum(&euler, 0);

    // открыть файл с входными данными
    input = fopen(rsa_input, "r");

    // получить входные данные: в порядке
    // 1. Исходный текст
    fgets(text_str, MAX_NUM_SIZE, input);
    text_str[strlen(text_str) - 1] = '\0';
    // 2. Число p
    fgets(p_str, MAX_NUM_SIZE, input);
    p_str[strlen(p_str) - 1] = '\0';
    // 3. Число q
    fgets(q_str, MAX_NUM_SIZE, input);
    q_str[strlen(q_str) - 1] = '\0';
    // 4. Число d
    fgets(d_str, MAX_NUM_SIZE, input);

    // Заполнить соответствующие большие числа входными данными
    fill_bignum(&p, &p_str);
    fill_bignum(&q, &q_str);
    fill_bignum(d, &d_str);
    fill_bignum(text, &text_str);

    // Проинициализировать максимально возможное число (используется для того
    // чтобы избежать операции взятия модуля в модульных функциях)
    for (i = 0; i < MAX_NUM_SIZE; i++) {
        max_num.num[i] = 0xFF;
        max_num.size++;
    }

    // n = p*q
    *n = multmod_bignum(p, q, max_num);

    p.num[MAX_NUM_SIZE - 1]--;
    q.num[MAX_NUM_SIZE - 1]--;
    // Значение функции Эйлера равно (p-1)*(q-1)
    euler = multmod_bignum(p, q, max_num);
    p.num[MAX_NUM_SIZE - 1]++;
    q.num[MAX_NUM_SIZE - 1]++;

    // функция для вычисления значения e и соотношения Безу
    choose_e_value(euler, &e, *d, &X, &Y);

    // Вывести выичсленные значения на экран
    printf("First Bezout number\n");
    print_decimal_bignum(X);
    printf("\nSecond Bezout number\n");
    print_decimal_bignum(Y);
    printf("\npublic key\n");
    print_decimal_bignum(e);
    printf("\nprivate key\n");
    print_decimal_bignum(*d);
    printf("\nn\n");
    print_decimal_bignum(*n);
    printf("\nopen text\n");
    print_decimal_bignum(*text);

    // Зашифрованный текст = text^e mod n
    *text = powmod_bignum(*text, e, *n);

    // Вывести зашифрованный текст
    printf("\nencrypted text\n");
    print_decimal_bignum(*text);
    printf("\n");

    // Закрыть файл с входными данными
    fclose(input);
}

// Алгоритм дешифрации RSA
void rsa_decryption(BigInt d, BigInt n, BigInt text)
{
    int i = 0;

    // Расшированный текст = text^d mod n
    text = powmod_bignum(text, d, n);

    // Вывести расшифрованный текст
    printf("decrypted text\n");
    print_decimal_bignum(text);
    printf("\n");
}

int choose_func(void)
{
    BigInt res;
    BigInt num_A;
    BigInt num_B;
    BigInt num_M;
    BigInt X;
    BigInt Y;

    int i = 0;
    char input_string[MAX_NUM_SIZE];
    int cmd = 0;

    clean_bignum(&num_A, 0);
    clean_bignum(&num_B, 0);
    clean_bignum(&num_M, 0);
    clean_bignum(&X, 0);
    clean_bignum(&Y, 0);

    // Считать команду
    scanf("%d", &cmd);

    //Считать значения, необходимые для выбранной операции
    if (cmd < 7) {
        printf("input number A: ");
        memset(input_string, '\0', MAX_NUM_SIZE);
        scanf("%s", &input_string);
        fill_bignum(&num_A, &input_string);

        printf("input number B: ");
        memset(input_string, '\0', MAX_NUM_SIZE);
        scanf("%s", &input_string);
        fill_bignum(&num_B, &input_string);
    }

    if (cmd < 4) {
        printf("input number M: ");
        memset(input_string, '\0', MAX_NUM_SIZE);
        scanf("%s", &input_string);
        fill_bignum(&num_M, &input_string);
    }

    // Выполнить выбранную операцию
    switch(cmd) {
        case 1:
            res = powmod_bignum(num_A, num_B, num_M);
            printf("modular power(A, B, M) = R\n");
            break;
        case 2:
            res = multmod_bignum(num_A, num_B, num_M);
            printf("modular multiplication(A, B, M) = R\n");
            break;
        case 3:
            res = add_mod_bignum(num_A, num_B, num_M);
            printf("modular addition(A, B, M) = R\n");
            break;
        case 4:
            res = sub_bignum(num_A, num_B);
            printf("subtraction(A, B) = R\n");
            break;
        case 5:
            res = add_bignum(num_A, num_B);
            printf("addition(A, B) = R\n");
            break;
        case 6:
            res = euclidean(num_A, num_B, &X, &Y);
            printf("GCD(A, B) = C\nA*X + B*Y = C\n");
            break;
        case 7:
            rsa_encryption(&num_A, &num_B, &num_M);
            rsa_decryption(num_A, num_B, num_M);
            break;
        default:
            printf("strange command\n");
            return 1;
    }

    // Вывести значения, используемые в соответствующей операции и результат
    switch(cmd) {
        case 1:
        case 2:
        case 3:
            printf("A   ");
            print_decimal_bignum(num_A);
            printf("B   ");
            print_decimal_bignum(num_B);
            printf("M   ");
            print_decimal_bignum(num_M);
            printf("R   ");
            print_decimal_bignum(res);
            break;
        case 4:
        case 5:
            printf("A   ");
            print_decimal_bignum(num_A);
            printf("B   ");
            print_decimal_bignum(num_B);
            printf("R   ");
            print_decimal_bignum(res);
            break;
        case 6:
            printf("A   ");
            print_decimal_bignum(num_A);
            printf("B   ");
            print_decimal_bignum(num_B);
            printf("C   ");
            print_decimal_bignum(res);
            printf("X   ");
            print_decimal_bignum(X);
            printf("Y   ");
            print_decimal_bignum(Y);
            break;
    }

    getchar();
    getchar();

    return 0;
}

JNIEXPORT jstring JNICALL
Java_mkorvin_cardemulation_C2JavaWrapLib_encrypt(JNIEnv *env, jclass type) {

    choose_func();

    return (*env)->NewStringUTF(env, "123");
}