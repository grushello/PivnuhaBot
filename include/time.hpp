#pragma once
#include <string>
#include <array>

class Time
{
public:
    Time();
    Time(int hour, int minute);
    Time(const std::string& time);

    std::string hhmm() const;
    std::string ddmm() const;
    std::string mmyyyy() const;
    std::string ddmmyyyy() const;

    static int daysInMonth();
    static double hoursDiff (const Time &t1, const Time &t2);
    static std::string toddmm(const std::string& day, const std::string& month);
    static bool isValidTime(const std::string& time_str);
    static std::string getMonthName(int month) { return Time::MONTH_NAMES[month-1]; }
    static std::string getPreviousMonthName(int month);
    
    int minute;
    int hour;
    int day;
    int month;
    int year;

    bool operator==(const Time& other) const;
    static const std::array<std::string, 12> MONTH_NAMES;

};