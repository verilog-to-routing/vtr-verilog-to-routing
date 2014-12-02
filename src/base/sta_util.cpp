#include <ctime>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "sta_util.hpp"

float time_sec(struct timespec start, struct timespec end) {
    float time = end.tv_sec - start.tv_sec;

    time += (end.tv_nsec - start.tv_nsec) * 1e-9;
    return time;
}

void print_histogram(const std::vector<float>& values, int nbuckets) {
    nbuckets = std::min(values.size(), (size_t) nbuckets);
    int values_per_bucket = ceil((float) values.size() / nbuckets);

    std::vector<float> buckets(nbuckets);

    //Sum up each bucket
    for(size_t i = 0; i < values.size(); i++) {
        int ibucket = i / values_per_bucket;

        buckets[ibucket] += values[i];
    }

    //Normalize to get average value
    for(int i = 0; i < nbuckets; i++) {
        buckets[i] /= values_per_bucket;
    }

    float max_bucket_val = *std::max_element(buckets.begin(), buckets.end());

    //Print the histogram
    std::ios_base::fmtflags saved_flags = std::cout.flags();
    std::streamsize prec = std::cout.precision();
    std::streamsize width = std::cout.width();

    std::streamsize int_width = ceil(log10(values.size()));
    std::streamsize float_prec = 1;

    int histo_char_width = 60;

    //std::cout << "\t" << std::endl;
    for(int i = 0; i < nbuckets; i++) {
        std::cout << std::setw(int_width) << i*values_per_bucket << ":" << std::setw(int_width) << (i+1)*values_per_bucket - 1;
        std::cout << " " <<  std::scientific << std::setprecision(float_prec) << buckets[i];
        std::cout << " ";

        for(int j = 0; j < histo_char_width*(buckets[i]/max_bucket_val); j++) {
            std::cout << "*";
        }
        std::cout << std::endl;
    }

    std::cout.flags(saved_flags);
    std::cout.precision(prec);
    std::cout.width(width);
}
