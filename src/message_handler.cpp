#include "message_handler.hpp"
#include "user_handler.hpp"
#include "table_handler.hpp"
#include "time.hpp"
#include "feedback.hpp"
#include "utils.hpp"
#include "config.hpp"

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
                            const char* PASSWORD)
{
    bot.getEvents().onAnyMessage(
        [&bot, &awaitingNameInput, &awaitingPasswordInput, &awaitingRoleInput, &awaitingAreaInput,
         &pendingNames, &awaitingCustomCheckinInput, &awaitingCustomCheckoutInput, &users,
         &blockedUsers, &barShift, &kitchenShift, PASSWORD]
        (TgBot::Message::Ptr message)
    {
        if (blockedUsers.count(message->chat->id))
            return;

        if (awaitingPasswordInput.count(message->from->id))
        {
            if (message->text == PASSWORD)
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
                sendSafeMessage(bot, message->chat->id, "⚠️ This name is already taken! Please /start again");
                std::cout << "user " << message->from->id << " Entered incorrect name\n";
                awaitingNameInput.erase(message->chat->id);
                return;
            }
            pendingNames[message->chat->id] = message->text;
            awaitingNameInput.erase(message->chat->id);
            awaitingRoleInput.insert(message->chat->id);
            sendSafeMessage(bot, message->chat->id, "Now choose your role: send 'manager' or 'employee'");
        }
        else if (awaitingRoleInput.count(message->from->id))
        {
            std::string text = message->text;
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
            if (text != "manager" && text != "employee") 
            {
                sendSafeMessage(bot, message->chat->id, "⚠️Invalid role! Please /start again and type either 'manager' or 'employee'.");
                awaitingRoleInput.erase(message->from->id);
                return;
            }
            USER_ROLE role = (text == "manager") ? USER_ROLE::MANAGER : USER_ROLE::EMPLOYEE;

            if (role == USER_ROLE::EMPLOYEE)
            {
                sendSafeMessage(bot, message->chat->id, "🙌 Almost done! Now choose your working area: send 'bar' or 'kitchen'");
                awaitingRoleInput.erase(message->chat->id);
                awaitingAreaInput.insert(message->chat->id);
                return;
            }

            User user(pendingNames[message->from->id], message->from->id, role, DEFAULT_AREA);
            saveUser(user);
            setCommandsForUser(bot, message->chat->id,
                user.role == EMPLOYEE ? DEFAULT_COMMAND_STATE :
                user.role == DEVELOPER ? DEVELOPER_COMMAND_STATE :
                                         MANAGER_COMMAND_STATE);
            sendSafeMessage(bot, message->chat->id, "🎉 Registration complete! Thanks, " + user.name + "! You are registered as a " + text + ".");
            std::cout << "User " << user.name << " registered with role: " << text << "\n";
            awaitingRoleInput.erase(message->chat->id);
            users.push_back(user);
        }
        else if (awaitingAreaInput.count(message->from->id))
        {
            std::string text = message->text;
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
            if (text != "bar" && text != "kitchen") 
            {
                sendSafeMessage(bot, message->chat->id, "⚠️Invalid area! Please /start again and type either 'kitchen' or 'bar'.");
                awaitingAreaInput.erase(message->from->id);
                return;
            }
            WORKING_AREA area = (text == "bar") ? WORKING_AREA::BAR : WORKING_AREA::KITCHEN;
            User user(pendingNames[message->from->id], message->from->id, EMPLOYEE, area);

            saveUser(user);
            if (area == BAR)
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
        else if (awaitingCustomCheckinInput.count(message->from->id))
        {
            User user = findUser(users, message->from->id);
            if (!Time::isValidTime(message->text))
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
                if (user.area == BAR)
                    barShift.push_back(user);
                else 
                    kitchenShift.push_back(user);
                notifyManagerCheckin(bot, users, user, time);
            }
            awaitingCustomCheckinInput.erase(message->from->id);
            logCode(user, code);
            feedbackCode(bot, user, code, time);
        }
        else if (awaitingCustomCheckoutInput.count(message->from->id))
        {
            User user = findUser(users, message->from->id);
            if (!Time::isValidTime(message->text))
            {
                awaitingCustomCheckoutInput.erase(message->from->id);
                logCode(user, INVALID_CHECKOUT_TIME_INPUT);
                feedbackCode(bot, user, INVALID_CHECKOUT_TIME_INPUT);
                return;
            }
            Time time(message->text);
            auto [code, inTime] = tableCheckout(user, time);

            if (code != SHIFT_TOO_SHORT && code != CHECKOUT_TIME_INCORRECT)
                setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);

            if (code == CHECKOUT_SUCCESS) 
            {
                setCommandsForUser(bot, user.chatID, COMMAND_STATE::CHECKED_OUT);
                removeUserFromShift(user, barShift);
                removeUserFromShift(user, kitchenShift);
                notifyManagerCheckout(bot, users, user, time);
            }
            awaitingCustomCheckoutInput.erase(message->chat->id);
            logCode(user, code);
            feedbackCode(bot, user, code, time, inTime);
        }
    });
}