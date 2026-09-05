#pragma once

#include <algorithm>

namespace RegShare {

// Physical pixels throughout, including negative desktop origins.
struct PixelRect {
  int left = 0, top = 0, right = 0, bottom = 0;
  int Width() const { return std::max(0, right - left); }
  int Height() const { return std::max(0, bottom - top); }
};

struct RegionCrop {
  PixelRect source;
  int destination_x = 0, destination_y = 0;
  bool Empty() const { return source.Width() == 0 || source.Height() == 0; }
};

inline bool Overlaps(PixelRect a, PixelRect b) {
  return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
}

inline RegionCrop ComputeCrop(PixelRect region, PixelRect monitor,
                              int texture_width, int texture_height) {
  const int left = std::max(region.left, monitor.left);
  const int top = std::max(region.top, monitor.top);
  const int right = std::min(region.right,
      monitor.left + std::min(monitor.Width(), std::max(0, texture_width)));
  const int bottom = std::min(region.bottom,
      monitor.top + std::min(monitor.Height(), std::max(0, texture_height)));
  if (right <= left || bottom <= top) return {};
  return {{left - monitor.left, top - monitor.top, right - monitor.left, bottom - monitor.top},
          left - region.left, top - region.top};
}

}  // namespace RegShare
