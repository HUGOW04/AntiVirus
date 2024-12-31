#ifndef GUI_H
#define GUI_H

#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <cmath>

extern float colorOffset;

void renderDynamicProgressAnimation(float centerX, float centerY, float size, float progress, float time, bool isScanning);

// Function to render text
void renderText(const char* text, float x, float y);

// Function to render the diagonal gradient background
void renderBackground();

#endif