#ifndef SIMULATION_RAM_H
#define SIMULATION_RAM_H

#include <mutex>
#include <memory>
#include <vector>
#include "AtomicRow.hpp"

class AtomicRam
{
private:
    std::mutex ramlock;
    std::vector<AtomicRow *> data_cells;


public:
    AtomicRam(size_t address_size, size_t word_size, int8_t init_value)
    {
        // compute the real depth from the adress space
        long long max_address = 1 << address_size;

        for(size_t address=0; address< max_address; address++)
        {
            data_cells.push_back(new AtomicRow(word_size, init_value));
        }
    }

    ~AtomicRam()
    {
        for(size_t address=0; address< data_cells.size(); address++)
        {
            delete data_cells[address];
        }
    }

    bool empty()
    {
        return this->data_cells.empty();
    }

    std::vector<int8_t> get_word_line(long long address, int64_t cycle)
    {
        if(address < 0 || address > data_cells.size())
            return std::vector<int8_t>(data_cells.size(), -1);

        std::lock_guard<std::mutex> lock(this->ramlock);
        return data_cells[address]->lock_free_get_data_row(cycle);
    }

    void set_word_line(long long address, std::vector<int8_t> values, int64_t cycle)
    {
        if(address < 0 || address > data_cells.size())
            return;

        std::lock_guard<std::mutex> lock(this->ramlock);
        data_cells[address]->lock_free_set_data_row(values, cycle);
    }   

    void init_word_line(long long address, char *values_in, int width)
    {
        if(address < 0 || address > data_cells.size())
            return;

        std::vector<int8_t> values;
        for(size_t i =0; i<data_cells[address]->size(); i++)
        {
            if(i< width)
                values.push_back(values_in[i]);
            else
                values.push_back(-1);
        }
        std::lock_guard<std::mutex> lock(this->ramlock);
        data_cells[address]->init_line_values(values);
    }   

    void copy_ram_foward_one_cycle(int64_t cycle)
    {
        std::lock_guard<std::mutex> lock(this->ramlock);
        for(size_t address=0; address< data_cells.size(); address++)
            data_cells[address]->lock_free_copy_row_foward_one_cycle(cycle);
    }
};

#endif
