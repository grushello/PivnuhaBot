#include "utils.hpp"
#include "time.hpp"
#include "iostream"
#include <fstream>

void saveLastProcessedDay(const std::string& ddmmyyyy)
{
    try
    {
        std::ofstream outFile("lastProcessedDay.txt");
        if (outFile)
        {
            outFile << ddmmyyyy;
        }
    }
    catch(...)
    {
        std::cout << "couldn't write last processed day to a file\n";
    }
}
std::string readLastProcessedDay()
{
    Time time;
    try
    {
        std::ifstream inFile("lastProcessedDay.txt");
        std::string date;

        if (inFile)
        {
            std::getline(inFile, date);
            return date;
        }
        return time.ddmmyyyy();
    }
    catch (...)
    {
        return time.ddmmyyyy();
    }
}
std::string readLastProcessedMonth()
{
    Time time;
    try
    {
        std::ifstream inFile("lastProcessedDay.txt");
        std::string date;

        if (inFile)
        {
            std::getline(inFile, date);
            if (date.size() >= 3)
                return date.substr(3); // skip "dd."
        }
        return time.ddmmyyyy().substr(3);
    }
    catch (...)
    {
        return time.ddmmyyyy().substr(3);
    }
}