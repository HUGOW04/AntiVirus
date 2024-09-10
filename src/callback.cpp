#include "callback.h"
#include "tinyfiledialogs.h"
#include "scan.h"
#include "main.h"

double mouseX = 0.0, mouseY = 0.0;
// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

bool scan = true;
bool network = false;

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