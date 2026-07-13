#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    /* Current wall-clock time (24-hour). Reads the RTC on target, the host
     * clock in the simulator. */
    void getTime(uint8_t& hour, uint8_t& minute, uint8_t& second) const;
protected:
    ModelListener* modelListener;
};

#endif // MODEL_HPP
