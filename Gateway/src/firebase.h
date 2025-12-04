#ifndef FIREBASE_H
#define FIREBASE_H

#include <Arduino.h>

void sendToFirebase(float pm25, float humidity, float temperature, float gas);
void pushHistory(float pm25, float humidity, float temperature, float gas);

#endif