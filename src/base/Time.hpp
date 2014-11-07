#ifndef Time_HPP
#define Time_HPP
#include <cmath>

class Time {
    public:
        //Initialize from constant float types
        Time(): time_(NAN) {}
        Time(double time): time_(time) {}

        float value() {return time_;}
        void set_value(float time) {time_ = time;}
    private:
        float time_;
};

#endif
