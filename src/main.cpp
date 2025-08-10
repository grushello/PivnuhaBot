#include <vector>
#include <set>
#include <map>
#include <cstdlib>
#include <atomic>

#include "telegram_handler.hpp"
#include "user_handler.hpp"
#include "table_handler.hpp"
#include "time.hpp"
#include "feedback.hpp"
#include "initializer.hpp"
#include "command_handler.hpp"
#include "message_handler.hpp"
#include "utils.hpp"

#include "config.hpp"

const char* BOT_TOKEN = nullptr;
const char* PASSWORD  = nullptr;
std::atomic<bool> shouldRestart = false;

int main() {
    BOT_TOKEN = std::getenv("BOT_TOKEN");
    PASSWORD = std::getenv("BOT_PASSWORD");
     if (BOT_TOKEN) {
        std::cout << "BOT_TOKEN: is set" << std::endl;
    } else {
        std::cout << "BOT_TOKEN not set." << std::endl;
    }

    if (PASSWORD) {
        std::cout << "BOT_PASSWORD: " << PASSWORD << std::endl;
    } else {
        std::cout << "BOT_PASSWORD not set." << std::endl;
    }
    TgBot::Bot bot(BOT_TOKEN);

    std::vector<User> users;
    std::vector<User> barShift;
    std::vector<User> kitchenShift;
    std::set<int64_t> awaitingNameInput;
    std::set<int64_t> awaitingPasswordInput;
    std::set<int64_t> awaitingRoleInput;
    std::set<int64_t> awaitingAreaInput;
    std::set<int64_t> awaitingCustomCheckinInput;
    std::set<int64_t> awaitingCustomCheckoutInput;
    std::map<int64_t, std::string> pendingNames;

    std::map<int64_t, int> startAttempts;
    std::set<int64_t> blockedUsers;

    initializeBot(bot, users, barShift, kitchenShift);
    registerCommandHandler(bot, users, barShift, kitchenShift,
                     blockedUsers, awaitingPasswordInput, awaitingNameInput,
                     awaitingRoleInput, awaitingAreaInput,
                     awaitingCustomCheckinInput, awaitingCustomCheckoutInput,
                     pendingNames, startAttempts, shouldRestart);
    registerMessageHandler(bot, users, barShift, kitchenShift,
                       blockedUsers, awaitingPasswordInput, awaitingNameInput,
                       awaitingRoleInput, awaitingAreaInput,
                       awaitingCustomCheckinInput, awaitingCustomCheckoutInput,
                       pendingNames, PASSWORD);
    try
    {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);

        int lastProcessedHour = Time().hour - 1;
        std::string lastProcessedDay = readLastProcessedDay();
        std::string lastProcessedMonth = readLastProcessedMonth();
        
        bool backedUpToday = false;
        while (true)
        {
            if (shouldRestart)
            {
                std::cout << "[INFO] Restarting...\n";
                std::exit(0);
            }
            Time currentTime;
            if(currentTime.hour == 4 && !backedUpToday)
            {
                for(int i = 0; i < users.size(); ++i)
                {
                    if(users[i].role == DEVELOPER)
                    {
                        sendSafeMessage(bot, users[i].chatID, "Table backup for " + currentTime.ddmmyyyy());
                        sendTableDoc(bot, users[i], getTableFilename(currentTime, BAR_TABLE), currentTime, true, false);
                        sendTableDoc(bot, users[i], getTableFilename(currentTime, KITCHEN_TABLE), currentTime, false, false);
                        sendTableDoc(bot, users[i], "users.csv", currentTime, true, false);
                    }
                }
                backedUpToday = true;
            }
            if (currentTime.ddmmyyyy() != lastProcessedDay)
            {
                backedUpToday = false;
                lastProcessedDay = currentTime.ddmmyyyy();
                std::cout << "[INFO] New day detected: " << lastProcessedDay << " — resetting command states.\n";
                resetCommandsForAllUsers(bot, users);
                barShift.clear();
                kitchenShift.clear();
            }

            // Check for new month
            if (currentTime.mmyyyy() != lastProcessedMonth)
            {
                lastProcessedMonth = currentTime.mmyyyy();
                std::cout << "[INFO] New month detected: " << lastProcessedMonth << " — creating new table.\n";
                createEmptyTable(users, currentTime, BAR_TABLE, NON_REWRITE);
                createEmptyTable(users, currentTime, KITCHEN_TABLE, NON_REWRITE);
            }
            
            if(currentTime.hour != lastProcessedHour)
            {
                lastProcessedHour = currentTime.hour;
                saveLastProcessedDay(currentTime.ddmmyyyy());
            }
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgBot::TgException &e)
    {
        printf("error: %s\n", e.what());
    } catch (const std::exception &e) {
        std::cerr << "[UNCAUGHT STD EXCEPTION] " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[UNKNOWN ERROR] Something crashed the bot.\n";
    }
    return 1;
}