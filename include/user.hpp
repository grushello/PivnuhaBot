#pragma once
#include <string>

enum USER_ROLE {
    DEFAULT_ROLE,
    EMPLOYEE,
    MANAGER,
    DEVELOPER
};
enum WORKING_AREA
{
    DEFAULT_AREA,
    BAR,
    KITCHEN
};
std::string userRoleToString(USER_ROLE role);
USER_ROLE userStringToRole(std::string role);
std::string userAreaToString(WORKING_AREA area);
WORKING_AREA userStringToArea(std::string area);
class User {
public:
    User() {}
    User(std::string givenName, int64_t givenChatID, USER_ROLE givenRole, WORKING_AREA givenArea) : name(givenName), chatID(givenChatID), role(givenRole), area(givenArea)    {}
    bool operator == (const User &other) { return other.chatID == chatID; }
    std::string name = "NONE_REGISTERED";
    USER_ROLE role = EMPLOYEE;
    int64_t chatID = 0;
    WORKING_AREA area = DEFAULT_AREA;
};