#ifndef FINITFIELDSHW_FINITE_FIELD_H
#define FINITFIELDSHW_FINITE_FIELD_H

#include <linux/types.h>
#include <linux/slab.h>
#include "polynom.h"

struct FiniteField {
    uint8_t p;
    Polynom pol; //irreducible, mult and division operations are performed modulo polynom
};
typedef struct FiniteField *FiniteField;

FiniteField CreateF_p(uint8_t p);

// deg_polynom - polynom deg, stored deg_polynom+1
FiniteField CreateF_q(uint8_t p, uint8_t deg_polynom, int const *polynom);

bool AreEqualFields(FiniteField lhs, FiniteField rhs);

void FreeField(FiniteField f);

#endif //FINITFIELDSHW_FINITE_FIELD_H
