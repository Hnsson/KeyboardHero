#pragma once

#include <iostream>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <sstream>
#include <conio.h>
#include <fstream>
#include <filesystem>

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
	bool menu();

	static Game& getInstance();
	static void releaseInstance();
private:
	// Thread properties
	std::thread workerThread;
	std::mutex mtx;
	std::condition_variable cv;
	bool timeExpired{ false };
	std::atomic<bool> keepRunning{ true };
	ThreadSafeQueue<std::tuple<std::vector<std::string>, std::string, std::string>> quoteQueue;

	// Game properties
	std::string gameTitle;
	std::string characterName;
	std::tuple<std::vector<std::string>, std::string, std::string> sentence;
	int currentWord = 0;
	std::string typedWord;

	bool currentWordCorrect = true;
	bool needRedraw = true;

	// Timers
	double game_length_min = 10;

	std::chrono::steady_clock::time_point startTime;
	bool timerStarted = false;  // New flag to check if timer has started

	bool sentenceStart = false;
	DWORD firstSentencePressedTime = 0;
	DWORD lastSentencePressedTime = 0;

	bool firstWordStart = false;
	DWORD firstKeyPressedTime = 0;
	DWORD lastKeyPressedTime = 0;

	// Scoring systems
	std::vector<DWORD> timePerWord;
	int base_score = 5;
	int current_score_multiplier = 0;
	double total_score = 0;

	int current_word_hit = 0;
	int total_word_hit = 0;

	std::vector<int> wpm;
	std::vector<int> tpw;

	// Game specific helper functions
	void ClearConsole();
	void DisplayScoreInfo();
	void DisplayCurrentTyping();
	void SaveProgress(std::string folder_path, double total_wpm);

	bool IsAllowedKey(UINT vkCode);
	void ProcessSpaceKey(PKBDLLHOOKSTRUCT p);
	void HandleSpecialKeys(PKBDLLHOOKSTRUCT p);
	void ProcessKeyPress(PKBDLLHOOKSTRUCT p);
	void StartTimers(DWORD pTime);

	// Thread handling
	void t_ContinuousFetch(std::atomic<bool>& running, ThreadSafeQueue<std::tuple<std::vector<std::string>, std::string, std::string>>& queue);
	//
	double calculateScore(double base_score, DWORD time_in_ms);
	int calculateScoreMultiplier();
};

