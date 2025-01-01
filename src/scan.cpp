#include "scan.h"
#include <fstream>
#include <future>

std::mutex queue_mutex;
std::mutex output_mutex;
std::atomic<bool> scanning(false);
std::atomic<int> files_processed(0);
std::atomic<int> total_files(0);
std::string filePath;
std::string hashString;
std::string status;
std::string numofthreat;
std::string msg;
std::atomic<size_t> threat(0);

// RAII wrapper for OpenSSL digest context
struct DigestContextRAII {
    EVP_MD_CTX* ctx;
    DigestContextRAII() : ctx(EVP_MD_CTX_new()) {
        if (!ctx) throw std::runtime_error("Failed to create digest context");
    }
    ~DigestContextRAII() { EVP_MD_CTX_free(ctx); }
    // Prevent copying
    DigestContextRAII(const DigestContextRAII&) = delete;
    DigestContextRAII& operator=(const DigestContextRAII&) = delete;
};

std::string sha256_file(const std::string& path) {
    DigestContextRAII mdctx;
    
    if (1 != EVP_DigestInit_ex(mdctx.ctx, EVP_sha256(), nullptr)) {
        msg = "Error initializing SHA-256";
        return "";
    }

    std::ifstream file(path, std::ifstream::binary);
    if (!file) {
        msg = "Error opening file: " + path;
        return "";
    }

    std::vector<char> buffer(8192);
    while (file.read(buffer.data(), buffer.size())) {
        if (1 != EVP_DigestUpdate(mdctx.ctx, buffer.data(), file.gcount())) {
            msg = "Error updating digest";
            return "";
        }
    }

    if (file.bad()) {
        msg = "Error reading file";
        return "";
    }

    std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
    unsigned int hash_len = 0;
    if (1 != EVP_DigestFinal_ex(mdctx.ctx, hash.data(), &hash_len)) {
        msg = "Error finalizing digest";
        return "";
    }

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool is_hash_in_set(const std::unordered_set<std::string>& hash_set, 
                   const std::string& hash) {
    std::string lowercase_hash = hash;
    std::transform(lowercase_hash.begin(), lowercase_hash.end(), 
                  lowercase_hash.begin(), ::tolower);
    return hash_set.find(lowercase_hash) != hash_set.end();
}

std::unordered_set<std::string> load_hashes(const std::string& filename) {
    std::unordered_set<std::string> hash_set;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        msg = "Error opening file: " + filename;
        return hash_set;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        if (line.length() == 64 && 
            line.find_first_not_of("0123456789abcdef") == std::string::npos) {
            hash_set.insert(line);
        } else {
            msg = "Invalid hash found: " + line;
        }
    }

    return hash_set;
}

void process_files(const std::unordered_set<std::string>& hash_set,
                  const std::vector<std::filesystem::path>& file_batch) {
    auto log_file = std::make_shared<std::ofstream>("log.txt", std::ios::app);
    
    for (const auto& file_path : file_batch) {
        try {
            msg.clear();
            
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
                        *log_file << "MALWARE DETECTED: " << filePath << '\n';
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
            msg = "Error processing file " + file_path.string() + ": " + e.what();
            
            if (log_file && log_file->is_open()) {
                *log_file << msg << '\n';
                log_file->flush();
            }
        }
    }
}

void scan_directory(const std::string& path, 
                   const std::unordered_set<std::string>& hash_set) {
    if (scanning.exchange(true)) return;
    
    files_processed = 0;
    total_files = 0;
    threat = 0;
    msg.clear();
    
    if (!std::filesystem::exists(path)) {
        msg = "Error: Directory does not exist: " + path;
        scanning = false;
        return;
    }

    if (!std::filesystem::is_directory(path)) {
        msg = "Error: Path is not a directory: " + path;
        scanning = false;
        return;
    }

    std::vector<std::filesystem::path> files;
    auto log_file = std::make_shared<std::ofstream>("log.txt", std::ios::app);
    
    try {
        std::error_code ec;
        const int MAX_DEPTH = 16;

        // Configure directory iterator to skip symlinks and handle permission issues
        std::filesystem::directory_options dir_options = 
            std::filesystem::directory_options::skip_permission_denied |
            std::filesystem::directory_options::follow_directory_symlink;

        for (auto iter = std::filesystem::recursive_directory_iterator(path, dir_options, ec);
             iter != std::filesystem::recursive_directory_iterator();) {
            
            try {
                if (ec) {
                    std::lock_guard<std::mutex> lock(output_mutex);
                    if (log_file && log_file->is_open()) {
                        *log_file << "Warning: " << iter->path().string() << ": " << ec.message() << '\n';
                        log_file->flush();
                    }
                    ec.clear();
                    iter.increment(ec);  // Try to continue to next entry
                    continue;
                }

                // Check for maximum depth
                if (iter.depth() > MAX_DEPTH) {
                    if (log_file && log_file->is_open()) {
                        *log_file << "Warning: Maximum depth exceeded at " << iter->path().string() << '\n';
                        log_file->flush();
                    }
                    iter.pop();
                    continue;
                }

                // Handle symbolic links
                const auto& entry = *iter;
                if (std::filesystem::is_symlink(entry, ec)) {
                    if (log_file && log_file->is_open()) {
                        *log_file << "Info: Skipping symlink " << entry.path().string() << '\n';
                        log_file->flush();
                    }
                    iter.increment(ec);
                    continue;
                }

                // Only process regular files
                if (std::filesystem::is_regular_file(entry, ec)) {
                    files.push_back(entry.path());
                    total_files++;
                }

                iter.increment(ec);

            } catch (const std::filesystem::filesystem_error& e) {
                std::lock_guard<std::mutex> lock(output_mutex);
                if (log_file && log_file->is_open()) {
                    *log_file << "Warning: " << e.what() << " at " << iter->path().string() << '\n';
                    log_file->flush();
                }
                
                // Try to skip problematic directory
                try {
                    iter.pop();
                } catch (...) {
                    iter.increment(ec);
                }
            }
        }
    }
    catch (const std::exception& e) {
        msg = "Error during directory scan: " + std::string(e.what());
        scanning = false;
        return;
    }

    if (files.empty()) {
        msg = "No files found in directory: " + path;
        scanning = false;
        return;
    }

    // Process files in batches
    const size_t BATCH_SIZE = 100;
    const unsigned int num_threads = std::thread::hardware_concurrency();
    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < files.size(); i += BATCH_SIZE) {
        size_t batch_end = std::min(i + BATCH_SIZE, files.size());
        std::vector<std::filesystem::path> batch(
            files.begin() + i, files.begin() + batch_end);

        futures.push_back(
            std::async(std::launch::async,
                      [&hash_set, batch = std::move(batch)]() {
                          process_files(hash_set, batch);
                      }));

        if (futures.size() >= num_threads) {
            for (auto& future : futures) {
                future.wait();
            }
            futures.clear();
        }
    }

    // Wait for remaining tasks
    for (auto& future : futures) {
        future.wait();
    }

    scanning = false;
}

void scan_file(const std::string& filePath, 
               const std::unordered_set<std::string>& hash_set) {
    std::cout << "Scanning file: " << filePath << std::endl;

    try {
        std::string fileHash = sha256_file(filePath);
        hashString = fileHash;

        if (fileHash.empty()) {
            msg = "Error: Unable to calculate hash for file.";
            return;
        }

        if (is_hash_in_set(hash_set, fileHash)) {
            msg = "File is potentially harmful (hash found in database).";
            status = "malware";
        } else {
            msg = "File is clean (hash not found in database).";
            status = "clean";
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        msg = "Filesystem error: " + std::string(e.what());
    }
}
