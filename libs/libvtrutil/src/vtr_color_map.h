#ifndef VTR_CMAP_H
#define VTR_CMAP_H
#include <vector>

namespace vtr {

///@brief A container to save the rgb components of a color
template<class T>
struct Color {
    T r;
    T g;
    T b;
};

///@brief A class that holds a complete color map
class ColorMap {
  public:
    ///@brief color map constructor
    ColorMap(float min, float max, const std::vector<Color<float>>& color_data);

    ///@brief color map destructor
    virtual ~ColorMap() = default;

    ///@brief Returns the full color corresponding to the input value
    Color<float> color(float value) const;

    ///@brief Return the min Color of this color map
    float min() const;

    ///@brief Return the max color of this color map
    float max() const;

    ///@brief Return the range of the color map
    float range() const;

  private:
    float min_;
    float max_;
    std::vector<Color<float>> color_data_;
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

} // namespace vtr
#endif
