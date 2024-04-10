#include "binary_field_extension.h"
#include "field_element.h"

uint8_t ToUint8(FieldElement element) {
    uint8_t res = 0;
    uint8_t add = 1;
    for (size_t i = 0; i < element->pol->coeff_size; i++) {
        res += add * element->pol->coefficients[i];
        add = add << 1;
    }
    return res;
}

uint16_t ToUint16(FieldElement element) {
    uint16_t res = 0;
    uint16_t add = 1;
    for (size_t i = 0; i < element->pol->coeff_size; i++) {
        res += add * element->pol->coefficients[i];
        add = add << 1;
    }
    return res;
}


uint32_t ToUint32(FieldElement element) {
    uint32_t res = 0;
    uint32_t add = 1;
    for (size_t i = 0; i < element->pol->coeff_size; i++) {
        res += add * element->pol->coefficients[i];
        add = add << 1;
    }
    return res;
}

// общую функцию по типу to_binary_array(int n, int len)
// писать не хочется т.к. используется type tmp и получаются
// касты которых хочется избежать, при этом не хочется дублировать код

#define uint_to_binary_array(type, n, target, count) ({ \
    type tmp = n;                                     \
    while(tmp > 0){                                     \
        tmp /= 2;                                       \
        count++;                                       \
    }                                                   \
    (target) = (int*)kmalloc((count)*sizeof(int),GFP_KERNEL);     \
    if(target != NULL){                                 \
        for(int i = (count) - 1; i >= 0; i--){            \
            (target)[i] = (n) % 2;                          \
            (n) /= 2;                                     \
        }                                               \
    }})


FieldElement FromUint8(FiniteField f, uint8_t binary) {
    uint64_t count = binary == 0 ? 1 : 0;
    int *target;
    uint_to_binary_array(uint8_t, binary, target, count);
    FieldElement elem = GetFromArray(f, target, count);
    kfree(target);
    return elem;
}

FieldElement FromUint16(FiniteField f, uint16_t binary) {
    uint64_t count = binary == 0 ? 1 : 0;
    int *target;
    uint_to_binary_array(uint16_t, binary, target, count);
    FieldElement elem = GetFromArray(f, target, count);
    kfree(target);
    return elem;
}

FieldElement FromUint32(FiniteField f, uint32_t binary) {
    uint64_t count = binary == 0 ? 1 : 0;
    int *target;
    uint_to_binary_array(uint32_t, binary, target, count);
    FieldElement elem = GetFromArray(f, target, count);
    kfree(target);
    return elem;
}
