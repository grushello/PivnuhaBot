#pragma once
#include <tgbot/tgbot.h>
#include <user.hpp>
#include <Time.hpp>
#include <vector>

enum COMMAND_STATE {DEFAULT_COMMAND_STATE, CHECKED_IN, CHECKED_OUT, MANAGER_COMMAND_STATE, DEVELOPER_COMMAND_STATE};
void sendSafeMessage(TgBot::Bot &bot, int64_t chatID, const std::string& message);
void listAvailableCommands(TgBot::Bot &bot, const User& user);
void setCommandsForUser(TgBot::Bot& bot, int64_t chatID, COMMAND_STATE state);
bool sendTableDoc(const TgBot::Bot& bot, const User& user, const std::string& filepath, const Time& time, bool isBarTable, bool isPreviousMonth);
void listCurrentShift(TgBot::Bot& bot, const std::vector<User>& barShift, const std::vector<User>& kitchenShift, const User& user);
void notifyManagerCheckin(TgBot::Bot& bot, const std::vector<User>& users, const User& user, const Time& time);
void notifyManagerCheckout(TgBot::Bot& bot, const std::vector<User>& users, const User& user, const Time& time);
void notifyManagerCancel(TgBot::Bot& bot, const std::vector<User>& users, const User& user);