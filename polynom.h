#ifndef FINITEFIELDSHW_POLYNOM_H
#define FINITEFIELDSHW_POLYNOM_H

#include <linux/types.h>

//polynom with coefficients from F_p
struct Polynom {
    uint8_t p;
    uint8_t coeff_size;
    uint8_t *coefficients; // little - endian
};
typedef struct Polynom *Polynom;

Polynom AddPolynom(Polynom lhs, Polynom rhs);

Polynom SubPolynom(Polynom lhs, Polynom rhs);

Polynom MultPolynom(Polynom lhs, Polynom rhs);

Polynom NegPolynom(Polynom elem);

Polynom ModPolynom(Polynom lhs, Polynom rhs);

Polynom IdentityPolynom(uint8_t p);

Polynom ZeroPolynom(uint8_t p);

Polynom PolynomFromArray(int const *array, uint8_t array_size, uint8_t p);

Polynom CopyPolynom(Polynom elem);

uint8_t PolynomDeg(Polynom elem);

bool AreEqualPolynom(Polynom lhs, Polynom rhs);

bool IsZeroPolynom(Polynom pol);

bool IsIdentityPolynom(Polynom pol);

void FreePolynom(Polynom elem);

#endif //FINITEFIELDSHW_POLYNOM_H
