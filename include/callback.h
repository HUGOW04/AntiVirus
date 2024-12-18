#ifndef CALLBACK_H
#define CALLBACK_H

#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <thread>

extern double mouseX, mouseY;
extern const GLuint WIDTH, HEIGHT;

extern bool scan;
extern bool network;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

#endif