#include "feedback.hpp"

void logCode(const User& user, EXIT_CODE code)
{
    std::string message;
    switch (code)
    {
        case DEFAULT_ERROR:
            message = "Unknown error occurred";
            break;
        case SHIFT_TOO_SHORT:
            message = "Shift was too short (< 0.1h)";
            break;
        case MULTIPLE_CHECKIN:
            message = "User tried to check in more than once";
            break;
        case MULTIPLE_CHECKOUT:
            message = "User tried to check out more than once";
            break;
        case MULTIPLE_REGISTRATION:
            message = "User tried to register more than once";
        case USER_NOT_FOUND:
            message = "User not found in table";
            break;
        case NO_CHECKIN:
            message = "User attempted checkout without check-in";
            break;
        case CHECKIN_SUCCESS:
            message = "User successfully checked in";
            break;
        case CHECKOUT_SUCCESS:
            message = "User successfully checked out";
            break;
        case INVALID_CHECKIN_TIME_INPUT:
            message = "User entered invalid time for checkin";
            break;
        case INVALID_CHECKOUT_TIME_INPUT:
            message = "User entered invalid time for checkout";
            break;
        case CANCEL_SUCCESS:
            message = "User succesfully canceled their shift!";
            break;
        case NOTHING_TO_CANCEL:
            message = "User tried to cancel non-existing shift!";
            break;
        case NO_PERMISSION:
            message = "User tried to use command they had no permission to use";
    }

    std::cout << "LOG (" << user.chatID << " - " << user.name << "): " << message << std::endl;
}

void feedbackCode(TgBot::Bot &bot, const User& user, EXIT_CODE code, const Time& time, const Time& inTime)
{
    std::string response;
    switch (code)
    {
        case DEFAULT_ERROR:
            response = "❌ An unknown error occurred. Please try again or contact support.";
            break;
        case SHIFT_TOO_SHORT:
            response = "⚠️ Your shift duration is too short (less than 6 minutes). Please ensure your check-in and checkout times are set properly.";
            break;
        case MULTIPLE_CHECKIN:
            response = "⚠️ You've already checked in for today.";
            break;
        case MULTIPLE_CHECKOUT:
            response = "⚠️ You've already checked out for today.";
            break;
        case MULTIPLE_REGISTRATION:
            response = "⚠️ It appears You are already registered.";
        case USER_NOT_FOUND:
            response = "⚠️ You are not registered. Use /start to register.";
            break;
        case NO_CHECKIN:
            response = "⚠️ No check-in record found. Please check in first.";
            break;
        case CHECKIN_SUCCESS:
            response = "✅ You were checked in at: " + time.hhmm();
            break;
        case CHECKOUT_SUCCESS:
        {
            double duration = Time::hoursDiff(time, inTime);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "✅ Checked in at: " << inTime.hhmm()
                << "\n✅ Checked out at: " << time.hhmm()
                << "\n🕒 Worked: " << Time::hoursDiff(time, inTime) << " hrs";
            response = oss.str();
            break;
        }
        case INVALID_CHECKIN_TIME_INPUT:
            response = "⚠️ Invalid time format, please use: HH:MM (00:00) and /customcheckin again";
            break;
        case INVALID_CHECKOUT_TIME_INPUT:
            response = "⚠️ Invalid time format, please use: HH:MM (00:00) and /customcheckout again";
            break;
        case CHECKOUT_TIME_INCORRECT:
            response = "⚠️ It appears you've entered incorrect checkout time";
            break;
        case CANCEL_SUCCESS:
            response = "Your today's shift was canceled!";
            break;
        case NOTHING_TO_CANCEL:
            response = "⚠️It appears, you have no shift to cancel!";
            break;
        case NO_PERMISSION:
            response = "⚠️You have no permission to use this command!";
            break;
    }

    if (!response.empty()) {
        sendSafeMessage(bot, user.chatID, response);
    }
}