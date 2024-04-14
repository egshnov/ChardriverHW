#ifndef DRIVER_GENERATOR_H
#define DRIVER_GENERATOR_H
#include "finite_fields.h"

struct generator {
    uint8_t k;
    FieldElement *a_i;
    FieldElement *x_i;
    FieldElement c;
    FiniteField field;
};

int setup_generator(struct generator *gen);
void free_generator(struct generator *gen);
int get_random(struct generator *gen, uint8_t *target);
int init_random(struct generator *gen, const char __user *buff, size_t len);
#endif //DRIVER_GENERATOR_H
