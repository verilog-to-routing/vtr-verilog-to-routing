#ifndef SIMULATION_RAM_Hs
#define SIMULATION_RAM_H

#include <mutex>
#include <memory>
#include <vector>
#include "AtomicBuffer.hpp"

class AtomicRow
{
private:
    std::mutex word_lock;
    std::vector<AtomicBuffer*> data_row;

public:
    AtomicRow(size_t word_size, int8_t init_value)
    {
        for(size_t cell=0; cell< word_size; cell++)
            data_row.push_back(new AtomicBuffer(init_value));
    }

    AtomicRow(std::vector<AtomicBuffer*> prebuilt_row)
    {
        data_row = std::vector<AtomicBuffer*>(prebuilt_row.begin(), prebuilt_row.end());
    }

    ~AtomicRow()
    {
        for(size_t cell=0; cell< data_row.size(); cell++)
        {
            delete data_row[cell];
        }
    }

    bool empty()
    {
        return this->data_row.empty();
    }

    size_t size()
    {
        return this->data_row.size();
    }

    std::vector<int8_t> get_data_row(int64_t cycle)
    {
        std::vector<int8_t> word_line;

        for(size_t i=0; i< data_row.size(); i++)
            word_line.push_back(data_row[i]->get_value(cycle));

        return word_line;
    }

    void set_data_row(std::vector<int8_t> values, int64_t cycle)
    {
        for(size_t i=0; i< data_row.size(); i++)
            data_row[i]->update_value(values[i], cycle);
    }    

    void copy_row_foward_one_cycle(int64_t cycle)
    {
        for(size_t i=0; i< data_row.size(); i++)
            data_row[i]->copy_foward_one_cycle(cycle);
    }


    std::vector<int8_t> lock_free_get_data_row(int64_t cycle)
    {
        std::vector<int8_t> word_line;

        for(size_t i=0; i< data_row.size(); i++)
            word_line.push_back(data_row[i]->lock_free_get_value(cycle));

        return word_line;
    }

    void lock_free_set_data_row(std::vector<int8_t> values, int64_t cycle)
    {
        for(size_t i=0; i< data_row.size(); i++)
            data_row[i]->lock_free_update_value(values[i], cycle);
    }    

    void lock_free_copy_row_foward_one_cycle(int64_t cycle)
    {
        for(size_t i=0; i< data_row.size(); i++)
            data_row[i]->lock_free_copy_foward_one_cycle(cycle);
    }

    void init_line_values(std::vector<int8_t> values)
    {
        for(size_t i=0; i< data_row.size(); i++)
            data_row[i]->init_all_values(values[i]);
    }    
};

#endif
