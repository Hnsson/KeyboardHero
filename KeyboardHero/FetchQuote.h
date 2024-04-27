#pragma once
#include "ThreadQueue.h"
#include <tuple>
#include <vector>
#include <string>
#include "curl/curl.h"
#include "regex"

#include "curl/curl.h"

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
            //// Extract the quote
            std::regex quote_regex("\"quote\":\"(.*?)\"");
            std::regex title_regex("\"title\":\"(.*?)\"");
            std::regex character_regex("\"character\":\"(.*?)\"");
            std::smatch match;

            //std::cout << readBuffer << std::endl;
            if (std::regex_search(readBuffer, match, quote_regex) && match.size() > 1) {
                out_sentence = ParseSentence(match[1].str());
            }
            else {
                std::cout << "No quote found!" << std::endl;
                return false;
            }
            if (std::regex_search(readBuffer, match, title_regex) && match.size() > 1) {
                out_gameTitle = match[1].str();
            }
            else {
                out_gameTitle = "Title not found...";
            }
            if (std::regex_search(readBuffer, match, character_regex) && match.size() > 1) {
                out_characterName = match[1].str();
            }
            else {
                out_characterName = "Title not found...";
            }
        }

        curl_easy_cleanup(curl);
    }
}
