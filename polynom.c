#include "polynom.h"
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/string.h>

#define MAX(a, b) (((a)>(b))?(a):(b))

static uint8_t mod(int lhs, uint8_t p) {
    if (lhs < 0) return (p - ((-lhs) % p)) % p;
    return lhs % p;
}

static uint8_t get_ith_coeff(Polynom elem, size_t i) {
    return i < elem->coeff_size ? elem->coefficients[i] : 0;
}

static Polynom init(uint8_t n, uint8_t p) {
    Polynom element = (Polynom) kmalloc(sizeof(struct Polynom), GFP_KERNEL);
    if (element != NULL) {
        element->p = p;
        element->coeff_size = n;
        element->coefficients = (uint8_t * )(kmalloc(sizeof(uint8_t) * element->coeff_size, GFP_KERNEL));
        if (element->coefficients == NULL) {
            kfree(element);
            return NULL;
        }
    }
    return element;
}

static bool trim_zeroes(Polynom target) {
    uint8_t ind = target->coeff_size - 1;
    while (target->coefficients[ind] == 0 && ind > 0) {
        ind--;
    }
    target->coeff_size = ind + 1;
    uint8_t *insurance = target->coefficients;
    target->coefficients = (uint8_t *) krealloc(target->coefficients, target->coeff_size * sizeof(uint8_t), GFP_KERNEL);
    if (target->coefficients == NULL) {
        kfree(insurance);
        return false;
    }
    return true;
}

static uint8_t neg_coeff(uint8_t p, uint8_t coeff) {
    return (p - coeff) % p;
}

uint8_t PolynomDeg(Polynom element) {
    return element->coeff_size - 1;
}

Polynom IdentityPolynom(uint8_t p) {
    Polynom element = init(1, p);
    if (element != NULL) {
        element->coefficients[0] = 1;
    }
    return element;
}

Polynom ZeroPolynom(uint8_t p) {
    Polynom element = init(1, p);
    if (element != NULL) {
        element->coefficients[0] = 0;
    }
    return element;
}

Polynom PolynomFromArray(int const *array, uint8_t array_size, uint8_t p) {
    Polynom element = init(array_size, p);
    if (element != NULL) {
        for (uint8_t i = array_size - 1, ind = 0; i > 0; i--, ind++) {
            element->coefficients[ind] = mod(array[i], p);
        }
        element->coefficients[array_size - 1] = mod(array[0], p);
    }
    if (!trim_zeroes(element)) {
        kfree(element);
        return false;
    }
    return element;
}

Polynom CopyPolynom(Polynom elem) {
    Polynom res = init(elem->coeff_size, elem->p);
    if (res != NULL) {
        memcpy(res->coefficients, elem->coefficients, sizeof(uint8_t) * elem->coeff_size);
    }
    return res;
}

void FreePolynom(Polynom elem) {
    if (elem != NULL) {
        kfree(elem->coefficients);
    }
    kfree(elem);
}

Polynom AddPolynom(Polynom lhs, Polynom rhs) {
    if (lhs->p != rhs->p) return NULL;
    Polynom res = init(MAX(lhs->coeff_size, rhs->coeff_size), lhs->p);
    if (res == NULL) return NULL;
    for (size_t i = 0; i < res->coeff_size; i++) {
        res->coefficients[i] = mod((get_ith_coeff(lhs, i) + get_ith_coeff(rhs, i)), res->p);
    }
    if (!trim_zeroes(res)) {
        kfree(res);
        return NULL;
    }
    return res;
}

Polynom NegPolynom(Polynom elem) {
    Polynom res = CopyPolynom(elem);
    if (res != NULL) {
        for (size_t i = 0; i < res->coeff_size; i++) {
            res->coefficients[i] = neg_coeff(res->p, res->coefficients[i]);
        }
    }
    return res;
}

Polynom SubPolynom(Polynom lhs, Polynom rhs) {
    if (lhs->p != rhs->p) return NULL;
    Polynom tmp = NegPolynom(rhs);
    if (tmp != NULL) {
        Polynom res = AddPolynom(lhs, tmp);
        FreePolynom(tmp);
        return res;
    }
    return NULL;
}

bool AreEqualPolynom(Polynom lhs, Polynom rhs) {
    if (lhs->coefficients == rhs->coefficients) return true;
    return lhs->p == rhs->p && lhs->coeff_size == rhs->coeff_size &&
           memcmp(lhs->coefficients, rhs->coefficients, rhs->coeff_size) == 0;
}

Polynom MultPolynom(Polynom lhs, Polynom rhs) {
    if (lhs->p != rhs->p) return NULL;
    Polynom res = init(lhs->coeff_size + rhs->coeff_size - 1, lhs->p);
    if (res == NULL) return NULL;
    memset(res->coefficients, 0, res->coeff_size);
    for (int i = 0; i < lhs->coeff_size; i++) {
        for (int j = 0; j < rhs->coeff_size; j++) {
            res->coefficients[i + j] += mod(lhs->coefficients[i] * rhs->coefficients[j], res->p);
            res->coefficients[i + j] = mod(res->coefficients[i + j], res->p);
        }
    }
    if (!trim_zeroes(res)) {
        return NULL;
    }
    return res;
}

bool IsZeroPolynom(Polynom pol) {
    if (pol == NULL) return false;
    return pol->coeff_size == 1 && pol->coefficients[0] == 0;
}

bool IsIdentityPolynom(Polynom pol) {
    return pol->coeff_size == 1 && pol->coefficients[0] == 1;
}

Polynom ModPolynom(Polynom lhs, Polynom rhs) {
    if (lhs->p != rhs->p || IsZeroPolynom(rhs)) {
        return NULL;
    }
    if (PolynomDeg(lhs) < PolynomDeg(rhs)) {
        return CopyPolynom(lhs);
    }
    Polynom remainder = CopyPolynom(lhs);

    if (remainder == NULL) return NULL;
    Polynom quotient = ZeroPolynom(remainder->p);

    if (quotient == NULL) {
        FreePolynom(remainder);
        return NULL;
    }

    while (!IsZeroPolynom(remainder) && PolynomDeg(remainder) >= PolynomDeg(rhs)) {
        uint8_t coeff = mod(
                remainder->coefficients[remainder->coeff_size - 1] / rhs->coefficients[rhs->coeff_size - 1],
                remainder->p);
        uint8_t deg = PolynomDeg(remainder) - PolynomDeg(rhs);
        int *tmp_coeffs = (int *) (kmalloc(sizeof(int) * (deg + 1), GFP_KERNEL));
        tmp_coeffs[0] = coeff;
        for (int i = 1; i < deg + 1; i++) {
            tmp_coeffs[i] = 0;
        }

        Polynom tmp = PolynomFromArray(tmp_coeffs, deg + 1, remainder->p);
        Polynom dummy = quotient;
        quotient = AddPolynom(quotient, tmp);
        FreePolynom(dummy);
        dummy = remainder;
        Polynom mult = MultPolynom(tmp, rhs);
        remainder = SubPolynom(remainder, mult);

        FreePolynom(dummy);
        FreePolynom(mult);
        FreePolynom(tmp);
        kfree(tmp_coeffs);
    }
    FreePolynom(quotient);

    return remainder;
}
