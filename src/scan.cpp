#include "scan.h"

std::mutex queue_mutex;
std::mutex output_mutex;
std::vector<std::filesystem::path> file_paths;
std::atomic<bool> scanning(false);
std::atomic<int> files_processed(0);
std::atomic<int> total_files(0);
std::string filePath = "";
std::string hashString = "";
std::string status = "";
std::string numofthreat = "";
size_t threat = 0;

std::string sha256_file(const std::string& path) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        std::cerr << "Error creating EVP_MD_CTX" << std::endl;
        return "";
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
        std::cerr << "Error initializing SHA-256" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    std::ifstream file(path, std::ifstream::binary);
    if (!file) {
        std::cerr << "Error opening file: " << path << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        if (1 != EVP_DigestUpdate(mdctx, buffer, file.gcount())) {
            std::cerr << "Error updating digest" << std::endl;
            EVP_MD_CTX_free(mdctx);
            return "";
        }
    }
    if (file.bad()) {
        std::cerr << "Error reading file" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    if (1 != EVP_DigestUpdate(mdctx, buffer, file.gcount())) {
        std::cerr << "Error updating digest" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (1 != EVP_DigestFinal_ex(mdctx, hash, &hash_len)) {
        std::cerr << "Error finalizing digest" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
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
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        if (line.length() == 64 && line.find_first_not_of("0123456789abcdef") == std::string::npos) {
            hash_set.insert(line);
        }
        else {
            std::cerr << "Invalid hash found: " << line << std::endl;
        }
    }

    return hash_set;
}

bool is_hash_in_set(const std::unordered_set<std::string>& hash_set, const std::string& hash) {
    std::string lowercase_hash = hash;
    std::transform(lowercase_hash.begin(), lowercase_hash.end(), lowercase_hash.begin(), ::tolower);
    return hash_set.find(lowercase_hash) != hash_set.end();
}

void process_files(const std::unordered_set<std::string>& hash_set, std::vector<std::filesystem::path> thread_file_paths) {
    while (!thread_file_paths.empty()) {
        std::filesystem::path file_path = thread_file_paths.back();
        thread_file_paths.pop_back();

        try {
            std::string hash = sha256_file(file_path.string());
            if (!hash.empty()) {
                std::lock_guard<std::mutex> lock(output_mutex);
                filePath = file_path.string();
                if (is_hash_in_set(hash_set, hash)) {
                    threat++;
                    hashString = hash;
                    status = "malware";
                    numofthreat = std::to_string(threat);
                }
                else {
                    hashString = hash;
                    status = "clean";
                }
            }
            else {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cerr << "File: " << file_path << " : Unable to read file." << std::endl;
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cerr << "Standard exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cerr << "Unknown exception occurred." << std::endl;
        }

        files_processed++;
    }
}


void scan_directory(const std::string& path, const std::unordered_set<std::string>& hash_set) {
    scanning = true;
    files_processed = 0;
    total_files = 0;

    // Populate the file queue
    std::vector<std::filesystem::path> file_paths;
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
            try {
                // Check if entry is a regular file and add to queue
                if (entry.is_regular_file()) {
                    file_paths.push_back(entry.path());
                    total_files++;
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                // Log the error and continue with the next entry
                std::cerr << "Skipping file due to access error: " << e.what() << std::endl;
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error while scanning directory: " << e.what() << std::endl;
        scanning = false;
        return;
    }

    // Create and start threads
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 2; // Fallback to a reasonable default
    }
    std::vector<std::thread> threads;
    size_t chunk_size = file_paths.size() / num_threads;

    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? file_paths.size() : start + chunk_size;
        std::vector<std::filesystem::path> thread_files(file_paths.begin() + start, file_paths.begin() + end);

        threads.emplace_back([&hash_set, thread_files]() mutable {
            process_files(hash_set, std::move(thread_files));
            });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    scanning = false;
}


void scan_file(const std::string& filePath, const std::unordered_set<std::string>& hash_set) {
    std::cout << "Scanning file: " << filePath << std::endl;

    std::string fileHash;
    try {
        fileHash = sha256_file(filePath);
        hashString = fileHash;

        if (fileHash.empty()) {
            std::cerr << "Error: Unable to calculate hash for file." << std::endl;
            return;
        }

        std::cout << "Calculated SHA-256 hash: " << fileHash << std::endl;

        if (is_hash_in_set(hash_set, fileHash)) {
            std::cout << "File is potentially harmful (hash found in database)." << std::endl;
            status = "malware";
        }
        else {
            std::cout << "File is clean (hash not found in database)." << std::endl;
            status = "clean";
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}
