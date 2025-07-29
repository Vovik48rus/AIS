#ifndef CSMS_MODBUS_MENU_H
#define CSMS_MODBUS_MENU_H

#include <Arduino.h>
#include "CSMSModbus.h"
#include "IMenu.h"
#include <ItemRange.h>

class CSMSModbusMenu : public IMenu
{
    MenuScreen *menu = nullptr;
    CSMSModbus *sensor1 = nullptr;

private:
    /* data */
public:
    CSMSModbusMenu(CSMSModbus *sensor1)
        : sensor1(sensor1)
    {
        // Initialize the menu with CSMS Modbus sensors
        std::vector<MenuItem *> items;
        // items.push_back(new ItemRange<int, int>(
        //     "Dry Soil",
        //     sensor1->getDrySoilValue(),
        //     1, 0, 1023,
        //     "Dry Soil: %d",
        //     0, false,
        //     [this](int val){ this->sensor1->setDrySoilValue(val); }
        // ));
        // items.push_back(new ItemRange<int, int>());
    }

    MenuScreen *getMenu() const override
    {
        return menu;
    }
};

#endif // CSMS_MODBUS_MENU_H
