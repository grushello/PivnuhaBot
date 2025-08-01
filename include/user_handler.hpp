#pragma once
#include "user.hpp"
#include "table_handler.hpp"
#include <vector>

void loadUsers(std::vector<User> &users);
void saveUser(User &user);
User findUser(const std::vector<User> &users, int64_t id);
bool nameIsRegistered(const std::vector<User>& users, const std::string &name);
void removeUserFromShift(const User &user, std::vector<User> &shift);
void setShift(const std::vector<User> users, std::vector<User>& barShift, std::vector<User> &kitchenShift, const Time& time);