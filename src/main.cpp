#include <Arduino.h>

#include "Config.hpp"
#include "SMS.hpp"
#include "Application.hpp"

static Application app;

void setup() {
	app.Init();
}

void loop() {
	app.Update();
}