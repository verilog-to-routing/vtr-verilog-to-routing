#ifndef VTR_RANDOM_H
#define VTR_RANDOM_H
namespace vtr {
    /*********************** Portable random number generators *******************/
    void srandom(int seed);
    unsigned int get_current_random();
    int irand(int imax);
    float frand();
} //namespace
#endif
