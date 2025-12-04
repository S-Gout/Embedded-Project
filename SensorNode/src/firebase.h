#ifndef FIREBASE_H
#define FIREBASE_H

#include <Arduino.h>

void sendToFirebase(float pm25, float hum);
void pushHistory(float pm25, float humidity, float temperature);

#endif