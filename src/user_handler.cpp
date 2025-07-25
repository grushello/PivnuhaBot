#include "user_handler.hpp"
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
        std::string name, chatIDstr, role;

        // Split by semicolon
        std::getline(ss, name, ';');
        std::getline(ss, chatIDstr, ';');
        std::getline(ss, role, ';');

        // clean up quotes
        if (!chatIDstr.empty() && chatIDstr.front() == '"') chatIDstr.erase(0, 1);
        if (!chatIDstr.empty() && chatIDstr.back() == '"') chatIDstr.pop_back();
        users.push_back(User{name, std::stoll(chatIDstr), userStringToRole(role)});
    }

    file.close();
}

void saveUser(User &user)
{
    std::ofstream myfile;
    myfile.open("users.csv", std::ios::app | std::ios::out);
    myfile << user.name << ";" << '"' << user.chatID << '"' << ";" << userRoleToString(user.role) << "\n";
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