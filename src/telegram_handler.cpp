#include "telegram_handler.hpp"
#include "time.hpp"
//below is a very weird line, the need to use which probably stems from improperly built tgbot library, but i'm not sure. Anyway be careful with it.
const std::string TgBot::BotCommandScopeChat::TYPE = "chat";

void sendSafeMessage(TgBot::Bot &bot, int64_t chatID, const std::string& message) {
    std::string output = message;
    if (output.size() <= 4096)
        bot.getApi().sendMessage(chatID, output);
    else{
        bot.getApi().sendMessage(chatID, "An error occured, the message the bot was going to send was too long!");
    }
}

void setCommandsForUser(TgBot::Bot& bot, int64_t chatID, COMMAND_STATE state)
{
    std::vector<TgBot::BotCommand::Ptr> commands;

    auto add = [&](const std::string& cmd, const std::string& desc) {
        TgBot::BotCommand::Ptr command(new TgBot::BotCommand);
        command->command = cmd;
        command->description = desc;
        commands.push_back(command);
    };

    add("help", "List all avaiable commands");

    switch (state)
    {
        case DEFAULT_COMMAND_STATE:
            add("checkin", "Check-in");
            add("customcheckin", "Check-in with set time");
            break;

        case CHECKED_IN:
            add("checkout", "Check-out");
            add("customcheckout", "Check-out with set time");
            add("cancel", "Cancel today's check-in/out");
            break;

        case CHECKED_OUT:
            add("cancel", "Cancel today's check-in/out");
            break;
        case MANAGER_COMMAND_STATE:
            add("currentshift", "See current shift");
            add("table", "This month table");
            add("lastmonthtable", "last month table");
            break;
        case DEVELOPER_COMMAND_STATE:
            add("currentshift", "See current shift");
            add("table", "This month table");
            add("lastmonthtable", "last month table");
            add("restart", "restart the bot");
            break;
    }

    TgBot::BotCommandScopeChat::Ptr userScope(new TgBot::BotCommandScopeChat);
    userScope->chatId = chatID;

    try {
        bot.getApi().setMyCommands(commands, userScope);
    } catch (const TgBot::TgException& e) {
        std::cerr << "Telegram API error while setting commands: " << e.what() << std::endl;
    }
}

bool sendTableDoc(const TgBot::Bot& bot, const User& user, const std::string& filepath, const Time& time, bool isPreviousMonth)
{
    using TgBot::InputFile;
    std::string monthName = isPreviousMonth? Time::getPreviousMonthName(time.month) : Time::getMonthName(time.month);
    try
    { // 1. Wrap the file that is already on your filesystem
        auto doc = InputFile::fromFile(
            filepath,                                                             // full path, e.g. "tables/07‑25.xlsx"
            "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"); // pick the right MIME type

        // 2. Ship it
        bot.getApi().sendDocument(
            user.chatID,             // who to send to
            doc,                           // the file itself
            "",                            // thumbnail (skip)
            "Here’s the table for " + monthName + " 🎉" // optional caption
        );                                 // the rest of the parameters keep their defaults
        std::cout << "The table was succesfully sent to user: " << user.chatID << " - " << user.name << std::endl; 
        return true;
    }
    catch (const std::ios_base::failure& e) {
        std::cerr << "I/O error for \"" << filepath << "\": " << e.what() << '\n';
        bot.getApi().sendMessage(user.chatID, "⚠️ Sorry, I couldn't open the file I'm supposed to send.");
    }
    catch (const TgBot::TgException& e) {
        std::cerr << "Telegram API error while sending \"" << filepath << "\": " << e.what() << '\n';
        bot.getApi().sendMessage( user.chatID, std::string("⚠️ Failed to send the document: ") + e.what());
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected error in sendTableDoc: " << e.what() << '\n';
    }
    catch (...) {
        std::cerr << "Unknown non‑std exception in sendTableDoc\n";
    }
    return false;
}
void listAvailableCommands(TgBot::Bot &bot, const User& user)
{
    if(user.role == USER_ROLE::MANAGER || user.role == DEVELOPER)
    {
        std::string message = "Here is the list of all avaiable commands!\n"
        "👥 /currentshift - see current shift\n"
        "📅 /table - Table for current month\n"
        "📂 /lastmonthtable - Table for last month\n";
        if(user.role == DEVELOPER)
        {    
            message += "/restart - restart the bot\n";
        }
        sendSafeMessage(bot, user.chatID, message);
    }
    else
    {
        sendSafeMessage(bot, user.chatID, "🤖 Here's what I can help you with:\n"
            "📅 /table - Table for current month\n"
            "📂 /lastmonthtable - Table for last month\n"
            "🕒/checkin - Check-in to work\n"
            "🕒/customcheckin - Check-in for set time\n"
            "🕒/checkout - Check-out from work\n"
            "🕒/customcheckout - Check-out for set time\n"
            "❌/cancel - Cancel today's check-in/check-out\n");
    }
    }
void listCurrentShift(TgBot::Bot& bot, const std::vector<User>& shift, const User& user)
{
    std::string message = "👥 Here is the current shift:\n";
    
    if(shift.size() == 0)
        message = "🌙 No one's working right now!";
    
    for(int i = 0; i < shift.size(); ++i)
        message += shift[i].name + '\n';
    sendSafeMessage(bot, user.chatID, message);
}
void notifyManagerCheckin(TgBot::Bot& bot, const std::vector<User>& users, const User& user, const Time& time)
{
    for(int i = 0; i < users.size(); ++i)
    {
        if(users[i].role == MANAGER || users[i].role == DEVELOPER)
        {
            sendSafeMessage(bot, users[i].chatID, user.name + " just checked in at " + time.hhmm());
        }
    }
}
void notifyManagerCheckout(TgBot::Bot& bot, const std::vector<User>& users, const User& user, const Time& time)
{
    for(int i = 0; i < users.size(); ++i)
    {
        if(users[i].role == MANAGER || users[i].role == DEVELOPER)
        {
            sendSafeMessage(bot, users[i].chatID, user.name + " just checked out at " + time.hhmm());
        }
    }
}
void notifyManagerCancel(TgBot::Bot& bot, const std::vector<User>& users, const User& user)
{
    for(int i = 0; i < users.size(); ++i)
    {
        if(users[i].role == MANAGER || users[i].role == DEVELOPER)
        {
            sendSafeMessage(bot, users[i].chatID, user.name + " just canceled their today's check-in");
        }
    }
}