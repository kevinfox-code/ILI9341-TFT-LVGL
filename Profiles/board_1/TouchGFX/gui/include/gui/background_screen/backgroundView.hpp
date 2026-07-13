#ifndef BACKGROUNDVIEW_HPP
#define BACKGROUNDVIEW_HPP

#include <cstdint>
#include <gui_generated/background_screen/backgroundViewBase.hpp>
#include <gui/background_screen/backgroundPresenter.hpp>

class backgroundView : public backgroundViewBase
{
public:
    backgroundView();
    virtual ~backgroundView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void update_time();
protected:
    uint8_t lastHour;
    uint8_t lastMinute;
    uint8_t lastSecond;
};

#endif // BACKGROUNDVIEW_HPP
