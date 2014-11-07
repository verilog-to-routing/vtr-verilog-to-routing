#ifndef Time_HPP
#define Time_HPP

class Time {
    public:
        //Initialize from constant float types
        explicit Time(double time): time_(time) {}

        float value() {return time_;}
        void set_value(float time) {time_ = time;}
    private:
        float time_;
};

#endif
