#pragma once
#include "table_handler.hpp"
#include "telegram_handler.hpp"

void logCode(const User& user, EXIT_CODE code);
void feedbackCode(TgBot::Bot &bot, const User& user, EXIT_CODE code, const Time& time = Time(), const Time& inTime = Time());