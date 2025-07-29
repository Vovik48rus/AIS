#ifndef IMENU_H
#define IMENU_H

#include <MenuScreen.h>

class IMenu
{
public:
    virtual MenuScreen* getMenu() const = 0;
};

#endif // IMENU_H
