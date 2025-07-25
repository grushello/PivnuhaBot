#pragma once
#include "user.hpp"
#include <vector>

void loadUsers(std::vector<User> &users);
void saveUser(User &user);
User findUser(const std::vector<User> &users, int64_t id);
bool nameIsRegistered(const std::vector<User>& users, const std::string &name);