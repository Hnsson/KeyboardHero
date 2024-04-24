#pragma once

#include <iostream>
#include <windows.h>
#include <vector>
#include <sstream>

#include <numeric>
#include <stdio.h>
#include <cmath>
#include <iomanip>

#include "Helpers.h"
#include "ThreadQueue.h"
#include "FetchQuote.h"

class Game
{
public:
	Game();
	~Game();

	static LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam);
	static Game* instance;
	
	void init();
	void update();
	void shutdown();
	void restart();
	void result();

	static Game& getInstance();
	static void releaseInstance();
private:
	std::thread workerThread;
	std::atomic<bool> keepRunning = true;
	ThreadSafeQueue<std::tuple<std::vector<std::string>, std::string, std::string>> quoteQueue;

	std::string gameTitle;
	std::string characterName;
	std::tuple<std::vector<std::string>, std::string, std::string> sentence;
	int currentWord = 0;
	std::string typedWord;

	bool sentenceStart = false;
	DWORD firstSentencePressedTime = 0;
	DWORD lastSentencePressedTime = 0;

	bool firstWordStart = false;
	DWORD firstKeyPressedTime = 0;
	DWORD lastKeyPressedTime = 0;

	std::vector<DWORD> timePerWord;

	std::vector<int> wpm;
	std::vector<int> tpw;

	bool IsAllowedKey(UINT vkCode);
	void ProcessSpaceKey(PKBDLLHOOKSTRUCT p);
	void HandleSpecialKeys(PKBDLLHOOKSTRUCT p);
	void ProcessKeyPress(PKBDLLHOOKSTRUCT p);
	void StartTimers(DWORD pTime);
};

