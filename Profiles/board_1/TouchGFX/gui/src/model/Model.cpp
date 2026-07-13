#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifdef SIMULATOR
#include <ctime>
#else
extern "C" uint32_t System_GetSecondsSinceMidnight(void);
#endif

Model::Model() : modelListener(0)
{

}

void Model::tick()
{

}

void Model::getTime(uint8_t& hour, uint8_t& minute, uint8_t& second) const
{
#ifdef SIMULATOR
    uint32_t secondsSinceMidnight = static_cast<uint32_t>(std::time(nullptr) % 86400);
#else
    uint32_t secondsSinceMidnight = System_GetSecondsSinceMidnight();
#endif
    hour = static_cast<uint8_t>(secondsSinceMidnight / 3600U);
    minute = static_cast<uint8_t>((secondsSinceMidnight % 3600U) / 60U);
    second = static_cast<uint8_t>(secondsSinceMidnight % 60U);
}
