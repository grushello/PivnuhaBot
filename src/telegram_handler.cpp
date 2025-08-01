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

void setCommandsForUser(const TgBot::Bot& bot, int64_t chatID, COMMAND_STATE state)
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
            add("checkin", "🕒Check-in");
            add("customcheckin", "🕒Check-in with set time");
            break;

        case CHECKED_IN:
            add("checkout", "🕒Check-out");
            add("customcheckout", "🕒Check-out with set time");
            add("cancel", "Cancel today's check-in/out");
            break;

        case CHECKED_OUT:
            add("cancel", "❌Cancel today's check-in/out");
            break;
        case MANAGER_COMMAND_STATE:
            add("currentshift", "👥 See current shift");
            add("tablebar", "📅 Bar table for this month");
            add("lastmonthbar", "📂 Bar table for last month");
            add("tablekitchen", "📅 Kitchen table for this month");
            add("lastmonthkitchen", "📂 Kitchen table for last month");
            break;
        case DEVELOPER_COMMAND_STATE:
            add("currentshift", "👥 See current shift");
            add("tablebar", "📅 Bar table for this month");
            add("lastmonthbar", "📂 Bar table for last month");
            add("tablekitchen", "📅 Kitchen table for this month");
            add("lastmonthkitchen", "📂 Kitchen table for last month");
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
void resetCommandsForAllUsers(const TgBot::Bot& bot, const std::vector<User>& users)
{
    for (const User &user : users)
    {
        if (user.role == MANAGER)
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::MANAGER_COMMAND_STATE);
        else if (user.role == DEVELOPER)
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEVELOPER_COMMAND_STATE);
        else if (user.role == EMPLOYEE)
        {
            setCommandsForUser(bot, user.chatID, COMMAND_STATE::DEFAULT_COMMAND_STATE);
        }
    }
}
bool sendTableDoc(const TgBot::Bot& bot, const User& user, const std::string& filepath, const Time& time, bool isBarTable, bool isPreviousMonth)
{
    std::string typestr = isBarTable? "bar" : "kitchen";
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
            "Here’s the " + typestr + " table for " + monthName + " 🎉" // optional caption
        );                                 // the rest of the parameters keep their defaults
        std::cout << "The " << typestr << " table was succesfully sent to user: " << user.chatID << " - " << user.name << std::endl; 
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
        "📅 /tablebar - Bar table for current month\n"
        "📂 /lastmonthbar - Bar table for last month\n"
        "📅 /tablekitchen - Kitchen table for current month\n"
        "📂 /lastmonthkitchen - Kitchen table for last month\n";
        if(user.role == DEVELOPER)
        {    
            message += "/restart - restart the bot\n";
        }
        sendSafeMessage(bot, user.chatID, message);
    }
    else
    {
        std::string tableCommands = user.area == BAR?
            "📅 /tablebar - Table for current month\n"
            "📂 /lastmonthbar - Table for last month\n"
            :
            "📅 /tablekitchen - Table for current month\n"
            "📂 /lastmonthkitchen - Table for last month\n";

        sendSafeMessage(bot, user.chatID, "🤖 Here's what I can help you with:\n" + tableCommands
            + "🕒/checkin - Check-in to work\n"
            "🕒/customcheckin - Check-in for set time\n"
            "🕒/checkout - Check-out from work\n"
            "🕒/customcheckout - Check-out for set time\n"
            "❌/cancel - Cancel today's check-in/check-out\n");
    }
    }
void listCurrentShift(TgBot::Bot& bot, const std::vector<User>& barShift, const std::vector<User>& kitchenShift, const User& user)
{
    std::string message = "👥 Here is the current shift:\n";
    
    if(barShift.size() != 0)
        message += "Bar:\n";
    for(int i = 0; i < barShift.size(); ++i)
        message += "\t" + barShift[i].name + '\n';
    
    if(kitchenShift.size() != 0)
        message += "Kitchen:\n";
    for(int i = 0; i < kitchenShift.size(); ++i)
        message += "\t" + kitchenShift[i].name + '\n';

    if(barShift.size() == 0 && kitchenShift.size() == 0)
        message = "🌙 No one's working right now!";
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