# RTT-GUI

This project is forked from [RT-Thread Package -- GUI Engine](https://github.com/RT-Thread-packages/gui_engine) (an open source embedded GUI framework) and heavily refactored. 


## Dependence

* [RT-Thread Library](https://github.com/onelife/Arduino_RT-Thread)
  - Enable GUI, "CONFIG_USING_GUI" in "rtconfig.h"
  - Enable LCD driver, e.g. "CONFIG_USING_ILI" in "rtconfig.h"
  - Enable touch screen driver, e.g. "CONFIG_USING_FT6206" in "rtconfig.h"
  - Enable SD driver, "CONFIG_USING_SPISD" in "rtconfig.h"

* [ChaN's TJpgDec](http://www.elm-chan.org/fsw/tjpgd/00index.html)
  - This is a tiny JPEG decoder developed by ChaN
  - To enable, set "GUIENGINE_IMAGE_JPEG" in "guiconfig.h"

* [Lode Vandevenne's LodePNG](http://lodev.org/lodepng/)
  - PNG decoder developed by Lode Vandevenne
  - Encoder is disabled currently
  - No dependency or linkage to zlib or libpng required
  - Made for C (ISO C90) and has C++ wrapper


## Available Widgets ##

| Widget | Super Class | Remark |
| --- | --- | --- |
| Widget | Object | The base class |
| Container | Widget | A container (as parent) contains other widgets (as children) |
| Box (Sizer) | Object | Attached to container to define the layout of its children |
| Window | Container | |
| Title (Bar) | Widget | The title bar of a window |
| Label | Widget | |
| Button | Label | |
| Progress (Bar) | Widget | |
| List | Widget | |
| File List | List | |


## Available Image Formats ##

| Format | Dependence | Remark |
| --- | --- | --- |
| XPM | | RTT-GUI internal image format |
| BMP | | Support format: 1bpp (bit per pixel), 2bpp (not tested yet), 4bpp, 8bpp, 16bpp (RGB565) and 24bpp (RGB888)<br>Support scale ratio: 0 (no change) to 1/1024 |
| JPEG | [ChaN's TJpgDec](http://www.elm-chan.org/fsw/tjpgd/00index.html) | Support scale ratio: 0 (no change), 1/2, 1/4 and 1/8 |
| PNG | [Lode Vandevenne's LodePNG](http://lodev.org/lodepng/) | |


## Available Fonts ##

| Format | Remark |
| --- | --- |
| ASCII12 | |
| ASCII16 | |
| HZ12 | Simplified Chinese |
| HZ16 | Simplified Chinese |

The HZ12 and HZ16 are too huge to load to memory, so please load them from SD card instead by: Enable "CONFIG_USING_FONT_FILE" in "guiconfig.h" and copy [hz12.fnt and hz16.fnt](./bin/font) to SD `\font\` directory.

May do the same for ASCII12 and ASCII16 to save memory.


## License  ##

| Component | License |
| --- | --- |
| RTT-GUI | GNU Lesser General Public License v2.1 |
| RT-Thread core | Apache License 2.0 |
| TJpgDec | BSD like License |
| LodePNG | zlib License |
