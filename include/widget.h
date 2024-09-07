#ifndef WIDGET_H
#define WIDGET_H

#include <algorithm>  // For std::min
#include <cmath>      // For std::cos, std::sin
#include <GL/glut.h> // Include GLUT for rendering text
#include <string>
#include <vector>

#define PI 3.14159265358979323846

class Widget {
public:
    Widget(float px, float py, float pwidth, float pheight, float pradius, const float pcolor[3], const std::string& ptext = "");
    void render() const;
    void setText(const std::string& newText);

private:
    float x, y, width, height, radius;
    float color[3];
    std::string text; // New member variable for text

    void drawRoundedRect(float x, float y, float width, float height, float radius) const;
    void drawArc(float cx, float cy, float radius, float startAngle, float endAngle) const;
    void drawText(float x, float y, const std::string& text) const;
};

#endif // WIDGET_H
