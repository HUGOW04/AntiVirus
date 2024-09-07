#include "widget.h"



Widget::Widget(float px, float py, float pwidth, float pheight, float pradius, const float pcolor[3], const std::string& ptext)
    : x(px), y(py), width(pwidth), height(pheight), radius(pradius), text(ptext) {
    color[0] = pcolor[0];
    color[1] = pcolor[1];
    color[2] = pcolor[2];
}

// Helper function to calculate the width of a string of text
float calculateTextWidth(const std::string& text) {
    return text.size() * 9.0f; // Approximate width per character
}

// Helper function to split text into lines
std::vector<std::string> wrapText(const std::string& text, float maxWidth) {
    std::vector<std::string> lines;
    std::string currentLine;
    float lineWidth = 0.0f;

    for (char c : text) {
        float charWidth = 9.0f; // Approximate width per character

        if (lineWidth + charWidth > maxWidth) {
            lines.push_back(currentLine);
            currentLine.clear();
            lineWidth = 0.0f;
        }

        currentLine += c;
        lineWidth += charWidth;
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}


void Widget::render() const {
    glColor3f(color[0], color[1], color[2]); // Set the rectangle color
    drawRoundedRect(x, y, width, height, radius);

    if (!text.empty()) {
        glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white

        const float maxTextWidth = 700.0f;
        const float textStartX = x + 10.0f;
        const float lineHeight = 20.0f; // Space between lines

        // Start from the highest point -10 pixels down from the top of the widget
        const float textStartY = y + height - 20.0f;

        std::vector<std::string> lines = wrapText(text, maxTextWidth);
        float currentY = textStartY;

        for (const std::string& line : lines) {
            drawText(textStartX, currentY, line);
            currentY -= lineHeight;
        }
    }
}


void Widget::setText(const std::string& newText) {
    text = newText;
}

void Widget::drawRoundedRect(float x, float y, float width, float height, float radius) const {
    // Ensure radius does not exceed half of the width or height
    radius = std::min(radius, std::min(width / 2.0f, height / 2.0f));

    // Draw the main rectangle
    glBegin(GL_QUADS);
    glVertex2f(x, y + radius);
    glVertex2f(x + width, y + radius);
    glVertex2f(x + width, y + height - radius);
    glVertex2f(x, y + height - radius);
    glEnd();

    // Draw the top and bottom rectangles
    glBegin(GL_QUADS);
    glVertex2f(x + radius, y);
    glVertex2f(x + width - radius, y);
    glVertex2f(x + width - radius, y + height);
    glVertex2f(x + radius, y + height);
    glEnd();

    // Draw the rounded corners
    drawArc(x + radius, y + radius, radius, 180, 270); // Bottom-left corner
    drawArc(x + width - radius, y + radius, radius, 270, 360); // Bottom-right corner
    drawArc(x + width - radius, y + height - radius, radius, 0, 90); // Top-right corner
    drawArc(x + radius, y + height - radius, radius, 90, 180); // Top-left corner
}

void Widget::drawArc(float cx, float cy, float radius, float startAngle, float endAngle) const {
    const int segments = 20;  // Number of segments to approximate the arc
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy); // Center of the arc
    for (int i = 0; i <= segments; ++i) {
        float angle = startAngle + (endAngle - startAngle) * i / segments;
        float xPos = cx + radius * cos(angle * PI / 180.0f);
        float yPos = cy + radius * sin(angle * PI / 180.0f);
        glVertex2f(xPos, yPos);
    }
    glEnd();
}

void Widget::drawText(float x, float y, const std::string& text) const {
    // Set the text color and position
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}