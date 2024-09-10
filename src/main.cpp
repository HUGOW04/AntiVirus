#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <string>
#include <filesystem>
#include <fstream>


#include "widget.h"
#include "button.h"
#include "scan.h"
#include "callback.h"


bool mouseLeftPressed = false;
Button* currentlyHoveredButton = nullptr;
std::vector<Button> scanButtons, sideButtons;
std::vector<Widget> scanRects, sideRects;

// Load hashes into an unordered_set
std::unordered_set<std::string> hash_set = load_hashes("full_sha256.txt");

// Function to render the dynamic progress animation
void renderDynamicProgressAnimation(float centerX, float centerY, float size, float progress, float time, bool isScanning) {
    const int segments = 100;
    const float lineWidth = 5.0f;
    const int numCircles = 3;
    const float maxRadius = size / 2;
    const float minRadius = size / 6;

    glPushMatrix();
    glTranslatef(centerX, centerY, 0);

    // Draw pulsating circles
    for (int i = 0; i < numCircles; ++i) {
        float phase = 2 * M_PI * i / numCircles;
        float radius = minRadius + (maxRadius - minRadius) * (sin(time * 2 + phase) + 1) / 2;

        glColor4f(0.0f, 0.7f, 1.0f, 0.3f);  // Light blue color with more transparency
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);
        for (int j = 0; j <= 30; ++j) {
            float angle = 2 * M_PI * j / 30;
            float x = radius * cos(angle);
            float y = radius * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }

    // Draw progress arc
    glLineWidth(lineWidth);
    if (isScanning) {
        glColor4f(0.0f, 0.7f, 1.0f, 1.0f);  // Light blue color for idle
        
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= segments; i++) {
            float theta = -(2.0f * M_PI * float(i) / float(segments)) + M_PI / 2.0f;  // Clockwise rotation
            if (isScanning) {
                if (i > segments * progress) break;
            } else {
                if (i > segments * fmod(time, 1.0f)) break;
            }
            float x = maxRadius * cosf(theta);
            float y = maxRadius * sinf(theta);
            glVertex2f(x, y);
        }
        glEnd();
    } else {
        //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // Green color for scanning
    }

    glPopMatrix();
}

// Function to render the progress bar
void renderProgressBar(float progress, float x, float y, float width, float height) {
    // Draw the background of the progress bar
    glColor3f(0.5f, 0.5f, 0.5f);  // Gray color
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Draw the filled part of the progress bar
    glColor3f(0.0f, 1.0f, 0.0f);  // Green color
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width * progress, y);
    glVertex2f(x + width * progress, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Draw the border of the progress bar
    glColor3f(0.0f, 0.0f, 0.0f);  // Black color
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

// Function to render the loading animation
void renderLoadingAnimation(float centerX, float centerY, float radius, float rotation) {
    const int segments = 60;
    const float lineWidth = 1.0f;
    const float fadeLength = 0.5f; // Length of the fade as a fraction of the circle

    glPushMatrix();
    glTranslatef(centerX, centerY, 0);
    glRotatef(-rotation, 0, 0, 1);

    glLineWidth(lineWidth);
    glBegin(GL_LINE_STRIP);

    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2 * PI;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        float alpha = 1.0f;
        if (i > segments * (1 - fadeLength)) {
            alpha = 1.0f - (float)(i - segments * (1 - fadeLength)) / (segments * fadeLength);
        }

        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glVertex2f(x, y);
    }

    glEnd();
    glPopMatrix();
}

// Function to render text
void renderText(const char* text, float x, float y) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// New global variables for background color
float backgroundStartColor[3] = {0.0f, 0.0f, 0.2f};  // Dark blue
float backgroundEndColor[3] = {0.0f, 0.5f, 1.0f};    // Light blue
float colorOffset = 0.0f;

// Function to render the diagonal gradient background
void renderBackground() {
    glBegin(GL_TRIANGLES);
    
    // Bottom-left to top-right diagonal
    for (int i = 0; i <= WIDTH + HEIGHT; i += 10) {
        float t1 = (float)i / (WIDTH + HEIGHT);
        float t2 = (float)(i + 10) / (WIDTH + HEIGHT);
        
        float wave1 = (sin(colorOffset + t1 * 3.14159f) + 1.0f) / 2.0f;
        float wave2 = (sin(colorOffset + t2 * 3.14159f) + 1.0f) / 2.0f;
        
        float r1 = backgroundStartColor[0] * (1 - wave1) + backgroundEndColor[0] * wave1;
        float g1 = backgroundStartColor[1] * (1 - wave1) + backgroundEndColor[1] * wave1;
        float b1 = backgroundStartColor[2] * (1 - wave1) + backgroundEndColor[2] * wave1;
        
        float r2 = backgroundStartColor[0] * (1 - wave2) + backgroundEndColor[0] * wave2;
        float g2 = backgroundStartColor[1] * (1 - wave2) + backgroundEndColor[1] * wave2;
        float b2 = backgroundStartColor[2] * (1 - wave2) + backgroundEndColor[2] * wave2;
        
        glColor3f(r1, g1, b1);
        glVertex2f(0, i);
        glVertex2f(i, 0);
        
        glColor3f(r2, g2, b2);
        glVertex2f(10, i + 10);
        
        glColor3f(r1, g1, b1);
        glVertex2f(i, 0);
        glVertex2f(i + 10, 0);
        
        glColor3f(r2, g2, b2);
        glVertex2f(10, i + 10);
    }
    
    glEnd();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create a GLFWwindow object
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "AntiVirus", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth testing (if using 2D rendering)
    glDisable(GL_DEPTH_TEST);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Set up an orthographic projection matrix to match the window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, WIDTH, 0.0, HEIGHT, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // clean or remove the log file
    std::ofstream ofs("log.txt", std::ofstream::out | std::ofstream::trunc);
    ofs.close();

    // Initialize buttons
    sideButtons = {
        Button(5.0f, 530.0f, 50.0f, 50.0f, "home", "", "img/scan.png"),
        Button(5.0f, 450.0f, 50.0f, 50.0f, "network", "", "img/wifi.png"),
    };

    scanButtons = {
        Button(110.0f, 500.0f, 80.0f, 30.0f, "Scan", "Scan", ""),
        Button(210.0f, 500.0f, 80.0f, 30.0f, "Fullscan", "Fullscan", ""),
        Button(160.0f, 460.0f, 80.0f, 30.0f, "Log", "Log", ""),
    };

    // Add rounded rectangles
    float rectColor[3] = { 0.2f, 0.2f, 0.2f };

    sideRects = {
        Widget(-20.0f, 0.0f, 80.0f, 600.0f, 20.0f, rectColor),
    };
    scanRects = {
        Widget(100.0f, 450.0f, 200.0f, 100.0f, 20.0f, rectColor),
        Widget(80.0f, 5.0f, 720.0f, 100.0f, 20.0f, rectColor, filePath),
        Widget(400.0f, 380.0f, 300.0f, 200.0f, 20.0f, rectColor),
        Widget(80.0f, 130.0f, 720.0f, 30.0f, 20.0f, rectColor,hashString),
        Widget(80.0f, 300.0f, 200.0f, 30.0f, 0.0f, rectColor,status),
        Widget(80.0f, 270.0f, 200.0f, 30.0f, 0.0f, rectColor,numofthreat),
    };

    float rotation = 0.0f;
      float animationTime = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
         animationTime += deltaTime;
        // Check for events
        glfwPollEvents();

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update color offset for animated gradient
        colorOffset += deltaTime * 0.5f;
        if (colorOffset > 2 * 3.14159f) colorOffset -= 2 * 3.14159f;

        // Render the gradient background
        renderBackground();


        // Render rounded rectangle widgets
        for (const auto& rect : sideRects)
            rect.render();
        for (const auto& button : sideButtons)
            button.render(mouseX, mouseY);



        // Update button states and determine the currently hovered button
        Button* newHoveredButton = nullptr;
        for (auto& button : scanButtons) {
            button.updateState(mouseX, mouseY, &button == currentlyHoveredButton);
            if (button.isHovered) newHoveredButton = &button;
        }
        for (auto& button : sideButtons) {
            button.updateState(mouseX, mouseY, &button == currentlyHoveredButton);
            if (button.isHovered) newHoveredButton = &button;
        }

        // Apply fade-out to the previous hovered button if necessary
        if (currentlyHoveredButton && currentlyHoveredButton != newHoveredButton) {
            currentlyHoveredButton->isHovered = false;
            currentlyHoveredButton->fadeAlpha = 0.0f;
        }
        // Update the currently hovered button
        currentlyHoveredButton = newHoveredButton;

        if (scan)
        {
            for (const auto& rect : scanRects)
                rect.render();
            // Render buttons

            for (const auto& button : scanButtons)
                button.render(mouseX, mouseY);

            // Update rotation
            rotation += 360.0f * deltaTime; // Rotate 360 degrees per second
            if (rotation >= 360.0f) rotation -= 360.0f;
            // Update progress if scanning
            if (scanning) {
                float progress = static_cast<float>(files_processed) / total_files;
                renderDynamicProgressAnimation(550, 500, 60.0f, progress, animationTime, true);
                char progressText[64];
                snprintf(progressText, sizeof(progressText), "%.1f%% (%d/%d files)",
                    progress * 100, files_processed.load(), total_files.load());
                glColor3f(1.0f, 1.0f, 1.0f);
                renderText(progressText, 450, 400);

                scanRects[1].setText("filepath: " + filePath);
                scanRects[3].setText("sha256: "+hashString);
                scanRects[4].setText("status: " + status);
                scanRects[5].setText("viruses found: " + numofthreat);
            }
            else
            {
                 renderDynamicProgressAnimation(550, 500, 60.0f, 0.0f, animationTime, false);
                scanRects[1].setText("filepath: ");
                scanRects[3].setText("sha256: "+hashString);
                scanRects[4].setText("status: " + status);
            }
        }
        // Swap the screen buffers
        glfwSwapBuffers(window);
    }

    // Terminate GLFW
    glfwTerminate();
    return 0;
}


