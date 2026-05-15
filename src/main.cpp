#include <Arduino.h>

#include "Application.hpp"

static Application app;

void setup() {
	app.Init();
}

void loop() {
	app.Update();
}