#pragma once
#include <vector>
#include "user.hpp"
#include "tgbot/tgbot.h"

void initializeBot(
    TgBot::Bot& bot,
    std::vector<User>& users,
    std::vector<User>& barShift,
    std::vector<User>& kitchenShift
);