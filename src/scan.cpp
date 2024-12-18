#include "scan.h"
#include <fstream>

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
std::string errorMSG = "";
size_t threat = 0;

std::string sha256_file(const std::string& path) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        errorMSG = "Error creating EVP_MD_CTX";
        return "";
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
        errorMSG = "Error initializing SHA-256";
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    std::ifstream file(path, std::ifstream::binary);
    if (!file) {
        errorMSG = "Error opening file: " + path;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        if (1 != EVP_DigestUpdate(mdctx, buffer, file.gcount())) {
            errorMSG = "Error updating digest";
            EVP_MD_CTX_free(mdctx);
            return "";
        }
    }
    if (file.bad()) {
        errorMSG = "Error reading file";
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    if (1 != EVP_DigestUpdate(mdctx, buffer, file.gcount())) {
        errorMSG = "Error updating digest";
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (1 != EVP_DigestFinal_ex(mdctx, hash, &hash_len)) {
        errorMSG = "Error finalizing digest";
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

bool is_hash_in_set(const std::unordered_set<std::string>& hash_set, const std::string& hash) {
    std::string lowercase_hash = hash;
    std::transform(lowercase_hash.begin(), lowercase_hash.end(), lowercase_hash.begin(), ::tolower);
    return hash_set.find(lowercase_hash) != hash_set.end();
}

std::unordered_set<std::string> load_hashes(const std::string& filename) {
    std::unordered_set<std::string> hash_set;
    std::ifstream file(filename);

    if (!file.is_open()) {
        errorMSG = "Error opening file: " + filename;
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
            errorMSG = "Invalid hash found: " + line;
        }
    }

    return hash_set;
}

void process_files(const std::unordered_set<std::string>& hash_set, std::vector<std::filesystem::path> thread_file_paths) {
    // Use a fixed-size buffer for reading files
    std::unique_ptr<std::ofstream> log_file = std::make_unique<std::ofstream>("log.txt", std::ios::app);
    
    // Process files in smaller batches
    const size_t BATCH_SIZE = 100; // Adjust this value based on your system's memory
    
    while (!thread_file_paths.empty()) {
        size_t batch_count = 0;
        std::vector<std::filesystem::path> batch;
        
        while (!thread_file_paths.empty() && batch_count < BATCH_SIZE) {
            batch.push_back(std::move(thread_file_paths.back()));
            thread_file_paths.pop_back();
            batch_count++;
        }
        
        for (const auto& file_path : batch) {
            try {
                // Clear any previous error message
                errorMSG.clear();
                
                std::string hash = sha256_file(file_path.string());
                if (!hash.empty()) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    
                    filePath = file_path.string();
                    hashString = hash;
                    
                    if (is_hash_in_set(hash_set, hash)) {
                        threat++;
                        status = "malware";
                        numofthreat = std::to_string(threat);
                        
                        if (log_file && log_file->is_open()) {
                            *log_file << "MALWARE DETECTED: " << filePath << std::endl;
                            log_file->flush();
                        }
                    } else {
                        status = "clean";
                    }
                }
                
                files_processed++;
            }
            catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(output_mutex);
                errorMSG = "Error processing file " + file_path.string() + ": " + e.what();
            }
        }
        
        // Clear the batch to free memory
        batch.clear();
    }
}

void scan_directory(const std::string& path, const std::unordered_set<std::string>& hash_set) {
    scanning = true;
    files_processed = 0;
    total_files = 0;
    threat = 0;
    errorMSG.clear();
    
    if (!std::filesystem::exists(path)) {
        std::lock_guard<std::mutex> lock(output_mutex);
        errorMSG = "Error: Directory does not exist: " + path;
        scanning = false;
        return;
    }

    if (!std::filesystem::is_directory(path)) {
        std::lock_guard<std::mutex> lock(output_mutex);
        errorMSG = "Error: Path is not a directory: " + path;
        scanning = false;
        return;
    }

    std::queue<std::filesystem::path> file_queue;
    
    try {
        std::error_code ec;
        // Modify directory iterator options to not follow symlinks
        std::filesystem::recursive_directory_iterator dir_iter(
            path,
            std::filesystem::directory_options::skip_permission_denied,  // Removed follow_directory_symlink
            ec
        );

        const int MAX_DEPTH = 16; // Set maximum directory depth

        for (const auto& entry : dir_iter) {
            if (dir_iter.depth() > MAX_DEPTH) {
                dir_iter.pop(); // Go back up one level
                continue;
            }

            if (ec) {
                std::lock_guard<std::mutex> lock(output_mutex);
                errorMSG = "Warning: Skipping path: " + entry.path().string() + " (" + ec.message() + ")";
                ec.clear();
                continue;
            }

            try {
                if (entry.is_regular_file() && !entry.is_symlink()) { // Only process regular files, not symlinks
                    file_queue.push(entry.path());
                    total_files++;
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::lock_guard<std::mutex> lock(output_mutex);
                errorMSG = "Warning: Skipping file: " + entry.path().string() + " - " + e.what();
                continue;
            }
        }
    }
    catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(output_mutex);
        errorMSG = "Error during directory scan: " + std::string(e.what());
        scanning = false;
        return;
    }

    if (file_queue.empty()) {
        std::lock_guard<std::mutex> lock(output_mutex);
        errorMSG = "No files found in directory: " + path;
        scanning = false;
        return;
    }

    // Process files in chunks
    const size_t CHUNK_SIZE = 1000; // Adjust based on your system's memory
    unsigned int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    while (!file_queue.empty()) {
        std::vector<std::filesystem::path> chunk;
        chunk.reserve(CHUNK_SIZE);
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            while (!file_queue.empty() && chunk.size() < CHUNK_SIZE) {
                chunk.push_back(file_queue.front());
                file_queue.pop();
            }
        }
        
        threads.emplace_back([chunk = std::move(chunk), &hash_set]() {
            process_files(hash_set, std::move(chunk));
        });
        
        // If we've reached max threads or no more files, wait for completion
        if (threads.size() >= num_threads || file_queue.empty()) {
            for (auto& thread : threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            threads.clear();
        }
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
            errorMSG = "Error: Unable to calculate hash for file.";
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
        errorMSG = "Filesystem error: " + std::string(e.what());
    }
}
