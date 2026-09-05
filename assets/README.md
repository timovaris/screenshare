# App Icon

`regionshare.svg` is the editable source for RegionShare's original icon: a green
selection frame with an outward share arrow. It is distributed under the project
license.

The checked-in PNG and multi-resolution ICO are generated from this SVG. They are
included so normal Windows builds do not require an image conversion dependency.
To regenerate using ImageMagick:

```sh
convert -background none assets/regionshare.svg -depth 8 assets/regionshare.png
convert -background none assets/regionshare.svg -depth 8 -define icon:auto-resize=256,128,64,48,32,24,16 assets/regionshare.ico
```
