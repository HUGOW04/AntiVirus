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
#include "gui.h"

bool mouseLeftPressed = false;
Button* currentlyHoveredButton = nullptr;
std::vector<Button> scanButtons, sideButtons;
std::vector<Widget> scanRects, sideRects,networkRects;

std::unordered_set<std::string> hash_set;

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
    std::ifstream fileHash("full_sha256.txt");
    if(!fileHash.good())
    {
        if (!updateHashDatabase()) {
            std::cerr << "Failed to update hash database: " << msg << std::endl;
            std::cerr << "Will try to use existing database file..." << std::endl;
        }
        std::cout << "Updating malware hash database..." << std::endl;
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
        Widget(80.0f, 5.0f, 720.0f, 140.0f, 20.0f, rectColor, msg),
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
                scanRects[6].setText("msg: " + msg);
            }
            else
            {
                 renderDynamicProgressAnimation(550, 500, 60.0f, 0.0f, animationTime, false);
                
                scanRects[2].setText("status: " + status);
                scanRects[3].setText("viruses found: " + numofthreat);
                scanRects[4].setText("sha256: "+hashString);
                scanRects[5].setText("filepath: ");
                scanRects[6].setText("msg: " + msg);
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


