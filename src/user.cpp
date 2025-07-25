#include "user.hpp"
std::string userRoleToString(USER_ROLE role)
{
    if(role == EMPLOYEE) return "employee";
    if(role == MANAGER) return "manager";
    if(role == DEVELOPER) return "developer";
    return "default";
}
USER_ROLE userStringToRole(std::string role)
{
    if(role == "employee") return EMPLOYEE;
    if(role == "manager") return MANAGER;
    if(role == "developer") return DEVELOPER;
    return USER_ROLE::DEFAULT_ROLE;
}