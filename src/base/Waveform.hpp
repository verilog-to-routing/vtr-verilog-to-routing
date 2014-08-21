#ifndef Waveform_HPP
#define Waveform_HPP
#include "Time.hpp"

//Propagation info
//  This describes the simplified view of a
//  signal waveform.
class Waveform {
    public:
        explicit Waveform(Time new_time, Slew new_slew)
            : time_(new_time)
            , slew_(new_slew)
            {}

        const Time& time() const {return time_;}
        const Slew& slew() const {return slew_;}

        void set_time(const Time& new_time) {time_ = new_time;}
        void set_slew(const Slew& new_slew) {slew_ = new_slew;}

    private:
        Time time_; //Time the signal has stabalized
        Slew slew_; //Time the signal takes to slew
};

#endif
