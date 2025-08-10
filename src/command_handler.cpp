#include "command_handler.hpp"
#include "user_handler.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "feedback.hpp"

void registerCommandHandler(TgBot::Bot& bot,
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
                      std::map<int64_t, int>& startAttempts,
                      std::atomic<bool>& shouldRestart) 
{
        bot.getEvents().onCommand("start", [&bot, &users, &awaitingPasswordInput, &startAttempts, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateNewUser(bot, user, users, blockedUsers, time))
            return;
        int &attempts = startAttempts[message->chat->id];
        attempts++;
        if(attempts > ALLOWED_START_ATTEMPTS)
        {
            std::cout << "User " << message->chat->id << " blocked after " << attempts << " /start attempts.\n";
            sendSafeMessage(bot, message->chat->id, "Too many start attempts, You've been blocked!");
            blockedUsers.insert(message->chat->id);
        }
        std::cout << "/start request was sent from user: " << message->chat->id << "\nExecuting request...\n";

        sendSafeMessage(bot, message->chat->id, "👋Hi!");
        sendSafeMessage(bot, message->chat->id, "🔐 Please enter the password!");
            
        awaitingPasswordInput.insert(message->chat->id);
    });
    bot.getEvents().onCommand("checkin", [&bot, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        if(!validateUserRole(bot, user, USER_ROLE::EMPLOYEE, time))
            return;
        std::cout << "/checkin request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        EXIT_CODE code = tableCheckin(user, time);
        if(code == CHECKIN_SUCCESS)
        {
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::CHECKED_IN);
            if(user.area == BAR)
                barShift.push_back(user);
            else 
                kitchenShift.push_back(user);
            notifyManagerCheckin(bot, users, user, time);
        }
        logCode(user, code);
        feedbackCode(bot, user, code, time);
    });
    bot.getEvents().onCommand("customcheckin", [&bot, &users, &awaitingCustomCheckinInput, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        if(!validateUserRole(bot, user, USER_ROLE::EMPLOYEE, time))
            return;

        std::cout << "/customcheckin request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        sendSafeMessage(bot, user.chatID, "🕐 Please, enter your check-in time");
        awaitingCustomCheckinInput.insert(user.chatID);
    });
    bot.getEvents().onCommand("checkout", [&bot, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message)
    {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        if(!validateUserRole(bot, user, USER_ROLE::EMPLOYEE, time))
            return;

        std::cout << "/checkout request was sent from user: " << user.chatID
                << " - " << user.name << "\nExecuting request...\n";

        auto [code, inTime] = tableCheckout(user, time);

        if (code == CHECKOUT_SUCCESS) 
        {
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::CHECKED_OUT);
            removeUserFromShift(user, barShift);
            removeUserFromShift(user, kitchenShift);

            notifyManagerCheckout(bot, users, user, time);
        }

        logCode(user, code);

        feedbackCode(bot, user, code, time, inTime); 
    });
    bot.getEvents().onCommand("customcheckout", [&bot, &users, &awaitingCustomCheckoutInput, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        if(!validateUserRole(bot, user, USER_ROLE::EMPLOYEE, time))
            return;

        std::cout << "/customCheckout request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        sendSafeMessage(bot, user.chatID, "🕐 Please, enter your check-out time");
        awaitingCustomCheckoutInput.insert(user.chatID);
    });
    bot.getEvents().onCommand("cancel", [&bot, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        if(!validateUserRole(bot, user, USER_ROLE::EMPLOYEE, time))
            return;
            
        std::cout << "/cancel request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        EXIT_CODE code = cancelTableCheckin(user, time);
        if(code == CANCEL_SUCCESS || code == NOTHING_TO_CANCEL)
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
        if(code == CANCEL_SUCCESS)
        {
            removeUserFromShift(user, barShift);
            removeUserFromShift(user, kitchenShift);
            notifyManagerCancel(bot, users, user);
        }
        logCode(user, code);
        feedbackCode(bot, user, code, time);

    });
    bot.getEvents().onCommand("tablebar", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        std::cout << "/tablebar request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        sendTableDoc(bot, user, getTableFilename(time, BAR_TABLE), time, true, false);
    });
    bot.getEvents().onCommand("tablekitchen", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
        if(blockedUsers.count(message->chat->id))
            return;

        Time time;
        User user = findUser(users, message->chat->id);
        if(user.chatID == 0)
        {
            user = User();
            logCode(user, USER_NOT_FOUND);
            return;
        }
        std::cout << "/tablekitchen request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        sendTableDoc(bot, user, getTableFilename(time, KITCHEN_TABLE), time, false, false);
    });
    bot.getEvents().onCommand("lastmonthbar", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;

        std::cout << "/lastmonthbar request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        sendTableDoc(bot, user, getLastMonthTableFilename(time, BAR_TABLE), time, true, true);
    });
    bot.getEvents().onCommand("lastmonthkitchen", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;

        std::cout << "/lastmonthkitchen request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        sendTableDoc(bot, user, getLastMonthTableFilename(time, KITCHEN_TABLE), time, false, true);
    });
    bot.getEvents().onCommand("help", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;

        std::cout << "/help request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        listAvailableCommands(bot, user);
    });
    bot.getEvents().onCommand("currentshift", [&bot, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message) {
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;

        std::cout << "/currentshift request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        listCurrentShift(bot, barShift, kitchenShift, user);
    });
    bot.getEvents().onCommand("restart", [&bot, &users, &blockedUsers, &shouldRestart](TgBot::Message::Ptr message){
        Time time;
        User user = findUser(users, message->chat->id);
        if(!validateUser(bot, user, users, blockedUsers, time))
            return;
        if(!validateUserRole(bot, user, USER_ROLE::DEVELOPER, time))
            return;

        std::cout << "/restart request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        sendSafeMessage(bot, user.chatID, "♻️ Restarting bot...");
        std::cout << "[INFO] Bot is restarting by developer request\n";

        shouldRestart = true;
    });
}