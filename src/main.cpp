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

const int ALLOWED_START_ATTEMPTS = 8;
const char* BOT_TOKEN;
const char* PASSWORD;
std::atomic<bool> shouldRestart = false;

void saveLastProcessedDay(const std::string& ddmmyyyy)
{
    try
    {
        std::ofstream outFile("lastProcessedDay.txt");
        if (outFile)
        {
            outFile << ddmmyyyy;
        }
    }
    catch(...)
    {
        std::cout << "couldn't write last processed day to a file\n";
    }
}
std::string readLastProcessedDay()
{
    Time time;
    try
    {
        std::ifstream inFile("lastProcessedDay.txt");
        std::string date;

        if (inFile)
        {
            std::getline(inFile, date);
            return date;
        }
        return time.ddmmyyyy();
    }
    catch (...)
    {
        return time.ddmmyyyy();
    }
}
std::string readLastProcessedMonth()
{
    Time time;
    try
    {
        std::ifstream inFile("lastProcessedDay.txt");
        std::string date;

        if (inFile)
        {
            std::getline(inFile, date);
            if (date.size() >= 3)
                return date.substr(3); // skip "dd."
        }
        return time.ddmmyyyy().substr(3);
    }
    catch (...)
    {
        return time.ddmmyyyy().substr(3);
    }
}
void init(TgBot::Bot &bot, std::vector<User> &users)
{
    Time time;
    loadUsers(users);
    std::cout << "The bot started. There are " << users.size() << " users\n";
    if(!tableExists(time, BAR_TABLE))
    {
        std::cout << "There was no bar table found on initialization\nCreating an empty new one\n";
        createEmptyTable(users, time, BAR_TABLE, NON_REWRITE);
        for(int i = 0; i < users.size(); ++i)
        {
            setCommandsForUser(bot, users[i].chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
        }
    }
    if(!tableExists(time, KITCHEN_TABLE))
    {
        std::cout << "There was no kitchen table found on initialization\nCreating an empty new one\n";
        createEmptyTable(users, time, KITCHEN_TABLE, NON_REWRITE);
        for(int i = 0; i < users.size(); ++i)
        {
            setCommandsForUser(bot, users[i].chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
        }
    }
}

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

    init(bot, users);
    bot.getEvents().onCommand("start", [&bot, &users, &awaitingPasswordInput, &startAttempts, &blockedUsers](TgBot::Message::Ptr message) {
        if(blockedUsers.count(message->chat->id))
            return;

        User user = findUser(users, message->chat->id);
        Time time;
        if(user.chatID != 0)
        {
            logCode(user, MULTIPLE_REGISTRATION);
            feedbackCode(bot, user, MULTIPLE_REGISTRATION, time);
            return;
        }

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
        if(blockedUsers.count(message->chat->id))
            return;

        User user = findUser(users, message->chat->id);
        Time time;
        if(user.chatID == 0)
        {
            user = User();
            logCode(user, USER_NOT_FOUND);
            return;
        }
        if(user.role != USER_ROLE::EMPLOYEE)
        {
            logCode(user, NO_PERMISSION);
            feedbackCode(bot, user, NO_PERMISSION, time);
            return;
        }
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
        if(blockedUsers.count(message->chat->id))
            return;

        User user = findUser(users, message->chat->id);
        Time time;
        if(user.chatID == 0)
        {
            user = User();
            logCode(user, USER_NOT_FOUND);
            return;
        }
        if(user.role != USER_ROLE::EMPLOYEE)
        {
            logCode(user, NO_PERMISSION);
            feedbackCode(bot, user, NO_PERMISSION, time);
            return;
        }
        std::cout << "/customcheckin request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        sendSafeMessage(bot, user.chatID, "🕐 Please, enter your check-in time");
        awaitingCustomCheckinInput.insert(user.chatID);
    });
    bot.getEvents().onCommand("checkout", [&bot, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message)
    {
        if(blockedUsers.count(message->chat->id))
            return;
        User user = findUser(users, message->chat->id);
        Time currentTime;

        if (user.chatID == 0)
        {
            user = User();
            logCode(user, USER_NOT_FOUND);
            feedbackCode(bot, user, USER_NOT_FOUND, currentTime);
            return;
        }
        if(user.role != USER_ROLE::EMPLOYEE)
        {
            logCode(user, NO_PERMISSION);
            feedbackCode(bot, user, NO_PERMISSION, currentTime);
            return;
        }
        std::cout << "/checkout request was sent from user: " << user.chatID
                << " - " << user.name << "\nExecuting request...\n";

        auto [code, inTime] = tableCheckout(user, currentTime);

        if (code == CHECKOUT_SUCCESS) 
        {
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::CHECKED_OUT);
            auto it = std::remove_if(barShift.begin(), barShift.end(), [&](const User& u) {
                return u.chatID == user.chatID;
            });
            barShift.erase(it, barShift.end());
            
            it = std::remove_if(kitchenShift.begin(), kitchenShift.end(), [&](const User& u) {
                return u.chatID == user.chatID;
            });
            kitchenShift.erase(it, kitchenShift.end());

            notifyManagerCheckout(bot, users, user, currentTime);
        }

        logCode(user, code);

        feedbackCode(bot, user, code, currentTime, inTime); 
    });
    bot.getEvents().onCommand("customcheckout", [&bot, &users, &awaitingCustomCheckoutInput, &blockedUsers](TgBot::Message::Ptr message) {
        if(blockedUsers.count(message->chat->id))
            return;

        User user = findUser(users, message->chat->id);
        Time time;
        if(user.chatID == 0)
        {
            user = User();
            logCode(user, USER_NOT_FOUND);
            return;
        }
        if(user.role != USER_ROLE::EMPLOYEE)
        {
            logCode(user, NO_PERMISSION);
            feedbackCode(bot, user, NO_PERMISSION, time);
            return;
        }
        std::cout << "/customCheckout request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        sendSafeMessage(bot, user.chatID, "🕐 Please, enter your check-out time");
        awaitingCustomCheckoutInput.insert(user.chatID);
    });
    bot.getEvents().onCommand("cancel", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
        if(blockedUsers.count(message->chat->id))
            return;

        User user = findUser(users, message->chat->id);
        Time time;
        if(user.chatID == 0)
        {
            user = User();
            logCode(user, USER_NOT_FOUND);
            return;
        }
        if(user.role != USER_ROLE::EMPLOYEE)
        {
            logCode(user, NO_PERMISSION);
            feedbackCode(bot, user, NO_PERMISSION, time);
            return;
        }
        std::cout << "/cancel request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        EXIT_CODE code = cancelTableCheckin(user, time);
        if(code == CANCEL_SUCCESS || code == NOTHING_TO_CANCEL)
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
        if(code == CANCEL_SUCCESS)
            notifyManagerCancel(bot, users, user);
        logCode(user, code);
        feedbackCode(bot, user, code, time);

    });
    bot.getEvents().onCommand("tablebar", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
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
        std::cout << "/lastmonthbar request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        sendTableDoc(bot, user, getLastMonthTableFilename(time, BAR_TABLE), time, true, true);
    });
    bot.getEvents().onCommand("lastmonthkitchen", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
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
        std::cout << "/lastmonthkitchen request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        sendTableDoc(bot, user, getLastMonthTableFilename(time, KITCHEN_TABLE), time, false, true);
    });
    bot.getEvents().onCommand("help", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message) {
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
        std::cout << "/help request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        listAvailableCommands(bot, user);
    });
    bot.getEvents().onCommand("currentshift", [&bot, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message) {
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
        std::cout << "/currentshift request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";
        listCurrentShift(bot, barShift, kitchenShift, user);
    });
    bot.getEvents().onCommand("restart", [&bot, &users, &blockedUsers](TgBot::Message::Ptr message){
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
        if (user.role != USER_ROLE::DEVELOPER) 
        {
            sendSafeMessage(bot, user.chatID, "❌ You do not have permission to restart the bot.");
            return;
        }
        
        std::cout << "/restart request was sent from user: " << user.chatID
        << " - " << user.name << "\nExecuting request...\n";

        sendSafeMessage(bot, user.chatID, "♻️ Restarting bot...");
        std::cout << "[INFO] Bot is restarting by developer request\n";

        shouldRestart = true;
    });

    bot.getEvents().onAnyMessage([&bot, &awaitingNameInput, &awaitingPasswordInput, &awaitingRoleInput, &awaitingAreaInput, &pendingNames,
        &awaitingCustomCheckinInput, &awaitingCustomCheckoutInput, &users, &blockedUsers, &barShift, &kitchenShift](TgBot::Message::Ptr message)
    {
        if(blockedUsers.count(message->chat->id))
            return;

        if (awaitingPasswordInput.count(message->from->id))
        {
            if(message->text == PASSWORD)
            {
                std::cout << "user " << message->from->id << " succesfully entered the password\n";
                sendSafeMessage(bot, message->chat->id, "✅Password is correct");
                sendSafeMessage(bot, message->chat->id, "In order to register, enter your name, please!");
                awaitingNameInput.insert(message->chat->id);
            }
            else 
            {
                sendSafeMessage(bot, message->chat->id, "❌Incorrect password, check for any mistakes and /start again!");
                std::cout << "user " << message->from->id << " entered the wrong password\n";
            }
            awaitingPasswordInput.erase(message->chat->id);
        }
        else if (awaitingNameInput.count(message->from->id)) 
        {

            if (message->text.size() == 0 || message->text.size() > 40
            || message->text.find_first_of(';') != std::string::npos
            || message->text.find_first_of('/') != std::string::npos)
            {
                sendSafeMessage(bot, message->chat->id, "⚠️ Invalid name! Please /start again and make sure that there are no '/' and ';' symbols in the name");
                std::cout << "user " << message->from->id << " Entered invalid name\n";
                awaitingNameInput.erase(message->chat->id);
                return;
            }
            if (nameIsRegistered(users, message->text))
            {
                sendSafeMessage(bot, message->chat->id, "⚠️ This name is already taken! Please, enter different name! Please /start again");
                std::cout << "user " << message->from->id << " Entered incorrect name\n";
                awaitingNameInput.erase(message->chat->id);
                return;
            }
            pendingNames[message->chat->id] = message->text;
            awaitingNameInput.erase(message->chat->id);
            awaitingRoleInput.insert(message->chat->id);
            sendSafeMessage(bot, message->chat->id, "Now choose your role: send 'manager' or 'employee'");
        }
        else if(awaitingRoleInput.count(message->from->id)) {
            std::string text = message->text;
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
            if(text != "manager" && text != "employee") 
            {
                sendSafeMessage(bot, message->chat->id, "⚠️Invalid role! Please /start again and type either 'manager' or 'employee' at this stage.");
                awaitingRoleInput.erase(message->from->id);
                return;
            }
            USER_ROLE role = (text == "manager") ? USER_ROLE::MANAGER : USER_ROLE::EMPLOYEE;

            if(role == USER_ROLE::EMPLOYEE)
            {
                sendSafeMessage(bot, message->chat->id, "🙌 Almost done! Now choose your working area: send 'bar' or 'kitchen'");
                awaitingRoleInput.erase(message->chat->id);
                awaitingAreaInput.insert(message->chat->id);
                return;
            }
            User user(pendingNames[message->from->id], message->from->id, role, DEFAULT_AREA);

            saveUser(user);
            setCommandsForUser(bot, message->chat->id, user.role == EMPLOYEE? DEFAULT_COMMAND_STATE 
                : user.role == DEVELOPER? DEVELOPER_COMMAND_STATE 
                : MANAGER_COMMAND_STATE);
            sendSafeMessage(bot, message->chat->id, "🎉 Registration complete! Thanks, " + user.name + "! You are registered as a " + text + ".");
            std::cout << "User " << user.name << " registered with role: " << text << "\n";
            awaitingRoleInput.erase(message->chat->id);
            users.push_back(user);
        }
        else if(awaitingAreaInput.count(message->from->id))
        {
            std::string text = message->text;
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
            if(text != "bar" && text != "kitchen") 
            {
                sendSafeMessage(bot, message->chat->id, "⚠️Invalid area! Please /start again and type either 'kitchen' or 'bar' at this stage.");
                awaitingAreaInput.erase(message->from->id);
                return;
            }
            WORKING_AREA area = (text == "bar") ? WORKING_AREA::BAR : WORKING_AREA::KITCHEN;
            User user(pendingNames[message->from->id], message->from->id, EMPLOYEE, area);

            saveUser(user);
            if(area == BAR)
                addUserToTable(user, BAR_TABLE, Time());
            else
                addUserToTable(user, KITCHEN_TABLE, Time());

            setCommandsForUser(bot, message->chat->id, DEFAULT_COMMAND_STATE);
            sendSafeMessage(bot, message->chat->id, "🎉 Registration complete! Thanks, " + user.name + "! You are registered as an " + userRoleToString(user.role) + ".");
            std::cout << "User " << user.name << " registered with role: " << text << "\n";
            awaitingAreaInput.erase(message->chat->id);
            pendingNames.erase(message->chat->id);
            users.push_back(user);
        }
        else if(awaitingCustomCheckinInput.count(message->from->id))
        {
            User user = findUser(users, message->from->id);
            if(!Time::isValidTime(message->text))
            {
                awaitingCustomCheckinInput.erase(message->from->id);
                logCode(user, INVALID_CHECKIN_TIME_INPUT);
                feedbackCode(bot, user, INVALID_CHECKIN_TIME_INPUT);
                return;
            }
            Time time(message->text);
            EXIT_CODE code = tableCheckin(user, time);
            if (code == CHECKIN_SUCCESS)
            {
                setCommandsForUser(bot, user.chatID, COMMAND_STATE::CHECKED_IN);
                if(user.area == BAR)
                    barShift.push_back(user);
                else 
                    kitchenShift.push_back(user);
                notifyManagerCheckin(bot, users, user, time);
            }
            awaitingCustomCheckinInput.erase(message->from->id);
            logCode(user, code);
            feedbackCode(bot, user, code, time);
        }
        else if(awaitingCustomCheckoutInput.count(message->from->id))
        {
            User user = findUser(users, message->from->id);
            if(!Time::isValidTime(message->text))
            {
                awaitingCustomCheckoutInput.erase(message->from->id);
                logCode(user, INVALID_CHECKOUT_TIME_INPUT);
                feedbackCode(bot, user, INVALID_CHECKOUT_TIME_INPUT);
                return;
            }
            Time time(message->text);
            auto [code, inTime] = tableCheckout(user, time);

            if (code != SHIFT_TOO_SHORT && code != CHECKOUT_TIME_INCORRECT)
            {
                setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
            }

            if (code == CHECKOUT_SUCCESS) 
            {
                setCommandsForUser(bot, user.chatID, COMMAND_STATE::CHECKED_OUT);

                auto it = std::remove_if(barShift.begin(), barShift.end(), [&](const User& u) {
                    return u.chatID == user.chatID;
                });
                barShift.erase(it, barShift.end());
                
                it = std::remove_if(kitchenShift.begin(), kitchenShift.end(), [&](const User& u) {
                    return u.chatID == user.chatID;
                });
                kitchenShift.erase(it, kitchenShift.end());

                notifyManagerCheckout(bot, users, user, time);
            }
            awaitingCustomCheckoutInput.erase(message->chat->id);
            logCode(user, code);

            feedbackCode(bot, user, code, time, inTime);
        }
        
    });
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
                    }
                }
                backedUpToday = true;
            }
            if (currentTime.ddmmyyyy() != lastProcessedDay)
            {
                backedUpToday = false;
                lastProcessedDay = currentTime.ddmmyyyy();
                std::cout << "[INFO] New day detected: " << lastProcessedDay << " — resetting command states.\n";
                for (User &user : users)
                {
                    if(user.role == MANAGER)
                        setCommandsForUser(bot, user.chatID, COMMAND_STATE::MANAGER_COMMAND_STATE);
                    else if(user.role == DEVELOPER)
                        setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEVELOPER_COMMAND_STATE);
                    else if(user.role == EMPLOYEE)
                        setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
                }
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