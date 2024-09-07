#include "button.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void Button::loadTexture(const char* filename) {
    if (filename == nullptr || std::string(filename).empty()) {       
        textureID = 0;  // Assign a default value or handle it appropriately in your code
        return;
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (image == nullptr) {
        std::cout << "Failed to load texture: " << filename << std::endl;
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);
}


void Button::drawText(float x, float y, const std::string& text) const {
    glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

void Button::updateState(double mouseX, double mouseY, bool isCurrentHovered) {
    bool currentlyHovered = mouseX >= x && mouseX <= x + width &&
        mouseY >= y && mouseY <= y + height;

    if (currentlyHovered) {
        isHovered = true;
        fadeAlpha = 0.5f;  // Reset fade alpha to maximum
        fadeStartTime = std::chrono::high_resolution_clock::now();  // Reset fade timer
    }
    else {
        if (isHovered) {
            // Button just lost hover, start fading out
            auto currentTime = std::chrono::high_resolution_clock::now();
            float elapsedTime = std::chrono::duration<float>(currentTime - fadeStartTime).count();
            float fadeDuration = 0.3f;  // Decreased fade duration to 0.3 seconds

            if (elapsedTime < fadeDuration) {
                fadeAlpha = 0.5f * (1.0f - elapsedTime / fadeDuration);
            }
            else {
                isHovered = false;
                fadeAlpha = 0.0f;
            }
        }
    }

    // Ensure only the current hovered button has non-zero fadeAlpha
    if (isCurrentHovered) {
        fadeAlpha = std::max(fadeAlpha, 0.5f);
    }
    else if (!currentlyHovered) {
        fadeAlpha = 0.0f;
    }
}

void Button::render(double mouseX, double mouseY) const {
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Draw textured button with rounded edges, stretching to edges
        glColor3f(1.0f, 1.0f, 1.0f); // White color to show texture as-is
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height);
        glEnd();

        // Draw rounded corners
        drawArc(x + RADIUS, y + RADIUS, RADIUS, 180, 270);
        drawArc(x + width - RADIUS, y + RADIUS, RADIUS, 270, 360);
        drawArc(x + width - RADIUS, y + height - RADIUS, RADIUS, 0, 90);
        drawArc(x + RADIUS, y + height - RADIUS, RADIUS, 90, 180);

        glDisable(GL_TEXTURE_2D);
    } else {
        // Draw the button without texture or with a solid color
        glColor3f(0.3f, 0.3f, 0.3f); // Grey color for no texture
        drawRoundedRect(x, y, width, height, RADIUS);
    }


    // Draw button text
    drawText(x + width / 2 - text.length() * 4.5f, y + height / 2 - 5, text);

    // If hovering or fading, render the lighting effect on top
    if (fadeAlpha > 0.0f) {
        // Calculate distances to each corner of the button for lighting effect
        float distX1 = mouseX - x;
        float distY1 = mouseY - y;
        float distance1 = sqrt(distX1 * distX1 + distY1 * distY1);

        float distX2 = mouseX - (x + width);
        float distY2 = mouseY - y;
        float distance2 = sqrt(distX2 * distX2 + distY2 * distY2);

        float distX3 = mouseX - (x + width);
        float distY3 = mouseY - (y + height);
        float distance3 = sqrt(distX3 * distX3 + distY3 * distY3);

        float distX4 = mouseX - x;
        float distY4 = mouseY - (y + height);
        float distance4 = sqrt(distX4 * distX4 + distY4 * distY4);

        // Smoother fade-out effect using larger maximum distance
        float maxDistance = std::sqrt(width * width + height * height);
        float intensity1 = 0.5f * (1.0f - (distance1 / maxDistance));
        float intensity2 = 0.5f * (1.0f - (distance2 / maxDistance));
        float intensity3 = 0.5f * (1.0f - (distance3 / maxDistance));
        float intensity4 = 0.5f * (1.0f - (distance4 / maxDistance));

        // Render the rounded lighting effect with gradient
        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        // Draw stencil shape
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        drawRoundedRect(x, y, width, height, RADIUS);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_EQUAL, 1, 1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        // Draw gradient quad using stencil
        glBegin(GL_QUADS);
        glColor4f(0.3f + intensity1 * fadeAlpha, 0.3f + intensity1 * fadeAlpha, 0.3f + intensity1 * fadeAlpha, fadeAlpha);
        glVertex2f(x, y);
        glColor4f(0.3f + intensity2 * fadeAlpha, 0.3f + intensity2 * fadeAlpha, 0.3f + intensity2 * fadeAlpha, fadeAlpha);
        glVertex2f(x + width, y);
        glColor4f(0.3f + intensity3 * fadeAlpha, 0.3f + intensity3 * fadeAlpha, 0.3f + intensity3 * fadeAlpha, fadeAlpha);
        glVertex2f(x + width, y + height);
        glColor4f(0.3f + intensity4 * fadeAlpha, 0.3f + intensity4 * fadeAlpha, 0.3f + intensity4 * fadeAlpha, fadeAlpha);
        glVertex2f(x, y + height);
        glEnd();

        glDisable(GL_STENCIL_TEST);
    }
}

void Button::drawRoundedRect(float x, float y, float width, float height, float radius) const {
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

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + radius);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y + radius);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height - radius);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height - radius);
    glEnd();
}

void Button::drawArc(float cx, float cy, float radius, float startAngle, float endAngle) const {
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