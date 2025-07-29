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
std::string userAreaToString(WORKING_AREA area)
{
    if(area == BAR) return "bar";
    if(area == KITCHEN) return "kitchen";
    return "default";
}
WORKING_AREA userStringToArea(std::string area)
{
    if(area == "bar") return BAR;
    if(area == "kitchen") return KITCHEN;
    return WORKING_AREA::DEFAULT_AREA;
}