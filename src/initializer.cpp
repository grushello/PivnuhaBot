#include "initializer.hpp"
#include "user_handler.hpp"
#include "table_handler.hpp"
#include "time.hpp"
#include <iostream>

void initializeBot(
    TgBot::Bot& bot,
    std::vector<User>& users,
    std::vector<User>& barShift,
    std::vector<User>& kitchenShift
) {
    Time time;
    loadUsers(users);

    std::cout << "[INFO] Bot started. Loaded " << users.size() << " users.\n";

    if (!tableExists(time, BAR_TABLE)) {
        std::cout << "[INFO] No bar table found, creating a new one...\n";
        createEmptyTable(users, time, BAR_TABLE, NON_REWRITE);
    }
    if (!tableExists(time, KITCHEN_TABLE)) {
        std::cout << "[INFO] No kitchen table found, creating a new one...\n";
        createEmptyTable(users, time, KITCHEN_TABLE, NON_REWRITE);
    }

    setShift(users, barShift, kitchenShift, time);
}