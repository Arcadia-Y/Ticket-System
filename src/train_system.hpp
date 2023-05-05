// a subsystem to process train and order data
#ifndef TRAIN_SYSTEM_HPP
#define TRAIN_SYSTEM_HPP

#define MAXSTA 100
#include "Date.hpp"

namespace sjtu
{

struct Train_Data
{
    char stationNum;
    char type;
    Date startDate;
    Date endDate;
    Time startTime;
    short travelTimes[MAXSTA-1];
    short stopoverTimes[MAXSTA-2];
    char stations[MAXSTA][31];
    int seatNum[MAXSTA-1];
    int prices[MAXSTA-1];
};

class Train_System
{

};

} // namespace sjtu

#endif
