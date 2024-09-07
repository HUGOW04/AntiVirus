#ifndef SCAN_H
#define SCAN_H

#include <string>
#include <unordered_set>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <unordered_set>
#include <algorithm>

// Declare global variables
extern std::queue<std::filesystem::path> file_queue;
extern std::mutex queue_mutex;
extern std::mutex output_mutex;
extern std::atomic<bool> scanning;
extern std::atomic<int> files_processed;
extern std::atomic<int> total_files;
extern std::string filePath;
extern std::string hashString;
extern std::string status;
extern std::string numofthreat;

std::string sha256_file(const std::string& path);
std::unordered_set<std::string> load_hashes(const std::string& filename);
bool is_hash_in_set(const std::unordered_set<std::string>& hash_set, const std::string& hash);
void process_files(const std::unordered_set<std::string>& hash_set);
void scan_directory(const std::string& path, const std::unordered_set<std::string>& hash_set);
void scan_file(const std::string& filePath, const std::unordered_set<std::string>& hash_set);

#endif
