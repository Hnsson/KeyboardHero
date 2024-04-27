#include "Game.h"

Game* Game::instance = nullptr;  // Initialize the static member

Game::Game() {}

Game::~Game()
{
    shutdown();
}

void Game::init()
{
    workerThread = std::thread(&Game::t_ContinuousFetch, this, std::ref(keepRunning), std::ref(quoteQueue));
    
    quoteQueue.wait_for_data();
    quoteQueue.try_pop(sentence);

    if (!menu()) return;
    ClearConsole(std::get<0>(sentence), currentWord, true, this->current_score_multiplier, this->total_score);
    update();
}

void Game::update()
{
    // Keyboard hooks and message processing
    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, MessageProc, 0, 0);
    MSG msg;

    while (!timeExpired) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (timerStarted) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime);

            if (elapsedTime.count() >= this->game_length_min) {
                timeExpired = true;
            }
        }
    }
    UnhookWindowsHookEx(hhkLowLevelKybd);
    result();
}

void Game::shutdown()
{
    SetColor(DARKGRAY);
    std::cout << "Waiting for thread to join..." << std::endl;
    keepRunning = false; // Tell the threads they are done working then...
    cv.notify_all(); // "Wake" up threads to the bad news :(

    if (workerThread.joinable()) {
        workerThread.join();
        std::cout << "Worker thread was joined!" << std::endl;
    }
    SetColor(WHITE);
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
    ClearConsole(std::get<0>(sentence), currentWord, true, this->current_score_multiplier, this->total_score);
}

void Game::result()
{
    SetColor(CYAN);
    std::cout << std::endl << std::endl << "Result for this session:" << std::endl;
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

bool Game::menu()
{
    SetColor(CYAN);  // Set a pleasant color for the title
    std::cout << "================ Keyboard Hero ================\n" << std::endl;
    SetColor(WHITE);  // Default color for the instructions

    std::cout << "Welcome to Keyboard Hero!\n\n"
        << "Instructions:\n"
        << "1. A series of video game quotes will appear on the screen.\n"
        << "2. Type them correctly to gain points. And plenty correct in a sequence earns you a score multiplier!\n"
        << "3. Your results will be displayed at the end of each game session.\n"
        << "4. You can exit the game anytime by pressing the ESC button.\n\n"
        << "Press any key to start the game...\n" << std::endl;

    // Wait for any key press to continue
    int ch = _getch();

    if (ch == 27) {  // Check if the key is the Escape key
        std::cout << "Exiting game..." << std::endl;
        return false;
    }
    return true;
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
            if (instance->IsAllowedKey(p->vkCode)) {
                instance->ProcessKeyPress(p);
            }
            else {
                instance->HandleSpecialKeys(p);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool Game::IsAllowedKey(UINT vkCode)
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
        timeExpired = true;
        break;
    }
}

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
        this->current_word_hit++;

        if (currentWord == std::get<0>(sentence).size()) { // sentence is completed queue next quote!
            lastSentencePressedTime = p->time;
            firstWordStart = false;
        
            restart();
            cv.notify_all(); // "Wake" threads up to check if the queue can be filled with quotes again
            return;
        }
        typedWord.clear();
        this->total_score += calculateScore(this->base_score, (lastKeyPressedTime - firstKeyPressedTime));
        ClearConsole(std::get<0>(sentence), currentWord, true, this->current_score_multiplier, this->total_score);
    }
    else {
        this->current_word_hit = 0;
        this->current_score_multiplier = 0;

        ClearConsole(std::get<0>(sentence), currentWord, false, this->current_score_multiplier, this->total_score);
        std::cout << typedWord;
    }
}

void Game::StartTimers(DWORD pTime) {
    // Handle per word timing
    if (!firstWordStart) {
        firstKeyPressedTime = pTime;
        firstWordStart = true;
    }
    // Start the time for a sentence
    if (!sentenceStart) {
        firstSentencePressedTime = pTime;
        sentenceStart = true;
    }
    // Start the high-resolution timer at the first key press
    if (!timerStarted) {
        startTime = std::chrono::steady_clock::now();
        timerStarted = true;
    }
}

void Game::t_ContinuousFetch(std::atomic<bool>& running, ThreadSafeQueue<std::tuple<std::vector<std::string>, std::string, std::string>>& queue)
{
    std::unique_lock<std::mutex> lk(mtx, std::defer_lock);

    while (running) {
        lk.lock();
        cv.wait(lk, [&]() { return queue.size() < 10 || !running; });

        if (!running) break;

        // Proceed with fetching
        std::vector<std::string> out_sentence;
        std::string out_gameTitle, out_characterName;
        if (SetupQuoteWithCURL(out_sentence, out_gameTitle, out_characterName)) {
            queue.push(std::make_tuple(out_sentence, out_gameTitle, out_characterName));
        }
        lk.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Regular interval
    }
}

double Game::calculateScore(double base_score, DWORD time_in_ms)
{
    double time_in_sec = time_in_ms / 1000.f;

    if (time_in_sec == 0) return 0;

    return (base_score / time_in_sec)*(calculateScoreMultiplier() + 1);
}

int Game::calculateScoreMultiplier()
{
    switch (current_word_hit) {
        case 8:
            this->current_score_multiplier = 1;
        break;
        case 12:
            this->current_score_multiplier = 2;
        break;
        case 16:
            this->current_score_multiplier = 3;
        break;
        case 20:
            this->current_score_multiplier = 4;
        break;
        default:
        break;
    }

    return this->current_score_multiplier;
}
