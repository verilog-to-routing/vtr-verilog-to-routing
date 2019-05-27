#ifndef VTR_CMAP_H
#define VTR_CMAP_H
#include <vector>

namespace vtr {

template<class T>
struct Color {
    T r;
    T g;
    T b;
};

class ColorMap {
  public:
    ColorMap(float min, float max, const std::vector<Color<float>>& color_data);
    virtual ~ColorMap() = default;
    Color<float> color(float value) const;
    float min() const;
    float max() const;
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
