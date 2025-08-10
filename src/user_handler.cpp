#include "user_handler.hpp"
#include "feedback.hpp"

#include <set>
#include <fstream>
#include <iostream>
#include <sstream>

void loadUsers(std::vector<User> &users) {
    std::ifstream file("users.csv");
    if (!file.is_open()) {
        std::cout << "Couldn't open users file: " << "users.csv" << " creating the file" << std::endl;
        std::ofstream myfile;
        myfile.open ("users.csv");
        myfile << "";
        myfile.close();
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, chatIDstr, role, area;

        // Split by semicolon
        std::getline(ss, name, ';');
        std::getline(ss, chatIDstr, ';');
        std::getline(ss, role, ';');
        std::getline(ss, area, ';');

        // clean up quotes
        if (!chatIDstr.empty() && chatIDstr.front() == '"') chatIDstr.erase(0, 1);
        if (!chatIDstr.empty() && chatIDstr.back() == '"') chatIDstr.pop_back();
        users.push_back(User{name, std::stoll(chatIDstr), userStringToRole(role), userStringToArea(area)});
    }

    file.close();
}

void saveUser(User &user)
{
    std::ofstream myfile;
    myfile.open("users.csv", std::ios::app | std::ios::out);
    myfile << user.name << ";" << '"' << user.chatID << '"' << ";" << userRoleToString(user.role) << ";" << userAreaToString(user.area) << "\n";
    myfile.close();
}

User findUser(const std::vector<User> &users, int64_t id)
{
    for(int i = 0; i < users.size(); ++i)
    {
        if(users[i].chatID == id)
            return users[i];
    }
    return User{};
}
bool nameIsRegistered(const std::vector<User>& users, const std::string& name)
{
    for(int i = 0; i < users.size(); ++i)
    {
        if(users[i].name == name)
            return true;
    }
    return false;
}
void removeUserFromShift(const User &user, std::vector<User> &shift)
{
    auto it = std::remove_if(shift.begin(), shift.end(), [&](const User& u) 
    {
        return u.chatID == user.chatID;
     });
    shift.erase(it, shift.end());
}
void setShift(const std::vector<User> users, std::vector<User>& barShift, std::vector<User> &kitchenShift, const Time& time)
{
    for(const User& user : users)
    {
        if(employeeIsAtShift(user, time))
        {
            if(user.area == BAR)
                barShift.push_back(user);
            else if(user.area == KITCHEN)
                kitchenShift.push_back(user);
        }
    }
}
bool validateNewUser(TgBot::Bot& bot, const User& user, const std::vector<User>& users, const std::set<int64_t>& blockedUsers, const Time& time)
{
    if(blockedUsers.count(user.chatID))
        return false;
    if(user.chatID != 0)
    {
        logCode(user, MULTIPLE_REGISTRATION);
        feedbackCode(bot, user, MULTIPLE_REGISTRATION, time);
        return false;
    }
    return true;
}
bool validateUser(TgBot::Bot& bot, const User& user, const std::vector<User>& users, const std::set<int64_t>& blockedUsers, const Time& time)
{
    if(blockedUsers.count(user.chatID))
        return false;
    if(user.chatID == 0)
    {
        logCode(user, USER_NOT_FOUND);
        feedbackCode(bot, user, USER_NOT_FOUND, time);
        return false;
    }
    return true;
}
bool validateUserRole(TgBot::Bot& bot, const User& user, USER_ROLE requiredRole, const Time& time)
{
    if(user.role != requiredRole)
    {
        logCode(user, NO_PERMISSION);
        feedbackCode(bot, user, NO_PERMISSION, time);
        return false;
    }
    return true;
}