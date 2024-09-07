#include <scan.h>
#include <algorithm> // for std::transform

// Define global variables
std::queue<std::filesystem::path> file_queue;
std::mutex queue_mutex;
std::mutex output_mutex;
std::atomic<bool> scanning(false);
std::atomic<int> files_processed(0);
std::atomic<int> total_files(0);
std::string filePath = "";
std::string hashString = "";
std::string status = "";
std::string numofthreat = "";
size_t threat = 0;

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

std::unordered_set<std::string> load_hashes(const std::string& filename) {
    std::unordered_set<std::string> hash_set;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return hash_set;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace from the start and end of the line
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Skip empty lines and lines that start with '#'
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Convert hash to lowercase
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        
        // Optional: Validate that the line is a valid SHA-256 hash
        if (line.length() == 64 && line.find_first_not_of("0123456789abcdef") == std::string::npos) {
            hash_set.insert(line);
        } else {
            std::cerr << "Invalid hash found: " << line << std::endl;
        }
    }
    
    //std::cout << "Loaded " << hash_set.size() << " hashes." << std::endl;
    return hash_set;
}

// Improved function to check if a file's SHA-256 hash is in the hash set
bool is_hash_in_set(const std::unordered_set<std::string>& hash_set, const std::string& hash) {
    std::string lowercase_hash = hash;
    std::transform(lowercase_hash.begin(), lowercase_hash.end(), lowercase_hash.begin(), ::tolower);
    return hash_set.find(lowercase_hash) != hash_set.end();
}

void process_files(const std::unordered_set<std::string>& hash_set) {
    
    while (true) {
        std::filesystem::path file_path;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (file_queue.empty()) {
                break;
            }
            file_path = file_queue.front();
            file_queue.pop();
        }

        std::string hash = sha256_file(file_path.string());
        if (!hash.empty()) {
            std::lock_guard<std::mutex> lock(output_mutex);
            filePath = file_path.string();
            if (is_hash_in_set(hash_set, hash)) {
                //std::cout << "File: " << file_path << " : Hash found: " << hash << std::endl;
                threat++;
                hashString = hash;
                status = "malware";
                numofthreat = std::to_string(threat);
            } else {
                //std::cout << "File: " << file_path << " : Hash not found: " << hash << std::endl;
                hashString = hash;
                status = "clean";
            }
        } else {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "File: " << file_path << " : Unable to read file." << std::endl;
        }

        files_processed++;
    }
}

void scan_directory(const std::string& path, const std::unordered_set<std::string>& hash_set) {
    scanning = true;
    files_processed = 0;
    total_files = 0;

    // Count total files and populate queue
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file()) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            file_queue.push(entry.path());
            total_files++;
        }
    }

    // Determine number of threads
    unsigned int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    // Create and start threads
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back(process_files, std::ref(hash_set));
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    scanning = false;
}

// Improved function to scan a file
void scan_file(const std::string& filePath, const std::unordered_set<std::string>& hash_set) {
    std::cout << "Scanning file: " << filePath << std::endl;

    // Calculate the file's SHA-256 hash
    std::string fileHash = sha256_file(filePath);
    hashString = fileHash;
    
    if (fileHash.empty()) {
        std::cerr << "Error: Unable to calculate hash for file." << std::endl;
        return;
    }

    std::cout << "Calculated SHA-256 hash: " << fileHash << std::endl;

    // Check if the hash exists in the known hash set
    if (is_hash_in_set(hash_set, fileHash)) {
        std::cout << "File is potentially harmful (hash found in database)." << std::endl;
        status = "malware";
    } else {
        std::cout << "File is clean (hash not found in database)." << std::endl;
        status = "clean";
    }
}