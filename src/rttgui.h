/***************************************************************************//**
 * @file    rttgui.h
 * @brief   Arduino RT-Thread GUI library header
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#ifndef __RTTGUI_H__
#define __RTTGUI_H__

extern "C" {
    #include "include/rtgui.h"
    #include "include/images/image.h"
    #include "include/widgets/container.h"
    #include "include/widgets/label.h"
    #include "include/widgets/button.h"
    #include "include/widgets/window.h"
    #include "include/app/app.h"
}

class RT_GUI {
 public:
    void begin(void);

 // private:
};

extern RT_GUI RTT_GUI;

#endif /*__RTTGUI_H__ */
