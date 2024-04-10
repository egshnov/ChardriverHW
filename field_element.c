#include "field_element.h"
#include <linux/slab.h>
#include <linux/string.h>

#define MAX(a, b) (((a)>(b))?(a):(b))

static FieldElement init(FiniteField f) {
    FieldElement element = (FieldElement) kmalloc(sizeof(struct FieldElement), GFP_KERNEL);
    //GFP_KERNEL - выделение производится от имени процесса запущенного в пространстве ядра
    if (element != NULL) {
        element->field = f;
    }
    return element;
}

static bool descend(FieldElement elem) {
    Polynom dummy = elem->pol;
    elem->pol = ModPolynom(elem->pol, elem->field->pol);
    FreePolynom(dummy);
    if (elem->pol == NULL) {
        kfree(elem);
        return false;
    }
    return true;
}

FieldElement GetIdentity(FiniteField f) {
    FieldElement element = init(f);
    if (element == NULL) return NULL;
    element->pol = IdentityPolynom(f->p);
    if (element->pol == NULL) {
        kfree(element);
        return NULL;
    }
    return element;
}

bool IsZero(FieldElement element) {
    return IsZeroPolynom(element->pol);
}

bool IsIdentity(FieldElement element) {
    return IsIdentityPolynom(element->pol);
}

FieldElement GetZero(FiniteField f) {
    FieldElement element = init(f);
    if (element == NULL) return NULL;
    element->pol = ZeroPolynom(f->p);
    if (element->pol == NULL) {
        kfree(element);
        return NULL;
    }

    return element;
}

//array coefficients are stored as big - endian
//array_size is the size of array
FieldElement GetFromArray(FiniteField f, int const *array, uint8_t array_size) {
    FieldElement element = init(f);
    if (element == NULL) return NULL;
    element->pol = PolynomFromArray(array, array_size, f->p);
    if (element->pol == NULL) {
        kfree(element);
        return NULL;
    }
    if (!descend(element)) {
        return NULL;
    }
    return element;
}

bool InSameField(FieldElement lhs, FieldElement rhs) {
    return AreEqualFields(lhs->field, rhs->field);
}

//returns NUll if error occurred
FieldElement Copy(FieldElement elem) {
    FieldElement res = init(elem->field);
    if (res != NULL) {
        res->pol = CopyPolynom(elem->pol);
    }
    return res;
}

void FreeElement(FieldElement elem) {
    if (elem != NULL) {
        FreePolynom(elem->pol);
    }
    kfree(elem);
}

//returns NULL if ERROR occurred
FieldElement Add(FieldElement lhs, FieldElement rhs) {
    if (!InSameField(lhs, rhs)) {
        return NULL;
    }
    FieldElement res = init(lhs->field);
    if (res == NULL) return NULL;
    res->pol = AddPolynom(lhs->pol, rhs->pol);
    if (res->pol == NULL) {
        kfree(res);
        return NULL;
    }
    return res;
}

FieldElement Mult(FieldElement lhs, FieldElement rhs) {
    if (!InSameField(lhs, rhs)) {
        return NULL;
    }
    FieldElement res = init(lhs->field);
    if (res == NULL) return NULL;
    res->pol = MultPolynom(lhs->pol, rhs->pol);
    if (res->pol == NULL) {
        kfree(res);
        return NULL;
    }
    if (!descend(res)) {
        return NULL;
    }
    return res;
}

static int int_fast_pow(int val, int pow) {
    if (val == 0) return 0;
    if (val == 1) return 1;
    int result = 1;
    while (pow > 0) {
        if (pow % 2 == 1) {
            result *= val;
        }
        val *= val;
        pow /= 2;
    }
    return result;
}

//p > 0
static FieldElement element_fast_pow(FieldElement elem, unsigned int p) {
    if (IsZeroPolynom(elem->pol)) {
        return GetZero(elem->field);
    }
    if (IsIdentityPolynom(elem->pol)) {
        return GetIdentity(elem->field);
    }
    FieldElement res = GetIdentity(elem->field);
    FieldElement value = Copy(elem);
    if (res == NULL || value == NULL) {
        FreeElement(res);
        FreeElement(value);
        return NULL;
    }
    while (p > 0) {
        if (p % 2 == 1) {
            FieldElement dummy = res;
            res = Mult(res, value);
            FreeElement(dummy);
            if (res == NULL) return NULL;
        }
        FieldElement dummy = value;
        value = Mult(value, value);
        FreeElement(dummy);
        if (value == NULL) return NULL;
        p /= 2;
    }
    FreeElement(value);
    return res;
}

FieldElement Inv(FieldElement element) {
    if (IsZero(element)) return NULL;
    uint64_t pn = element->field->pol == NULL ? element->field->p - 1 : int_fast_pow(element->field->p,
                                                                                     PolynomDeg(element->field->pol));
    return element_fast_pow(element, pn - 2);
}

FieldElement Pow(FieldElement elem, int p) {
    if (p < 0) {
        FieldElement tmp = Inv(elem);
        FieldElement res = element_fast_pow(tmp, -p);
        FreeElement(tmp);
        return res;
    }
    return element_fast_pow(elem, p);
}

FieldElement Division(FieldElement lhs, FieldElement rhs) {
    if (IsZeroPolynom(rhs->pol)) return NULL;
    FieldElement tmp = Inv(rhs);
    if (tmp == NULL) return NULL;
    FieldElement res = Mult(lhs, tmp);
    FreeElement(tmp);
    return res;
}

//returns NULL if error occurred
FieldElement Neg(FieldElement elem) {
    FieldElement res = init(elem->field);
    if (res == NULL) return NULL;
    res->pol = NegPolynom(elem->pol);
    return res;
}

//returns NULL if error occurred
FieldElement Sub(FieldElement lhs, FieldElement rhs) {
    FieldElement tmp = Neg(rhs);
    if (tmp != NULL) {
        FieldElement res = Add(lhs, tmp);
        FreeElement(tmp);
        return res;
    }
    return NULL;
}

bool AreEqual(FieldElement lhs, FieldElement rhs) {
    return InSameField(lhs, rhs) && AreEqualPolynom(lhs->pol, rhs->pol);
}
