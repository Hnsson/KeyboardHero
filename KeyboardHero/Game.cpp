#include "Game.h"

Game* Game::instance = nullptr;  // Initialize the static member

Game::Game()
{
    init();
}

Game::~Game()
{
    shutdown();
}

void Game::init()
{
    workerThread = std::thread(ContinuousFetch, std::ref(keepRunning), std::ref(quoteQueue));
    workerThread.detach();
    
    quoteQueue.wait_for_data();
    quoteQueue.try_pop(sentence);
    ClearConsole(std::get<0>(sentence), currentWord, true);
}

void Game::update()
{
    // Keyboard hooks and message processing
    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, MessageProc, 0, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(hhkLowLevelKybd);
}

void Game::shutdown()
{
    keepRunning = false;
    if (workerThread.joinable()) {
        workerThread.join();
        std::cout << "Thread was joined!" << std::endl;
    }
}

void Game::restart()
{
    Loading();

    // Add analytics from the quote
    if (!timePerWord.empty()) {
        double sum = std::accumulate(timePerWord.begin(), timePerWord.end(), 0.0);
        int averageTimePerWord = static_cast<int>(std::round(sum / timePerWord.size()));
        tpw.push_back(averageTimePerWord);
    }

    double timeInSeconds = (lastSentencePressedTime - firstSentencePressedTime) / 1000.0;
    double timeInSecondsRounded = std::round(timeInSeconds * 10.0) / 10.0;  // Round to one decimal place

    if (timeInSeconds > 0) {
        int wordsPerMinute = static_cast<int>(std::round(std::get<0>(sentence).size() / (timeInSeconds / 60.0)));
        wpm.push_back(wordsPerMinute);
    }
    //
    // Wait for data and then pop / push next quote to be showed
    quoteQueue.wait_for_data();
    quoteQueue.try_pop(sentence);

    // Reset everything for statistics
    currentWord = 0;
    typedWord = "";

    sentenceStart = false;
    firstSentencePressedTime = 0;
    lastSentencePressedTime = 0;

    firstWordStart = false;
    firstKeyPressedTime = 0;
    lastKeyPressedTime = 0;
    //

    // Clear console and show new game
    ClearConsole(std::get<0>(sentence), currentWord, true);
}

void Game::result()
{
    SetColor(CYAN);
    std::cout << std::endl << "Result for this session:" << std::endl;
    SetColor(LIGHTGRAY);

    std::cout << "These are the Words Per Minute (WPM) for each round:" << std::endl;
    for (int i = 0; i < wpm.size(); i++) {
        std::cout << "(" << i << ")" << "\t" << wpm[i] << std::endl;
    }
    std::cout << "The average WPM: ";
    SetColor(YELLOW);
    std::cout << calculateAverage(wpm) << std::endl << std::endl;
    SetColor(LIGHTGRAY);

    std::cout << "These are the your Time Per Words (TPW) for each round:" << std::endl;
    for (int i = 0; i < tpw.size(); i++) {
        std::cout << "(" << i << ")" << "\t" << tpw[i] << std::endl;
    }
    std::cout << "The average TPW: ";
    SetColor(YELLOW);
    std::cout << calculateAverage(tpw) << std::endl << std::endl;
    SetColor(LIGHTGRAY);
}

Game& Game::getInstance()
{
    if (instance == nullptr) {
        instance = new Game();
    }
    return *instance;
}

void Game::releaseInstance()
{
    delete instance;
    instance = nullptr;
}

LRESULT Game::MessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        if (wParam == WM_KEYDOWN) {
            instance->StartTimers(p->time);
            if (instance->IsAlphanumericOrPunctuation(p->vkCode)) {
                instance->ProcessKeyPress(p);
            }
            else {
                instance->HandleSpecialKeys(p);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool Game::IsAlphanumericOrPunctuation(UINT vkCode)
{
	return (vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9') ||
		(vkCode == VK_OEM_COMMA || vkCode == VK_OEM_PERIOD || vkCode == VK_OEM_MINUS || vkCode == VK_OEM_PLUS || vkCode == VK_OEM_2);
}

void Game::ProcessKeyPress(PKBDLLHOOKSTRUCT p)
{
    bool shift_pressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; // high bit set indicates key is currently down (pressed)
    bool caps_active = (GetKeyState(VK_CAPITAL) & 0x0001) != 0; // low bit set indicates toggle state of key (caps, num lock, ...)
    // Check combination of shift + caps to determine lower case
    bool lowerCase = (!caps_active && !shift_pressed) || (caps_active && shift_pressed);

    // Convert to lower if needed
    char keyCode = GetCharFromVKCode(p->vkCode, p->scanCode, lowerCase);

    ClearLine(); // Clear current line to keep the sentence not flicker
    typedWord += keyCode;
    std::cout << typedWord;
}

void Game::HandleSpecialKeys(PKBDLLHOOKSTRUCT p)
{
    switch (p->vkCode) {
    case VK_BACK:
        if (!typedWord.empty()) {
            typedWord.pop_back();
            ClearLine();
            std::cout << typedWord;
        }
        break;
    case VK_SPACE:
        ProcessSpaceKey(p);
        break;
    case VK_ESCAPE:
        PostQuitMessage(0);
        result();
        break;
    }
}

//ResultScreen(std::get<0>(sentence), characterName, gameTitle, firstSentencePressedTime, lastSentencePressedTime, timePerWord);
void Game::ProcessSpaceKey(PKBDLLHOOKSTRUCT p)
{
    if (firstWordStart && p->vkCode == VK_SPACE) {
        lastKeyPressedTime = p->time;
        timePerWord.push_back((lastKeyPressedTime - firstKeyPressedTime));
        firstWordStart = false;
    }
    if (typedWord == std::get<0>(sentence)[currentWord]) {
        // Forward pointer to next word in sentence
        currentWord++;
        // Check if whole sentence is completed
        if (currentWord == std::get<0>(sentence).size()) {
            lastSentencePressedTime = p->time;
            firstWordStart = false;
        
            restart();
            //PostQuitMessage(0); // Post a WM_QUIT message to the message queue
            return;
        }
        typedWord.clear();
        ClearConsole(std::get<0>(sentence), currentWord, true);
    }
    else {
        ClearConsole(std::get<0>(sentence), currentWord, false);
        std::cout << typedWord;
    }
}

void Game::StartTimers(DWORD pTime) {
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