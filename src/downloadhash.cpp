#include "downloadhash.h"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), realsize);
    return realsize;
}

bool downloadHashFile(const std::string& url, const std::string& outputPath) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DownloadConfig::USER_AGENT.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return false;
    }

    return extractZipFile(response.data(), response.size(), outputPath);
}

bool extractZipFile(const void* zipData, size_t zipSize, const std::string& outputPath) {
    zip_error_t zip_error;
    zip_error_init(&zip_error);
    
    zip_source_t* src = zip_source_buffer_create(zipData, zipSize, 0, &zip_error);
    if (!src) {
        zip_error_fini(&zip_error);
        return false;
    }

    zip_t* za = zip_open_from_source(src, 0, &zip_error);
    if (!za) {
        zip_source_free(src);
        zip_error_fini(&zip_error);
        return false;
    }

    // Get the first file in the archive
    zip_stat_t sb;
    if (zip_stat_index(za, 0, 0, &sb) != 0) {
        zip_close(za);
        zip_error_fini(&zip_error);
        return false;
    }

    // Read the file content
    zip_file_t* zf = zip_fopen_index(za, 0, 0);
    if (!zf) {
        zip_close(za);
        zip_error_fini(&zip_error);
        return false;
    }

    std::vector<char> buffer(sb.size);
    zip_fread(zf, buffer.data(), sb.size);
    zip_fclose(zf);
    zip_close(za);
    zip_error_fini(&zip_error);

    // Write to output file
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        return false;
    }

    outFile.write(buffer.data(), buffer.size());
    outFile.close();

    return true;
}

bool updateHashDatabase() {
    const auto& url = DownloadConfig::SHA256_FULL_URL;
    const auto& outputPath = DownloadConfig::DEFAULT_OUTPUT_PATH;
    
    // Create a timestamp for backup
    std::time_t now = std::time(nullptr);
    std::string backupPath = outputPath + ".txt";

    // Backup existing file if it exists
    if (std::ifstream(outputPath)) {
        std::rename(outputPath.c_str(), backupPath.c_str());
    }

    bool success = downloadHashFile(url, outputPath);
    
    if (!success && std::ifstream(backupPath)) {
        // Restore backup if download failed
        std::rename(backupPath.c_str(), outputPath.c_str());
    }

    return success;
}