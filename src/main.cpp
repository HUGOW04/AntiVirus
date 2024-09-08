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
#include "tinyfiledialogs.h"


// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Global variables for mouse position and state
double mouseX = 0.0, mouseY = 0.0;
bool mouseLeftPressed = false;
Button* currentlyHoveredButton = nullptr;
std::vector<Button> scanButtons, sideButtons;
std::vector<Widget> scanRects, sideRects;

// Load hashes into an unordered_set
std::unordered_set<std::string> hash_set = load_hashes("full_sha256.txt");

bool scan = true;
bool network = false;

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
    const float lineWidth = 3.0f;
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
        Widget(80.0f, 150.0f, 700.0f, 30.0f, 20.0f, rectColor,hashString),
        Widget(80.0f, 300.0f, 200.0f, 30.0f, 0.0f, rectColor,status),
        Widget(80.0f, 270.0f, 200.0f, 30.0f, 0.0f, rectColor,numofthreat),
    };

    float rotation = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        // Check for events
        glfwPollEvents();

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);



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
                renderLoadingAnimation(550, 500, 30.0f, rotation);

                float progress = static_cast<float>(files_processed) / total_files;
                char progressText[64];
                snprintf(progressText, sizeof(progressText), "%.1f%% (%d/%d files)",
                    progress * 100, files_processed.load(), total_files.load());
                glColor3f(1.0f, 1.0f, 1.0f);
                renderText(progressText, 450, 400);

                scanRects[1].setText("filepath: " + filePath);
                scanRects[3].setText(hashString);
                scanRects[4].setText("status: " + status);
                scanRects[5].setText("viruses found: " + numofthreat);
            }
            else
            {
                scanRects[1].setText("filepath: ");
                scanRects[3].setText(hashString);
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

// Callback for keyboard input
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// Callback for mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = HEIGHT - ypos; // Flip y-coordinate to match OpenGL's coordinate system
}

// Callback for mouse button events
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        for (const auto& button : sideButtons) {
            if (mouseX >= button.getX() && mouseX <= button.getX() + button.getWidth() &&
                mouseY >= button.getY() && mouseY <= button.getY() + button.getHeight()) {
                if (button.getId() == "home")
                {
                    scan = true;
                    network = false;
                }
                else if (button.getId() == "network")
                {
                    scan = false;
                    network = true;
                }
            }
        }
        if (scan)
        {
            for (const auto& button : scanButtons) {
                if (mouseX >= button.getX() && mouseX <= button.getX() + button.getWidth() &&
                    mouseY >= button.getY() && mouseY <= button.getY() + button.getHeight()) {
                    if (button.getId() == "Scan") {
                        // Open a file dialog using TinyFileDialogs
                        const char* file = tinyfd_openFileDialog(
                            "Select a file to scan",      // Title
                            "",                          // Default path
                            0,                           // Number of filter patterns
                            nullptr,                        // Filter patterns
                            nullptr,                        // Filter description
                            0                           // Allow multiple selection
                        );

                        if (file) {
                            std::cout << "Selected file: " << file << std::endl;
                            // Start scanning the selected file
                            std::thread scan_thread(scan_file, file, std::ref(hash_set));
                            scan_thread.detach();  // Detach the thread so it can run independently
                        }
                        else {
                            std::cout << "No file selected" << std::endl;
                        }
                    }
                    else if (button.getId() == "Fullscan") {
                        std::string path = "";
#ifdef __linux__
                        std::string pcName = getenv("USER");
                        path = "/home/" + pcName + "/";
#elif _WIN32
                        path = "C:\\";
#elif __APPLE__
                        std::string homePath = getenv("HOME");
                        path = homePath + "/";
#endif

                        // Start scanning in a separate thread
                        std::thread scan_thread(scan_directory, path, std::ref(hash_set));
                        scan_thread.detach();  // Detach the thread so it can run independently
                    }
                    else if (button.getId() == "Log")
                    {
                        std::cout<<"hi"<<std::endl;
                    }
                }
            }
        }
    }
}
