#pragma once
#include <string>

enum USER_ROLE {
    DEFAULT_ROLE,
    EMPLOYEE,
    MANAGER,
    DEVELOPER
};
std::string userRoleToString(USER_ROLE role);
USER_ROLE userStringToRole(std::string role);
class User {
public:
    User() {}
    User(std::string givenName, int64_t givenChatID, USER_ROLE givenRole) : name(givenName), chatID(givenChatID), role(givenRole)   {}
    bool operator == (const User &other) { return other.chatID == chatID; }
    std::string name = "";
    USER_ROLE role = EMPLOYEE;
    int64_t chatID = 0;
};