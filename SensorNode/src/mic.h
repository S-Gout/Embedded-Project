#ifndef MIC_H
#define MIC_H

#include <Arduino.h>

// Call once in setup()
void micInit();

// Call repeatedly in loop()
void micUpdate();

#endif
