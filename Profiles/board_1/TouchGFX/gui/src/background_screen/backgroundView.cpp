#include <gui/background_screen/backgroundView.hpp>

backgroundView::backgroundView() : lastHour(0xFF), lastMinute(0xFF), lastSecond(0xFF)
{

}

void backgroundView::setupScreen()
{
    backgroundViewBase::setupScreen();
}

void backgroundView::tearDownScreen()
{
    backgroundViewBase::tearDownScreen();
}

void backgroundView::update_time()
{
    uint8_t hour, minute, second;
    presenter->getTime(hour, minute, second);
    if (hour != lastHour || minute != lastMinute || second != lastSecond)
    {
        lastHour = hour;
        lastMinute = minute;
        lastSecond = second;
        digitalClock1.setTime24Hour(hour, minute, second);
    }
}
