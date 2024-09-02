#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <filesystem>
#include <openssl/sha.h>

// Function to calculate SHA-256 hash of a file
std::string sha256_file(const std::string& path) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    std::ifstream file(path, std::ifstream::binary);
    if (!file) {
        return ""; // Return empty string if file cannot be opened
    }

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    SHA256_Update(&sha256, buffer, file.gcount());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// Function to load hashes from a file into an unordered_set
std::unordered_set<std::string> load_hashes(const std::string& filename) {
    std::unordered_set<std::string> hash_set;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return hash_set;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        hash_set.insert(line);
    }
    return hash_set;
}

// Function to check if a file's SHA-256 hash is in the hash set
bool is_hash_in_set(const std::unordered_set<std::string>& hash_set, const std::string& hash) {
    return hash_set.find(hash) != hash_set.end();
}

int main() {
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

    // Load hashes into an unordered_set
    std::unordered_set<std::string> hash_set = load_hashes("full_sha256.txt");

    // Iterate through files in the directory and check their SHA-256 hashes
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
        try {
            if (entry.is_regular_file()) {
                std::string hash = sha256_file(entry.path().string());
                if (!hash.empty()) {
                    if (is_hash_in_set(hash_set, hash)) {
                        std::cout << "File: " << entry.path() << " : Hash found: " << hash << std::endl;
                    } else {
                        std::cout << "File: " << entry.path() << " : Hash not found: " << hash << std::endl;
                    }
                } else {
                    std::cout << "File: " << entry.path() << " : Unable to read file." << std::endl;
                }
            } 
        } catch (const std::exception& e) {
            std::cerr << "Error processing entry " << entry.path() << ": " << e.what() << std::endl;
        }
    }

    return 0;
}
