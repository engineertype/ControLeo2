// Written by Peter Easton
// Released under WTFPL license
//
// Change History:
// 14 August 2014        Initial Version

#ifndef CONTROLEO2_MAX31855_H
#define CONTROLEO2_MAX31855_H

#include "Arduino.h"

#define	FAULT_OPEN		10000
#define	FAULT_SHORT_GND	10001
#define	FAULT_SHORT_VCC	10002

enum	unit_t
{
	CELSIUS,
	FAHRENHEIT
};

class	ControLeo2_MAX31855
{
public:
    ControLeo2_MAX31855(void);
	
    double	readThermocouple(unit_t	unit);
    double	readJunction(unit_t	unit);
    
private:
    unsigned long readData();
};
#endif  // CONTROLEO2_MAX31855_H