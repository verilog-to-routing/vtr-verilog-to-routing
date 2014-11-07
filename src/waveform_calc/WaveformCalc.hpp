#ifndef WaveformCalc_HPP
#define WaveformCalc_HPP
#include <memory>

#include "Waveform.hpp"
class TimingEdge;
class TimingNode;

using std::unique_ptr;

//Abstract Waveform calculators
class WaveformCalc {
    public:
        virtual Waveform node_waveform(const TimingNode& node) = 0;
        virtual Waveform edge_waveform(const Waveform& input, const TimingEdge& edge) = 0;
};

//Concrete Waveform Calculator
class DelaySlew_WaveformCalc : public WaveformCalc {
    public:
        explicit DelaySlew_WaveformCalc(unique_ptr<NodeDelayCalc> node_delay_calc, 
                                        unique_ptr<NodeSlewCalc> node_slew_calc, 
                                        unique_ptr<EdgeDelayCalc> edge_delay_calc, 
                                        unique_ptr<EdgeSlewCalc> edge_slew_calc) {
            //Explcitly take ownership of the pointers
            node_delay_calc_ = std::move(node_delay_calc);
            node_slew_calc_ = std::move(node_slew_calc);
            edge_delay_calc_ = std::move(edge_delay_calc);
            edge_slew_calc_ = std::move(edge_slew_calc);
        }

        Waveform node_waveform(const TimingNode& node) override {
            return Waveform(node_delay_calc_->delay(node), node_slew_calc_->slew(node));
        }

        Waveform edge_waveform(const Waveform& input, const TimingEdge& edge) override {
            return Waveform(edge_delay_calc_->delay(input, edge), edge_slew_calc_->slew(input, edge));
        }

    private:
        unique_ptr<NodeDelayCalc> node_delay_calc_;
        unique_ptr<NodeSlewCalc> node_slew_calc_;
        unique_ptr<EdgeDelayCalc> edge_delay_calc_;
        unique_ptr<EdgeSlewCalc> edge_slew_calc_;
};

#endif
