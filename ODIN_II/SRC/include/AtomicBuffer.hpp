#ifndef SIMULATION_BUFFER_H
#define SIMULATION_BUFFER_H

#include <atomic>
#include <thread>
/**
 * Odin use -1 internally, so we need to go back on forth
 * TODO: change the default odin value to match both the Blif value and this buffer
 */
typedef signed char data_t;

#define BUFFER_SIZE         8   //use something divisible by 4 since the compact buffer can contain 4 value
#define CONCURENCY_LIMIT    (BUFFER_SIZE-1)   // access to cycle -1 with an extra pdding cell

#define VERI_LENGTH(type) ((sizeof(type)*8)/2) // 2 bits per verilog bit
typedef uint8_t bitfield_t;

class AtomicBuffer
{
private:
	bitfield_t bits[BUFFER_SIZE/VERI_LENGTH(bitfield_t)];
	std::atomic<bool> lock;
	int32_t my_cycle;
	
    uint8_t get_bitfield_size()
    {
        return VERI_LENGTH(bitfield_t);
    }

    void lock_it()
	{
        std::atomic_thread_fence(std::memory_order_acquire);
        while(lock.exchange(true, std::memory_order_relaxed))
        {
            std::this_thread::yield();
        }
	}

	void unlock_it()
	{
        lock.exchange(false, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_relaxed);
	}
	
	///////////////////////////
	// 2 bits are used per value
    // this converts back to a signed char type value
    data_t val(uint8_t val_in)
	{
        if(val_in == 0)
            return 0;
        else if(val_in == 1)
            return 1;
        else
            return -1;        
	}

    ///////////////////////////
	// 2 bits are used per value
    // this converts back to a unsigned char type value
    uint8_t val(data_t val_in)
	{
        if(val_in == 0)
            return 0x0;
        else if(val_in == 1)
            return 0x1;
        else
            return 0x3;        
	}
	
	uint8_t get_bits(uint8_t index)
	{
	    uint8_t address = index/get_bitfield_size();
	    uint8_t bit_index = index%get_bitfield_size();
        uint8_t real_index = static_cast<uint8_t>(bit_index*2);

        uint8_t bit_at = static_cast<uint8_t>(this->bits[address] >> real_index);
	    uint8_t mask = 0x3;

        bit_at &= mask; //mask the two first bit as a verilog bits

        return bit_at;
	}
	
	void set_bits(uint8_t index, uint8_t value)
	{
        uint8_t address = index/get_bitfield_size();
	    uint8_t bit_index = index%get_bitfield_size();
        uint8_t real_index = static_cast<uint8_t>(bit_index*2);
	    uint8_t mask = 0x3;

	    value &= mask;
        value = static_cast<uint8_t>(value << real_index);

        uint8_t zero_mask = static_cast<uint8_t>(mask << real_index);
        zero_mask = static_cast<uint8_t>(~(zero_mask));

        this->bits[address] &= zero_mask;
        this->bits[address] |= value;

	}

    uint8_t get_index_from_cycle(int64_t cycle)
    {
        return (uint8_t)(cycle%(BUFFER_SIZE));
    }

public:

    AtomicBuffer(data_t value_in)
    {
        this->lock = false;
        this->my_cycle = -1;
        this->init_all_values(value_in);
    }

    void print()
    {
        for(uint8_t i=0; i<BUFFER_SIZE; i++)
        {
            uint8_t value = get_bits( i);
            printf("%s",(value == 0)? "0": (value == 1)? "1": "x");
        }
        printf("\n");
    }

    void init_all_values( data_t value)
    {
    	this->lock = false;
        for(uint8_t i=0; i<BUFFER_SIZE; i++)
            set_bits( i, val(value));
    }

    int32_t lock_free_get_cycle()
    {
        return this->my_cycle;
    }

    void lock_free_update_cycle( int64_t cycle_in)
    {
        //if (cycle_in > this->cycle)
        this->my_cycle = (int32_t)cycle_in;
    }

    data_t lock_free_get_value( int64_t cycle_in)
    {
        uint8_t index = get_index_from_cycle(cycle_in);

        return val(get_bits(index));
    }

    void lock_free_update_value( data_t value_in, int64_t cycle_in)
    {
        if (cycle_in > this->my_cycle)
        {
            uint8_t index = get_index_from_cycle(cycle_in);

            set_bits(index, val(value_in));
            lock_free_update_cycle(cycle_in);
        }
    }

    void lock_free_copy_foward_one_cycle( int64_t cycle_in)
    {
        uint8_t index = get_index_from_cycle(cycle_in);
        uint8_t next_index = get_index_from_cycle(cycle_in+1);

        set_bits( get_index_from_cycle(next_index) ,get_bits(index));
        lock_free_update_cycle( cycle_in);
    }

    int32_t get_cycle()
    {
		lock_it();
		int32_t value = lock_free_get_cycle();
        unlock_it();
        return value;
    }

    void update_cycle( int64_t cycle_in)
    {
		lock_it();
        lock_free_update_cycle(cycle_in);
        unlock_it();
    }

    data_t get_value( int64_t cycle_in)
    {
		lock_it();
    	data_t value = lock_free_get_value( cycle_in);
        unlock_it();
        return value;
    }

    void update_value( data_t value_in, int64_t cycle_in)
    {
		lock_it();
        lock_free_update_value( value_in, cycle_in);
        unlock_it();

    }

    void copy_foward_one_cycle( int64_t cycle_in)
    {
		lock_it();
        lock_free_copy_foward_one_cycle( cycle_in);
        unlock_it();
    }
    
};

#endif
