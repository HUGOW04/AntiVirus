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
#include "downloadhash.h"

bool mouseLeftPressed = false;
Button* currentlyHoveredButton = nullptr;
std::vector<Button> scanButtons, sideButtons;
std::vector<Widget> scanRects, sideRects,networkRects;

std::unordered_set<std::string> hash_set;

void renderDynamicProgressAnimation(float centerX, float centerY, float size, float progress, float time, bool isScanning) {
    const int segments = 120;
    const float lineWidth = 4.0f;
    const float maxRadius = size / 2;

    glPushMatrix();
    glTranslatef(centerX, centerY, 0);

    if (isScanning) {
        // Atomic/Biohazard scanning effect
        const float rotationSpeed = time * 1.2f;
        
        // Render biohazard symbol segments
        const int numSymbolSegments = 3;
        float symbolSize = maxRadius * 0.85f;
        float innerRadius = symbolSize * 0.3f;
        float outerRadius = symbolSize * 0.7f;
        
        for (int i = 0; i < numSymbolSegments; i++) {
            float segmentAngle = (2.0f * M_PI * i / numSymbolSegments) + rotationSpeed;
            
            // Draw the main segment arc
            glLineWidth(lineWidth * 1.5f);
            glBegin(GL_LINE_STRIP);
            for (int j = 0; j <= segments / 3; j++) {
                float t = (float)j / (segments / 3);
                float arcAngle = segmentAngle + t * 2.0f * M_PI / numSymbolSegments;
                float scanProgress = (arcAngle + M_PI) / (2.0f * M_PI);
                float alpha = fmod(scanProgress, 1.0f) <= progress ? 1.0f : 0.2f;
                
                // Pulsing effect
                float pulse = sin(time * 5 + t * 10) * 0.1f + 0.9f;
                float radius = outerRadius * pulse;
                
                glColor4f(0.0f, 0.8f, 1.0f, alpha);
                float x = radius * cos(arcAngle);
                float y = radius * sin(arcAngle);
                glVertex2f(x, y);
            }
            glEnd();
            
            // Draw orbital rings
            const int numOrbits = 2;
            for (int orbit = 0; orbit < numOrbits; orbit++) {
                float orbitRadius = innerRadius + (outerRadius - innerRadius) * ((float)orbit / numOrbits);
                float orbitRotation = rotationSpeed * (orbit % 2 ? 1 : -1) * 0.5f;
                
                glLineWidth(lineWidth * 0.8f);
                glBegin(GL_LINE_STRIP);
                for (int j = 0; j <= segments; j++) {
                    float t = (float)j / segments;
                    float angle = segmentAngle + t * 2.0f * M_PI + orbitRotation;
                    float alpha = fmod((angle + M_PI) / (2.0f * M_PI), 1.0f) <= progress ? 0.6f : 0.1f;
                    
                    // Orbital wave effect
                    float wave = sin(angle * 6 + time * 4) * 0.1f;
                    float radius = orbitRadius * (1.0f + wave);
                    
                    glColor4f(0.0f, 1.0f, 1.0f, alpha);
                    float x = radius * cos(angle);
                    float y = radius * sin(angle);
                    glVertex2f(x, y);
                }
                glEnd();
            }
        }
        
        // Electron particles orbiting the structure
        const int numParticles = 18;
        for (int i = 0; i < numParticles; i++) {
            float particleProgress = fmod(time * 0.5f + (float)i / numParticles, 1.0f);
            if (particleProgress <= progress) {
                float angle = particleProgress * 2 * M_PI + rotationSpeed;
                float spiralR = maxRadius * (0.3f + particleProgress * 0.5f);
                
                // Electron core
                glBegin(GL_TRIANGLE_FAN);
                float px = spiralR * cos(angle);
                float py = spiralR * sin(angle);
                
                glColor4f(0.0f, 1.0f, 1.0f, 0.9f);
                glVertex2f(px, py);
                glColor4f(0.0f, 0.8f, 1.0f, 0.0f);
                
                float particleSize = lineWidth * 2.0f;
                for (int j = 0; j <= 12; j++) {
                    float a = j * 2 * M_PI / 12;
                    glVertex2f(px + particleSize * cos(a), py + particleSize * sin(a));
                }
                glEnd();
                
                // Electron trail
                glLineWidth(lineWidth * 0.6f);
                glBegin(GL_LINE_STRIP);
                for (int t = 0; t < 12; t++) {
                    float trailAngle = angle - t * 0.1f;
                    float trailR = spiralR * (1.0f - t * 0.02f);
                    float tx = trailR * cos(trailAngle);
                    float ty = trailR * sin(trailAngle);
                    glColor4f(0.0f, 1.0f, 1.0f, 0.4f * (1.0f - (float)t / 12));
                    glVertex2f(tx, ty);
                }
                glEnd();
            }
        }

        // Central core pulse
        float pulseSize = maxRadius * 0.2f * (1.0f + 0.2f * sin(time * 8));
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.0f, 1.0f, 1.0f, 0.8f);
        glVertex2f(0, 0);
        glColor4f(0.0f, 0.8f, 1.0f, 0.0f);
        
        for (int i = 0; i <= 32; i++) {
            float a = i * 2 * M_PI / 32;
            glVertex2f(pulseSize * cos(a), pulseSize * sin(a));
        }
        glEnd();
        
    } else {
        // Existing idle animation remains unchanged
        const int numOrbitals = 3;
        const int particlesPerOrbit = 6;
        
        for (int orbit = 0; orbit < numOrbitals; orbit++) {
            float orbitRadius = maxRadius * (0.4f + orbit * 0.3f);
            float orbitSpeed = 0.7f - orbit * 0.2f;
            
            for (int i = 0; i < particlesPerOrbit; i++) {
                float baseAngle = (2 * M_PI * i) / particlesPerOrbit;
                float angle = baseAngle + time * orbitSpeed * (orbit % 2 == 0 ? 1 : -1);
                
                float wobble = sin(time * 3 + baseAngle) * 0.1f;
                float x = orbitRadius * (1 + wobble) * cos(angle);
                float y = orbitRadius * (1 + wobble) * sin(angle);
                
                glLineWidth(lineWidth * 0.5f);
                glBegin(GL_LINE_STRIP);
                for (int t = 0; t < 8; t++) {
                    float trailAngle = angle - t * 0.1f * (orbit % 2 == 0 ? 1 : -1);
                    float trailX = orbitRadius * cos(trailAngle);
                    float trailY = orbitRadius * sin(trailAngle);
                    float trailAlpha = 0.3f * (1.0f - (float)t / 8);
                    glColor4f(0.0f, 1.0f, 1.0f, trailAlpha);
                    glVertex2f(trailX, trailY);
                }
                glEnd();
                
                float particleSize = lineWidth * (1.0f - orbit * 0.2f);
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(0.0f, 1.0f, 1.0f, 0.8f);
                glVertex2f(x, y);
                glColor4f(0.0f, 1.0f, 1.0f, 0.0f);
                for (int j = 0; j <= 16; j++) {
                    float a = j * 2 * M_PI / 16;
                    glVertex2f(x + particleSize * cos(a), y + particleSize * sin(a));
                }
                glEnd();
            }
        }
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

float colorOffset = 0.0f;

// Function to render the diagonal gradient background
void renderBackground() {
        float backgroundStartColor[3];
    float backgroundEndColor[3];

    // Change colors based on scanning state
    if (scanning) {
        backgroundStartColor[0] = 0.2f; backgroundStartColor[1] = 0.0f; backgroundStartColor[2] = 0.0f;  // Dark red
        backgroundEndColor[0] = 1.0f; backgroundEndColor[1] = 0.5f; backgroundEndColor[2] = 0.0f;        // Orange
    } else {
        backgroundStartColor[0] = 0.0f; backgroundStartColor[1] = 0.0f; backgroundStartColor[2] = 0.2f;  // Dark blue
        backgroundEndColor[0] = 0.0f; backgroundEndColor[1] = 0.5f; backgroundEndColor[2] = 1.0f;        // Light blue
    }
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

    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Update hash database before initializing the rest
    std::cout << "Updating malware hash database..." << std::endl;
    std::ifstream fileHash("full_sha256.txt");
    if(!fileHash.good())
    {
        if (!updateHashDatabase()) {
            std::cerr << "Failed to update hash database: " << errorMSG << std::endl;
            std::cerr << "Will try to use existing database file..." << std::endl;
        }
    }

    // Load hashes into an unordered_set (keep your existing code)
    hash_set = load_hashes("full_sha256.txt");

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
        Widget(400.0f, 380.0f, 300.0f, 200.0f, 20.0f, rectColor),
        Widget(80.0f, 350.0f, 200.0f, 30.0f, 0.0f, rectColor,status),
        Widget(80.0f, 320.0f, 200.0f, 30.0f, 0.0f, rectColor,numofthreat),
        Widget(80.0f, 280.0f, 720.0f, 30.0f, 20.0f, rectColor,hashString),
        Widget(80.0f, 150.0f, 720.0f, 100.0f, 20.0f, rectColor, filePath),
        Widget(80.0f, 5.0f, 720.0f, 100.0f, 20.0f, rectColor, filePath),
    };

    networkRects = {
        Widget(100.0f, 300.0f, 300.0f, 200.0f, 20.0f, rectColor)
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


                scanRects[2].setText("status: " + status);
                scanRects[3].setText("viruses found: " + numofthreat);
                scanRects[4].setText("sha256: "+hashString);
                scanRects[5].setText("filepath: " + filePath);
                scanRects[6].setText("errorMSG: " + errorMSG);
            }
            else
            {
                 renderDynamicProgressAnimation(550, 500, 60.0f, 0.0f, animationTime, false);
                
                scanRects[2].setText("status: " + status);
                scanRects[3].setText("viruses found: " + numofthreat);
                scanRects[4].setText("sha256: "+hashString);
                scanRects[5].setText("filepath: ");
                scanRects[6].setText("errorMSG: " + errorMSG);
            }
        }
        else if(network)
        {
            for (const auto& rect : networkRects)
                rect.render();
        }
        // Swap the screen buffers
        glfwSwapBuffers(window);
    }
    curl_global_cleanup();
    // Terminate GLFW
    glfwTerminate();
    return 0;
}


