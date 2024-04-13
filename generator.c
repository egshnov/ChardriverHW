#include "generator.h"
#include <linux/fs.h>
int setup_generator(struct generator *gen)
{
    gen->k = 0;
    gen->a_i = NULL;
    gen->x_i = NULL;
    gen->c = NULL;
    int irreducible[] = {1,1,1,1,1,1,0,0, 1}; // x^8 + x^7 + x^6 + x^5 + x^4 + x^3 + 1
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

static void reset_generator(struct generator *gen){
    free_elem_buff_if_necessary(gen->a_i, gen->k);
    free_elem_buff_if_necessary(gen->x_i, gen->k);
    FreeElement(gen->c);
}

void free_generator(struct generator *gen)
{
    reset_generator(gen);
    FreeField(gen->field);
}

//TODO: error handling
//TODO: copy generator before executing?

int get_random(struct generator *gen, uint8_t *target)
{
    uint8_t k = gen->k;
    FieldElement x_n = GetZero(gen->field);
    if(x_n == NULL){
        return -1;
    }

    for(size_t i = 0; i < k; i++){
        FieldElement summand = Mult(gen->a_i[i], gen->x_i[i]);
        if(summand == NULL){
            FreeElement(x_n);
            return -1;
        }

        /* сдвигаем буфер на один элемент назад */
        if(i != 0){
            gen->x_i[i-1] = Copy(gen->x_i[i]);
        }

        FreeElement(gen->x_i[i]);
        /* сдвинули */

        FieldElement tmp_x_n = x_n;
        x_n = Add(summand, x_n);
        FreeElement(tmp_x_n);
        FreeElement(summand);

        if(x_n == NULL)
            return -1;

    }
    FieldElement tmp = x_n;
    x_n = Add(gen->c, x_n);
    FreeElement(tmp);
    gen->x_i[gen->k - 1] = Copy(x_n);

    if(x_n == NULL)
        return -1;

    *target = ToUint8(x_n);
    FreeElement(x_n);
    return 0;
}

int init_random(struct generator *gen, const char __user *buff, size_t len)
{
    //TODO: redundant copy ? можно сразу из user_space брать
    char* local_buffer =  (char *) kmalloc(sizeof(char) * len, GFP_KERNEL);

    /*copy_from_user returns number of bytes that could not be copied. On success, this will be zero. */
    if(copy_from_user(local_buffer,buff, len)){
        return -1;
    }

    reset_generator(gen);

    gen->k = local_buffer[0];
    gen->a_i = (FieldElement *) kmalloc(sizeof(FieldElement) * gen->k, GFP_KERNEL);

    if(gen->a_i == NULL) return -1;

    gen->x_i = (FieldElement *) kmalloc(sizeof(FieldElement) * gen->k, GFP_KERNEL);

    if(gen->x_i == NULL){
        free_elem_buff_if_necessary(gen->a_i,gen->k);
        return -1;
    }


    size_t k = gen->k;
    for(size_t i = 1; i <= k ; i++){
        uint8_t byte = local_buffer[i];
        gen->a_i[i-1] = FromUint8(gen->field, byte);

        if(gen->a_i[i-1] == NULL){
            free_elem_buff_if_necessary(gen->a_i,gen->k);
            return -1;
        }
    }

    for(size_t i = k + 1; i <= 2*k ; i++){
        uint8_t byte = local_buffer[i];
        gen->x_i[i-k-1] = FromUint8(gen->field,byte);

        if(gen->x_i[i-k-1] == NULL){
            free_elem_buff_if_necessary(gen->a_i,gen->k);
            free_elem_buff_if_necessary(gen->x_i, gen->k);
            return -1;
        }
    }

    gen->c = FromUint8(gen->field, local_buffer[2*k+1]);

    if(gen->c == NULL){
        reset_generator(gen);
        return -1;
    }

    kfree(local_buffer);

    return 0;
}
