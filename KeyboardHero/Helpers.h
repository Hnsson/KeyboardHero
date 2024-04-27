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

inline void Loading() {
    system("cls");
    std::cout << "Loading...";
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