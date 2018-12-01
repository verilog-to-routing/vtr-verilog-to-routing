#ifndef SIMULATION_BUFFER_H
#define SIMULATION_BUFFER_H

#include <atomic>
#include <bitset>
#include <mutex>

/**
 * DO NOT CHANGE THESE
 * we use a single 64 bit integer to store al the value and we bit mask into it
 * 2 bits per value gives us a buffer of 32
 * 
 */
#define BUFFER_SIZE         16
#define CONCURENCY_LIMIT    (BUFFER_SIZE-2) // access to cycle -1 with an extra pdding cell
#define DATA_TYPE_BIT_SIZE  2
#define BUFFER_BIT_SIZE     (BUFFER_SIZE*DATA_TYPE_BIT_SIZE)


/**
 * we only need 2 bits to store the 4 possible values
 */
#define INT_0   0x0
#define INT_1   0x1
#define INT_X   0x2

/**
 * Odin use -1 internally, so we need to go back on forth
 * TODO: change the default odin value to match both the Blif value and this buffer
 */
typedef signed char data_t;

typedef std::bitset<DATA_TYPE_BIT_SIZE> return_t;

class AtomicBuffer
{
private:
    std::mutex bitlock;
    std::bitset<BUFFER_BIT_SIZE> buffer;
    std::atomic<int64_t> cycle;

    uint64_t mod_cycle(int64_t cycle_in)
    {
        // buffer is length of 32 and each internal 'bits' need 2 bit
        return ((cycle_in & (BUFFER_SIZE-1)) *DATA_TYPE_BIT_SIZE); 
    }

    void set_bits(return_t value, uint8_t bit_index, bool lock_it)
    {
        if(lock_it)
            std::lock_guard<std::mutex> lock(this->bitlock);

        for(size_t i=0; i<DATA_TYPE_BIT_SIZE; i++)
            this->buffer[bit_index+i] = value[i];
    }

    return_t get_bits(uint8_t bit_index, bool lock_it)
    {
        if(lock_it)
            std::lock_guard<std::mutex> lock(this->bitlock);

        return_t to_return = 0;
        for(size_t i=0; i<DATA_TYPE_BIT_SIZE; i++)
            to_return[i] = this->buffer[bit_index+i];

        return to_return;
    }

    void copy_foward(uint8_t bit_index, bool lock_it)
    {
        if(lock_it)
            std::lock_guard<std::mutex> lock(this->bitlock);

        for(size_t i=0; i<DATA_TYPE_BIT_SIZE; i++)
            this->buffer[bit_index+DATA_TYPE_BIT_SIZE+i] = this->buffer[bit_index+i];
    }

    return_t to_bit(data_t value_in)
    {
        if(value_in == 0)         return INT_0;
        else if(value_in == 1)    return INT_1;
        else                      return INT_X;
    }

    data_t to_value(return_t bit_in)
    {
        if(bit_in == 0)         return 0;
        else if(bit_in == 1)    return 1;
        else                    return -1;
    }

public:

    void init_all_values(data_t value)
    {
        return_t value_to_set = to_bit(value);
        for(int i=0; i<BUFFER_SIZE; i++)
            this->set_bits(value_to_set, this->mod_cycle(i),false);
    }

    AtomicBuffer()
    {
        this->update_cycle(-1);
    }

    AtomicBuffer(data_t value_in)
    {
        this->update_cycle(-1);
        this->init_all_values(value_in);
    }

    int64_t get_cycle()
    {
        return this->cycle;
    }

    void update_cycle(int64_t cycle_in)
    {
        this->cycle = cycle_in;
    }

    void print()
    {
        printf("\t");
        for(int i=0; i<BUFFER_SIZE; i++)
        {
            if(!(i%4))
                printf(" ");

            data_t value = this->get_value(i);
            if(value == -1)
                printf("x");
            else
                printf("%d", value);
            

        }
        
        printf("\n");
    }

    data_t lock_free_get_value(int64_t cycle_in)
    {
        return this->to_value(this->get_bits(mod_cycle(cycle_in),false));
    }

    data_t get_value(int64_t cycle_in)
    {
        return this->to_value(this->get_bits(mod_cycle(cycle_in),true));
    }

    void lock_free_update_value(data_t value_in, int64_t cycle_in)
    {
        this->set_bits(this->to_bit(value_in), mod_cycle(cycle_in),false);
        this->update_cycle(cycle_in);
    }

    void update_value(data_t value_in, int64_t cycle_in)
    {
        this->set_bits(this->to_bit(value_in), mod_cycle(cycle_in), true);
        this->update_cycle(cycle_in);
    }

    void lock_free_copy_foward_one_cycle(int64_t cycle_in)
    {
        this->copy_foward(mod_cycle(cycle_in), false);
        this->update_cycle(cycle_in);
    }

    void copy_foward_one_cycle(int64_t cycle_in)
    {
        this->copy_foward(mod_cycle(cycle_in), true);
        this->update_cycle(cycle_in);
    }
};

#endif