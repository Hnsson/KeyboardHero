#pragma once

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

inline char GetCharFromVKCode(UINT vkCode, UINT scanCode, bool lowerCase) {
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

inline void SetColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

inline void ClearLine() {
    std::cout << "\033[2K";  // ANSI escape code to clear the line
    std::cout << "\r";       // Carriage return to move the cursor to the beginning of the line
}

inline void ClearConsole(std::vector<std::string> sentence, int currentWord, bool currentWordCorrect, int multiplier, int total_score) {
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
    std::cout << std::endl << std::endl;
    SetColor(LIGHTGRAY);  // Reset to default color
    std::cout << "Current multiplier: ";
    SetColor(YELLOW);
    std::cout << (multiplier + 1) << std::endl;
    SetColor(LIGHTGRAY);
    std::cout << "Current score: ";
    SetColor(YELLOW);
    std::cout << static_cast<int>(total_score) << std::endl;
    std::cout << std::endl;
    SetColor(LIGHTGRAY);
}

inline void Loading() {
    system("cls");
    std::cout << "Loading...";
}

inline void ResultScreen(std::vector<std::string> sentence, std::string characterName, std::string gameTitle, DWORD firstSentencePressedTime, DWORD lastSentencePressedTime, std::vector<DWORD>& timePerWord) {
    system("cls");
    for (int i = 0; i < sentence.size(); ++i) {
        SetColor(GREEN);  // Green for words before the current word
        std::cout << sentence[i] << " ";
    }
    SetColor(CYAN);
    std::cout << "Result:" << std::endl;
    SetColor(LIGHTGRAY);
    if (!characterName.empty()) {
        std::cout << "- " << characterName;
    }
    if (!gameTitle.empty()) {
        std::cout << " : " << gameTitle << std::endl;
    }


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

}

inline std::vector<std::string> ParseSentence(std::string str) {
    std::istringstream iss(str);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

inline double calculateAverage(const std::vector<int>& v) {
    if (v.empty()) return 0; // Handle the case where the vector is empty

    int sum = 0;
    for (int num : v) {
        sum += num;
    }
    return std::round((static_cast<double>(sum) / v.size()) * 10) / 10.0;
}