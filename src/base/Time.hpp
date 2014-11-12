#ifndef Time_HPP
#define Time_HPP

#include <cmath>
#include <iosfwd>

class Time {
    public:
        //Initialize from constant float types
        Time(): time_(NAN) {}
        explicit Time(const double time): time_(time) {}

        float value() const {return time_;}
        void set_value(float time) {time_ = time;}
    private:
        float time_;
};

std::ostream& operator<<(std::ostream& os, const Time& time);


#endif
