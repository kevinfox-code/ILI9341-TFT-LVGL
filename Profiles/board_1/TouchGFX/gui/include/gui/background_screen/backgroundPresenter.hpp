#ifndef BACKGROUNDPRESENTER_HPP
#define BACKGROUNDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class backgroundView;

class backgroundPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    backgroundPresenter(backgroundView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    void getTime(uint8_t& hour, uint8_t& minute, uint8_t& second) const
    {
        model->getTime(hour, minute, second);
    }

    virtual ~backgroundPresenter() {}

private:
    backgroundPresenter();

    backgroundView& view;
};

#endif // BACKGROUNDPRESENTER_HPP
