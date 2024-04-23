#include <iostream>
#include <windows.h>
#include <vector>
#include <sstream>

#include <numeric>
#include <stdio.h>
#include <cmath>
#include <iomanip>
#include <regex>

#include "curl/curl.h"

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15

/* SOURCES */
// Setup a windows hook:    https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexa
// Setup a callback:        https://learn.microsoft.com/en-us/windows/win32/winmsg/messageproc
// low & high bit set:      https://learn.microsoft.com/en-us/windows/win32/learnwin32/keyboard-input

/* GOOD TO KNOW */
// DWORD = unsigned long


// Global Variables
std::string gameTitle;
std::string characterName;
std::vector<std::string> sentence = {};
int currentWord = 0;
std::string typedWord;

bool sentenceStart = false;
DWORD firstSentencePressedTime = 0;
DWORD lastSentencePressedTime = 0;

bool firstWordStart = false;
DWORD firstKeyPressedTime = 0;
DWORD lastKeyPressedTime = 0;

std::vector<DWORD> timePerWord;

void SetColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void ClearLine() {
    std::cout << "\033[2K";  // ANSI escape code to clear the line
    std::cout << "\r";       // Carriage return to move the cursor to the beginning of the line
}

void ClearConsole(bool currentWordCorrect, bool displayInfo = false) {
    system("cls");  // Clear console
    for (int i = 0; i < sentence.size(); ++i) {
        if (i < currentWord) {
            SetColor(GREEN);  // Green for words before the current word
        }
        else if (i == currentWord) {
            // Set color based on whether the current word is correct
            SetColor(currentWordCorrect ? YELLOW : RED);  // Yellow for correct, Red for incorrect
        }
        else {
            SetColor(WHITE);  // White for words after the current word
        }
        std::cout << sentence[i] << " ";
    }
    std::cout << std::endl;
    SetColor(LIGHTGRAY);  // Reset to default color
    if (displayInfo) {
        if (!characterName.empty()) {
            std::cout << "- " << characterName;
        }
        if (!gameTitle.empty()) {
            std::cout << " : " << gameTitle << std::endl;
        }
    }
}

// Handling character translation
char GetCharFromVKCode(UINT vkCode, UINT scanCode, bool lowerCase) {
    BYTE keyboardState[256];
    GetKeyboardState(keyboardState);  // Get the state of all keys

    WCHAR charOutput[2] = { 0 };
    char resultChar = '\0';  // Default to null character if conversion fails

    // Convert virtual key code to Unicode character
    int result = ToUnicode(vkCode, scanCode, keyboardState, charOutput, 2, 0);

    if (result == 1) {
        // Successfully translated to a single Unicode character, now convert it to multi-byte character (ANSI)
        WideCharToMultiByte(CP_ACP, 0, charOutput, 1, &resultChar, 1, NULL, NULL);
    }
    else {
        resultChar = vkCode;
    }
    
    resultChar = lowerCase ? tolower(resultChar) : resultChar;

    return resultChar;
}

void StartTimers(DWORD pTime) {
    // Handle per word timing
    if (!firstWordStart) {
        firstKeyPressedTime = pTime;
        firstWordStart = true;
    }
    if (!sentenceStart) {
        firstSentencePressedTime = pTime;
        sentenceStart = true;
    }
}

LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        if (wParam == WM_KEYDOWN) {
            std::cout << GetCharFromVKCode(p->vkCode, p->scanCode, false);
            if ((p->vkCode >= 'A' && p->vkCode <= 'Z') || (p->vkCode == 188 || p->vkCode == 190 || p->vkCode == 191)) { // 188 = [,] 190 = [.]
                // Get current state of shift and caps
                bool shift_pressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; // high bit set indicates key is currently down (pressed)
                bool caps_active = (GetKeyState(VK_CAPITAL) & 0x0001) != 0; // low bit set indicates toggle state of key (caps, num lock, ...)
                // Check combination of shift + caps to determine lower case
                bool lowerCase = (!caps_active && !shift_pressed) || (caps_active && shift_pressed);

                // Convert to lower if needed
                char keyCode = GetCharFromVKCode(p->vkCode, p->scanCode, lowerCase);

                StartTimers(p->time);
                
                ClearLine(); // Clear current line to keep the sentence not flicker
                typedWord += keyCode;
                std::cout << typedWord;
            }
            else if ((p->vkCode == '1' || p->vkCode == 187)) { // 187 = [+] -> [Shift + 187] = [?]
                bool shift_pressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; // high bit set indicates key is currently 

                if (shift_pressed) {
                    // Convert to lower if needed
                    char keyCode = GetCharFromVKCode(p->vkCode == 187 ? '?' : '!', p->scanCode, false);

                    StartTimers(p->time);

                    ClearLine(); // Clear current line to keep the sentence not flicker
                    typedWord += keyCode;
                    std::cout << typedWord;
                }
            }
            else if (p->vkCode == VK_BACK) {
                if (!typedWord.empty()) {
                    typedWord.pop_back();
                    ClearLine(); // Clear current line to keep the sentence not flicker
                    std::cout << typedWord;
                }
            }
            else if (p->vkCode == VK_SPACE) {
                if (firstWordStart && p->vkCode == VK_SPACE) {
                    lastKeyPressedTime = p->time;
                    timePerWord.push_back((lastKeyPressedTime - firstKeyPressedTime));
                    firstWordStart = false;
                }
                if (typedWord == sentence[currentWord]) {
                    // Forward pointer to next word in sentence
                    currentWord++;
                    // Check if whole sentence is completed
                    if (currentWord == sentence.size()) {
                        std::cout << "Congratulations! You've typed the entire sentence correctly.\n";
                        ClearConsole(true, true);

                        lastSentencePressedTime = p->time;
                        firstWordStart = false;

                        PostQuitMessage(0); // Post a WM_QUIT message to the message queue
                        return 1;
                    }
                    typedWord.clear();
                    ClearConsole(true);
                }
                else {
                    ClearConsole(false);
                    std::cout << typedWord;
                }
            }
            else if (p->vkCode == VK_ESCAPE) {
                std::cout << std::endl << "Escape key pressed, exiting now." << std::endl;
                PostQuitMessage(0); // Post a WM_QUIT message to the message queue
                return 1;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

std::vector<std::string> ParseSentence(std::string str) {
    std::istringstream iss(str);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool SetupQuoteWithCURL(std::vector<std::string>& out_sentence, std::string& out_gameTitle, std::string& out_characterName) {
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
            return -1;
        }
        else {
            //// Extract the quote
            std::cout << readBuffer << std::endl;
            std::regex quote_regex("\"quote\":\"(.*?)\"");
            std::regex title_regex("\"title\":\"(.*?)\"");
            std::regex character_regex("\"character\":\"(.*?)\"");
            std::smatch match;

            //std::cout << readBuffer << std::endl;
            if (std::regex_search(readBuffer, match, quote_regex) && match.size() > 1) {
                sentence = ParseSentence(match[1].str());
            }
            else {
                std::cout << "No quote found!" << std::endl;
                return -1;
            }
            if (std::regex_search(readBuffer, match, title_regex) && match.size() > 1) {
                gameTitle = match[1].str();
            }
            else {
                gameTitle = "Title not found...";
            }
            if (std::regex_search(readBuffer, match, character_regex) && match.size() > 1) {
                characterName = match[1].str();
            }
            else {
                characterName = "Title not found...";
            }
        }

        curl_easy_cleanup(curl);
    }
}

int main() {
    // Setup quote with CURL and Regex
    SetupQuoteWithCURL(sentence, gameTitle, characterName);
    // Display logic
    ClearConsole(true);

    // Keyboard hooks and message processing
    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, MessageProc, 0, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(hhkLowLevelKybd);
    
    SetColor(CYAN);
    std::cout << std::endl << "Result: ";
    SetColor(LIGHTGRAY);
    // Calculate and display timing and WPM result
    if (!timePerWord.empty()) {
        double sum = std::accumulate(timePerWord.begin(), timePerWord.end(), 0.0);
        int averageTimePerWord = static_cast<int>(std::round(sum / timePerWord.size()));
        std::cout << std::endl << " - Your average time per word were " << averageTimePerWord << "ms";
    }

    double timeInSeconds = (lastSentencePressedTime - firstSentencePressedTime) / 1000.0;
    double timeInSecondsRounded = std::round(timeInSeconds * 10.0) / 10.0;  // Round to one decimal place

    std::cout << std::endl << " - The sentence took you " << std::fixed << std::setprecision(1) << timeInSecondsRounded << "s to complete!" << std::endl;

    if (timeInSeconds > 0) {
        int wordsPerMinute = static_cast<int>(std::round(sentence.size() / (timeInSeconds / 60.0)));
        std::cout << " - That corresponds to a WPM of " << wordsPerMinute << std::endl;
    }

    return 0;
}
