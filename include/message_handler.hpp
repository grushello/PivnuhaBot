#pragma once
#include "telegram_handler.hpp"
#include <set>
#include <map>
#include <vector>
#include <atomic>

void registerMessageHandler(TgBot::Bot& bot,
                            std::vector<User>& users,
                            std::vector<User>& barShift,
                            std::vector<User>& kitchenShift,
                            std::set<int64_t>& blockedUsers,
                            std::set<int64_t>& awaitingPasswordInput,
                            std::set<int64_t>& awaitingNameInput,
                            std::set<int64_t>& awaitingRoleInput,
                            std::set<int64_t>& awaitingAreaInput,
                            std::set<int64_t>& awaitingCustomCheckinInput,
                            std::set<int64_t>& awaitingCustomCheckoutInput,
                            std::map<int64_t, std::string>& pendingNames,
                            const char* PASSWORD);