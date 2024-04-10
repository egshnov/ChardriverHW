
#ifndef FINITEFIELDSHW_BINARY_FIELD_EXTENSION_H
#define FINITEFIELDSHW_BINARY_FIELD_EXTENSION_H

#include "finite_field.h"
#include "field_element.h"

uint8_t ToUint8(FieldElement element);

uint16_t ToUint16(FieldElement element);

uint32_t ToUint32(FieldElement element);

FieldElement FromUint8(FiniteField f, uint8_t binary);

FieldElement FromUint16(FiniteField f, uint16_t binary);

FieldElement FromUint32(FiniteField f, uint32_t binary);

#endif //FINITEFIELDSHW_BINARY_FIELD_EXTENSION_H
