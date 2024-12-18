#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <curl/curl.h>
#include <string>
#include <sstream>
#include <fstream>
#include <ctime>
#include <zip.h>
#include "scan.h"

/**
 * @brief Callback function for handling downloaded data
 * @param contents Pointer to the downloaded data
 * @param size Size of each data element
 * @param nmemb Number of elements
 * @param userp User pointer (used for storing the response)
 * @return Total size of processed data
 */
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

/**
 * @brief Downloads and extracts a ZIP file from a given URL
 * @param url Source URL to download from
 * @param outputPath Local path where the extracted file should be saved
 * @return true if download and extraction were successful, false otherwise
 */
bool downloadHashFile(const std::string& url, const std::string& outputPath);

/**
 * @brief Updates the hash database by downloading the latest hashes
 * @return true if update was successful, false otherwise
 */
bool updateHashDatabase();

/**
 * @brief Extracts the first file from a ZIP archive to the specified output path
 * @param zipData ZIP file contents in memory
 * @param zipSize Size of the ZIP data
 * @param outputPath Path where the extracted file should be saved
 * @return true if extraction was successful, false otherwise
 */
bool extractZipFile(const void* zipData, size_t zipSize, const std::string& outputPath);

// Constants for URLs and file paths
namespace DownloadConfig {
    const std::string SHA256_FULL_URL = "https://bazaar.abuse.ch//export/txt/sha256/full/";
    const std::string DEFAULT_OUTPUT_PATH = "full_sha256.txt";
    const std::string USER_AGENT = "Mozilla/5.0";
}

#endif // DOWNLOAD_MANAGER_H