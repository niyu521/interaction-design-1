//// filepath:
////home/iwasakim/Documents/PlatformIO/Projects/interaction-design-1/src/main.cpp
#include "imu.hpp"
#include "slot_machine.hpp"
#include <M5Unified.h>

static IMU myIMU;
static SlotMachine slot(myIMU);

void setup() { slot.begin(); }

void loop() { slot.update(); }