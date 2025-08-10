#pragma once

#include <string>
#include <atomic>
#include <table_handler.hpp>

extern const char* BOT_TOKEN;
extern const char* PASSWORD;

constexpr int ALLOWED_START_ATTEMPTS = 8;

constexpr const char* LAST_PROCESSED_DAY_FILE = "lastProcessedDay.txt";
constexpr const char* USERS_FILE              = "users.csv";

constexpr int BACKUP_HOUR = 4;

extern std::atomic<bool> shouldRestart;