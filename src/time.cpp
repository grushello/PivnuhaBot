#include "Time.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <regex>

namespace {
    //------------------------------------------------------------------------------
    //  Helper utilities (internal linkage)
    //------------------------------------------------------------------------------

    //  Check if a given year is a leap year
    bool isLeap(int y)
    {
        return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
    }

    //  Number of days in a month, 1‑based month index (1‑Jan … 12‑Dec)
    int daysInMonth(int m, int y)
    {
        static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2) return isLeap(y) ? 29 : 28;
        return days[m - 1];
    }

    //  Return current local calendar time as std::tm
    std::tm currentLocalTm()
    {
        using namespace std::chrono;

        const std::time_t t = system_clock::to_time_t(system_clock::now());
        std::tm local{};
    #ifdef _WIN32
        localtime_s(&local, &t);
    #else
        local = *std::localtime(&t);
    #endif
        return local;
    }

    //  Apply the “day starts at 05:00” rule: hours < 5 belong to previous day
    void applyFiveAMRule(std::tm& tm)
    {
        if (tm.tm_hour < 5)
        {
            tm.tm_mday -= 1;
            if (tm.tm_mday == 0)
            {
                tm.tm_mon -= 1;               //  borrow a month
                if (tm.tm_mon < 0)
                {
                    tm.tm_mon = 11;          //  December of previous year
                    tm.tm_year -= 1;
                }
                tm.tm_mday = daysInMonth(tm.tm_mon + 1, tm.tm_year + 1900);
            }
        }
    }
}   //  anonymous namespace

//------------------------------------------------------------------------------
//  Constructors
//------------------------------------------------------------------------------

Time::Time()
{
    std::tm now = currentLocalTm();
    applyFiveAMRule(now);

    minute = now.tm_min;
    hour   = now.tm_hour;
    day    = now.tm_mday;
    month  = now.tm_mon + 1;        //  tm_mon is 0‑based
    year   = now.tm_year + 1900;    //  tm_year is years since 1900
}

Time::Time(int hour_, int minute_)
{
    if (hour_ < 0 || hour_ > 23 || minute_ < 0 || minute_ > 59)
        throw std::out_of_range("Hour or minute out of valid range");

    std::tm now = currentLocalTm();

    if(now.tm_hour < 5 && hour_ >= 5)
    {
        now.tm_mday--;
    }
    if(hour_ < 5 && now.tm_hour >= 5)
    {
        now.tm_mday++;
    }
    now.tm_min  = minute_;
    now.tm_hour = hour_;


    applyFiveAMRule(now);

    minute = now.tm_min;
    hour   = now.tm_hour;
    day    = now.tm_mday;
    month  = now.tm_mon + 1;
    year   = now.tm_year + 1900;
}

Time::Time(const std::string& time_str) {
    std::regex time_regex(R"(^\s*(\d{1,2}):(\d{1,2})\s*$)");
    std::smatch match;

    if (!std::regex_match(time_str, match, time_regex))
        throw std::invalid_argument("Invalid time format");

    int hour_ = std::stoi(match[1].str());
    int minute_ = std::stoi(match[2].str());

    if (hour_ < 0 || hour_ > 23 || minute_ < 0 || minute_ > 59)
        throw std::out_of_range("Hour or minute out of valid range");

    std::tm now = currentLocalTm();

    if(now.tm_hour < 5 && hour_ >= 5)
    {
        now.tm_mday--;
    }
    if(hour_ < 5 && now.tm_hour >= 5)
    {
        now.tm_mday++;
    }

    now.tm_min = minute_;
    now.tm_hour = hour_;

    applyFiveAMRule(now);

    minute = now.tm_min;
    hour   = now.tm_hour;
    day    = now.tm_mday;
    month  = now.tm_mon + 1;
    year   = now.tm_year + 1900;
}
//------------------------------------------------------------------------------
//  Formatting helpers
//------------------------------------------------------------------------------

std::string Time::hhmm() const
{
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hour << ':'
        << std::setw(2) << std::setfill('0') << minute;
    return oss.str();
}

std::string Time::ddmm() const
{
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << day << '.'
        << std::setw(2) << std::setfill('0') << month;
    return oss.str();
}

std::string Time::mmyyyy() const
{
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << month << '.'
        << std::setw(4) << std::setfill('0') << year;
    return oss.str();
}

std::string Time::ddmmyyyy() const
{
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << day << '.'
        << std::setw(2) << std::setfill('0') << month << '.'
        << std::setw(4) << std::setfill('0') << year;
    return oss.str();
}

//------------------------------------------------------------------------------
//  Static helpers
//------------------------------------------------------------------------------

int Time::daysInMonth()
{
    std::tm now = currentLocalTm();
    return ::daysInMonth(now.tm_mon + 1, now.tm_year + 1900);
}

//  Difference between two Time objects, in hours (fractional)
//  Uses the “day starts at 05:00” convention by applying a 5‑hour circular shift.

double Time::hoursDiff(const Time& a, const Time& b)
{
    auto toCircularHours = [](const Time& t) {
        double h = t.hour + t.minute / 60.0;
        h -= 5.0;                //  shift so 05:00 becomes 0
        if (h < 0) h += 24.0;    //  wrap negative values into the 0–24 range
        return h;
    };

    double ha = toCircularHours(a);
    double hb = toCircularHours(b);

    return std::fabs(hb - ha);    //  always non‑negative
}

std::string Time::toddmm(const std::string& _day, const std::string& _month)
{
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << _day << '.'
        << std::setw(2) << std::setfill('0') << _month;
    return oss.str();
}


bool Time::isValidTime(const std::string& time_str)
{
    std::regex time_regex(R"(^(\d{1,2}):(\d{1,2})$)");
    std::smatch match;

    if (std::regex_match(time_str, match, time_regex)) {
        int hour = std::stoi(match[1].str());
        int minute = std::stoi(match[2].str());

        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
            return true;
        }
    }

    return false;
}

bool Time::operator==(const Time& other) const {
    return hour == other.hour &&
           minute == other.minute &&
           day == other.day &&
           month == other.month &&
           year == other.year;
}
std::string Time::getPreviousMonthName(int month)
{
    int prevMonth = month - 1;
    if (prevMonth == 0)
        prevMonth = 12;
    return getMonthName(prevMonth);
}
const std::array<std::string, 12> Time::MONTH_NAMES = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};