#ifndef BUTTON_H
#define BUTTON_H

#include <string.h>
#include <chrono>
#include <iostream>
#include <GL/glut.h> // Include GLUT for rendering text

const float RADIUS = 20.0f;  // Radius of the rounded corners
#define PI 3.14159265358979323846

class Button {
public:
    bool isHovered;
    float fadeAlpha;
    std::string id;
    std::string text;
    GLuint textureID;

    Button(float px, float py, float pwidth, float pheight, const std::string& pid, const std::string& ptext, const char* imagePath)
        : x(px), y(py), width(pwidth), height(pheight),
          isHovered(false), fadeAlpha(0.0f),
          id(pid), text(ptext), fadeStartTime(std::chrono::high_resolution_clock::now()) {
        loadTexture(imagePath);
    }


    void updateState(double mouseX, double mouseY, bool isCurrentHovered);
    void render(double mouseX, double mouseY) const;
    const std::string& getId() const { return id; }

    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

private:
    float x, y, width, height;
    std::chrono::time_point<std::chrono::high_resolution_clock> fadeStartTime;

    void drawRoundedRect(float x, float y, float width, float height, float radius) const;
    void drawArc(float cx, float cy, float radius, float startAngle, float endAngle) const;
    void drawText(float x, float y, const std::string& text) const;
    void loadTexture(const char* filename);
};

#endif