#include "RegionModel/RegionModel.h"
#include <cstdlib>
#include <iostream>

using namespace RegShare;
void Check(bool value, const char* message) {
  if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
  auto c = ComputeCrop({100, 200, 1380, 920}, {0, 0, 1920, 1080}, 1920, 1080);
  Check(c.source.left == 100 && c.source.top == 200 && c.source.Width() == 1280 &&
        c.source.Height() == 720, "Physical pixels must not apply DPI twice");
  c = ComputeCrop({-1800, -900, -520, -180}, {-1920, -1080, 0, 0}, 1920, 1080);
  Check(c.source.left == 120 && c.source.top == 180 && c.destination_x == 0,
        "Negative monitor origins");
  c = ComputeCrop({-100, -50, 500, 300}, {0, 0, 1920, 1080}, 1920, 1080);
  Check(c.source.Width() == 500 && c.source.Height() == 300 &&
        c.destination_x == 100 && c.destination_y == 50, "Cross-monitor padding");
  c = ComputeCrop({1800, 900, 2200, 1300}, {0, 0, 1920, 1080}, 1900, 1000);
  Check(c.source.Width() == 100 && c.source.Height() == 100, "Texture resize bounds");
  Check(ComputeCrop({2000, 0, 2100, 100}, {0, 0, 1920, 1080}, 1920, 1080).Empty(),
        "Disjoint region");
  Check(ComputeCrop({0, 0, 100, 100}, {0, 0, 1920, 1080}, 0, 0).Empty(), "Empty texture");
  Check(!Overlaps({0, 0, 100, 100}, {100, 0, 200, 100}), "Touching edges");
  Check(Overlaps({0, 0, 100, 100}, {99, 99, 200, 200}), "One-pixel overlap");
  std::cout << "RegionModel tests passed\n";
}
