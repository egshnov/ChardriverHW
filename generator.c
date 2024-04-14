#include "generator.h"
#include <linux/fs.h>

int setup_generator(struct generator *gen)
{
    gen->k = 0;
    gen->a_i = NULL;
    gen->x_i = NULL;
    gen->c = NULL;
    int irreducible[] = {1,1,1,1,1,1,0,0,1}; // x^8 + x^7 + x^6 + x^5 + x^4 + x^3 + 1
    gen->field = CreateF_q(2, 8, irreducible);
    return -(gen->field == NULL);
}

static void free_elem_buff_if_necessary(FieldElement *buff, size_t size)
{
    if(buff != NULL){
        for(size_t i = 0; i < size; i++){
            FreeElement(buff[i]);
        }
        kfree(buff);
    }
}

void free_generator(struct generator *gen)
{
    free_elem_buff_if_necessary(gen->a_i, gen->k);
    free_elem_buff_if_necessary(gen->x_i, gen->k);
    FreeElement(gen->c);
    FreeField(gen->field);
}

#define __swap(T, x, y) \
    {                 \
        T obj = (x);  \
        (x) = (y);    \
        (y) = obj;    \
    }

int get_random(struct generator *gen, uint8_t *target)
{
    FieldElement x_n = GetZero(gen->field);
    if(x_n == NULL) return -1;
    FieldElement *tmp_x_i = (FieldElement *) kzalloc(sizeof(FieldElement)*gen->k, GFP_KERNEL);
    if(tmp_x_i == NULL){
        FreeElement(x_n);
        return -1;
    }
    uint8_t k = gen->k;

    for(size_t i = 0; i < k; i++){
        FieldElement summand = Mult(gen->a_i[i], gen->x_i[i]);

        if(summand == NULL){
            free_elem_buff_if_necessary(tmp_x_i, gen->k);
            FreeElement(x_n);
            return -1;
        }

        /* сдвигаем буфер на один элемент назад */
        if(i != 0){
            tmp_x_i[i-1] = Copy(gen->x_i[i]);
        }

        FieldElement tmp_x_n = x_n;
        x_n = Add(summand, x_n);
        FreeElement(tmp_x_n);
        FreeElement(summand);

        if(x_n == NULL) {
            free_elem_buff_if_necessary(tmp_x_i, gen->k);
            return -1;
        }
    }
    FieldElement tmp = x_n;
    x_n = Add(gen->c, x_n);
    FreeElement(tmp);

    if(x_n == NULL){
        free_elem_buff_if_necessary(tmp_x_i, gen->k);
        return -1;
    }
    tmp_x_i[gen->k - 1] = x_n;
    *target = ToUint8(x_n);

    __swap(FieldElement* ,tmp_x_i, gen->x_i);
    free_elem_buff_if_necessary(tmp_x_i, gen->k);

    return 0;
}

static int alloc_buffers(FieldElement **a_i, FieldElement **x_i, uint8_t k){
    *a_i = (FieldElement *) kzalloc(sizeof(FieldElement) * k, GFP_KERNEL);
    if(*a_i == NULL) return -1;
    *x_i = (FieldElement *) kzalloc(sizeof(FieldElement) * k, GFP_KERNEL);
    if(*x_i == NULL){
        free_elem_buff_if_necessary(*a_i, k);
        return -1;
    }
    return 0;
}

static int dealloc_buffers(FieldElement * a_i, FieldElement *x_i, uint8_t k) {
    free_elem_buff_if_necessary(a_i, k);
    free_elem_buff_if_necessary(x_i, k);
    return -1;
}


int init_random(struct generator *main_gen, const char __user *buff, size_t len)
{
    uint8_t k;
    if(get_user(k,buff)) return -1;
    FieldElement *tmp_a_i, *tmp_x_i;
    if(alloc_buffers(&tmp_a_i, &tmp_x_i, k) < 0) return -1;

    for(size_t i = 1; i <= k ; i++){
        uint8_t a,x;
        if(get_user(a, buff + i) || get_user(x, buff + i + k)) return dealloc_buffers(tmp_a_i, tmp_x_i, k);

        tmp_a_i[i-1] = FromUint8(main_gen->field, a);
        tmp_x_i[i-1] = FromUint8(main_gen->field, x);

        if(tmp_a_i[i-1] == NULL || tmp_x_i[i-1] == NULL) return dealloc_buffers(tmp_a_i, tmp_x_i, k);
    }

    uint8_t c;
    if(get_user(c, buff + 2*k + 1)) return dealloc_buffers(tmp_a_i, tmp_x_i, k);

    FieldElement tmp_c = FromUint8(main_gen->field, c);

    if(tmp_c == NULL) return dealloc_buffers(tmp_a_i, tmp_x_i, k);

    __swap(FieldElement *, tmp_a_i, main_gen->a_i)
    free_elem_buff_if_necessary(tmp_a_i,main_gen->k);

    __swap(FieldElement *, tmp_x_i, main_gen->x_i)
    free_elem_buff_if_necessary(tmp_x_i, main_gen->k);

    main_gen->k = k;

    __swap(FieldElement, tmp_c, main_gen->c)
    FreeElement(tmp_c);

    return 0;
}
