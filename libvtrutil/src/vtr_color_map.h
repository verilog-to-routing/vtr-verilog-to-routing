#ifndef VTR_CMAP_H
#define VTR_CMAP_H
#include <vector>
#include <tuple>

namespace vtr {

class ColorMap {
    public:
        ColorMap(float min, float max, const std::vector<std::tuple<float,float,float>>& color_data);
        virtual ~ColorMap() = default;
        std::tuple<float,float,float> color(float value);
    private:
        float min_;
        float max_;
        std::vector<std::tuple<float,float,float>> color_data_;
};

class InfernoColorMap : public ColorMap {
    public:
        InfernoColorMap(float min, float max);
};

class PlasmaColorMap : public ColorMap {
    public:
        PlasmaColorMap(float min, float max);
};

class ViridisColorMap : public ColorMap {
    public:
        ViridisColorMap(float min, float max);
};

} //namespace
#endif
