#ifndef CHAN_WIDTH_H
#define CHAN_WIDTH_H

#include <vector>

struct t_chan_width {
    int max = 0;
    int x_max = 0;
    int y_max = 0;
    int x_min = 0;
    int y_min = 0;
    std::vector<int> x_list;
    std::vector<int> y_list;
};

#endif