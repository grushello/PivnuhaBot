#pragma once
#include <user.hpp>
#include <vector>
#include <time.hpp>
#include <xlnt/xlnt.hpp>

enum EXIT_CODE {DEFAULT_ERROR, SHIFT_TOO_SHORT, MULTIPLE_CHECKIN, MULTIPLE_CHECKOUT, MULTIPLE_REGISTRATION, USER_NOT_FOUND, NO_CHECKIN, CHECKIN_SUCCESS, CHECKOUT_SUCCESS, INVALID_CHECKIN_TIME_INPUT, INVALID_CHECKOUT_TIME_INPUT, CHECKOUT_TIME_INCORRECT, CANCEL_SUCCESS, NOTHING_TO_CANCEL, NO_PERMISSION};
enum TABLE_CREATE_OPTION {REWRITE, NON_REWRITE};
enum TABLE_TYPE {BAR_TABLE, KITCHEN_TABLE};
void createEmptyTable(const std::vector<User>& users, const Time& time, TABLE_TYPE type, TABLE_CREATE_OPTION option = NON_REWRITE);
void addUserToTable(const User& user, TABLE_TYPE type, const Time& time);
EXIT_CODE tableCheckin(const User& user, const Time& time);
std::pair<EXIT_CODE, Time> tableCheckout(const User& user, const Time& time);
EXIT_CODE cancelTableCheckin(const User& user, const Time& time);
std::string getTableFilename(const Time& time, TABLE_TYPE type);
std::string getLastMonthTableFilename(const Time& time, TABLE_TYPE type);
std::string getTableName(const Time& time, TABLE_TYPE type);
bool tableExists(const Time& time, TABLE_TYPE type);
void writeTimeToCell(xlnt::cell &cell, const Time &t);
Time readCellTime(const xlnt::cell &cell);
bool employeeIsAtShift(const User& user, const Time& time);