#pragma once
#include "ThreadQueue.h"
#include <tuple>
#include <vector>
#include <string>
#include "curl/curl.h"
#include "regex"

#include "curl/curl.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

inline static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


inline bool SetupQuoteWithCURL(std::vector<std::string>& out_sentence, std::string& out_gameTitle, std::string& out_characterName) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://ultima.rest/api/random");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Enable following redirection

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }
        else {
            try {
                auto j = json::parse(readBuffer);

                // Extract data using JSON
                out_sentence = ParseSentence(j.value("quote", "Quote not found...")); // assuming "quote" is a JSON array of strings
                out_gameTitle = j.value("title", "Title not found...");  // Use default if "title" is not present
                out_characterName = j.value("character", "Character not found...");  // Use default if "character" is not present
            }
            catch (json::exception& e) {
                std::cerr << "JSON parsing error: " << e.what() << '\n';
                return false;
            }
        }

        curl_easy_cleanup(curl);
    }
}
