#include "table_handler.hpp"
#include <sstream>
#include <xlnt/xlnt.hpp>
#include <filesystem>

TABLE_TYPE getCorrespondingTableType(const User& user)
{
    if(user.area == BAR)
        return BAR_TABLE;
    return KITCHEN_TABLE;
}
void writeTimeToCell(xlnt::cell &cell, const Time &t)
{
    double excelTime = (t.hour * 3600 + t.minute * 60) / 86400.0;
    cell.value(excelTime);
    cell.number_format(xlnt::number_format("HH:MM"));
}

Time readCellTime(const xlnt::cell &cell)
{
    Time t;
    if (!cell.has_value()) return t;

    switch (cell.data_type())
    {
        case xlnt::cell_type::number:
        {
            // Excel stores time as fraction of a day
            double fraction = cell.value<double>();
            int total_minutes = static_cast<int>(fraction * 1440); // 1440 = 24 * 60

            t.hour = total_minutes / 60;
            t.minute = total_minutes % 60;
            break;
        }

        case xlnt::cell_type::shared_string:
        {
            std::string value = cell.to_string(); // expected format: "HH:MM"
            if(!Time::isValidTime(value))
            {
                std::cerr << "Attempting to read incorrectly set time from a cell\n";
                break;
            }
            std::istringstream iss(value);
            char colon;

            iss >> t.hour >> colon >> t.minute;
            break;
        }

        default:
            break;
    }

    return t;
}

int getDayRow(int day) { return day+2; }
int getNameColumn(const std::string& name, const Time& time, TABLE_TYPE type)
{
    xlnt::workbook wb;
    wb.load(getTableFilename(time, type));
    auto ws = wb.active_sheet();
    
    int column = 2;
    while(ws.cell(column, 1).has_value())
    {
        if(ws.cell(column, 1).to_string() == name)
            return column;
        column += 3;
    }
    return -1;
}
void createEmptyTable(const std::vector<User>& users, const Time& time ,TABLE_TYPE type, TABLE_CREATE_OPTION option)
{
    if(tableExists(time, type) && option == NON_REWRITE)
    {
        std::cout << "Table for the month already exists, rewrite attempt dismissed\n";
        return;
    }

    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet();

    ws.title(getTableName(time, type));
    wb.save(getTableFilename(time, type)); 
    
    for (int i = 0; i < users.size(); ++i)
    {
        if(users[i].role == EMPLOYEE && getCorrespondingTableType(users[i]) == type)
            addUserToTable(users[i], type, time);
    }

    wb.load(getTableFilename(time, type));
    ws = wb.active_sheet();

    ws.title(getTableName(time, type));

    // Set "Date" in A1/A2
    ws.cell("A1").value("");
    ws.merge_cells("A1:A2");

    int totalRow = getDayRow(Time::daysInMonth()) + 1;
    
    ws.cell(1, totalRow).value("Total");

    // Freeze header
    ws.freeze_panes("B3");

    for (int day = 1; day <= Time::daysInMonth(); ++day)
    {
        ws.cell(1, getDayRow(day)).value(Time::toddmm(std::to_string(day), std::to_string(time.month)));
    }

    try
    {
        wb.save(getTableFilename(time, type));
    }
    catch (const xlnt::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
void addUserToTable(const User& user, TABLE_TYPE type, const Time& time)
{
    xlnt::workbook wb;
    wb.load(getTableFilename(time, type));
    xlnt::worksheet ws = wb.active_sheet();

    xlnt::alignment centered;
    centered.horizontal(xlnt::horizontal_alignment::center);
    centered.vertical(xlnt::vertical_alignment::center);

    int col = 2;
    while(ws.cell(col, 1).has_value())
    {
        col+=3;
    }
    std::string topLeft = xlnt::cell_reference(col, 1).to_string();
    std::string topRight = xlnt::cell_reference(col + 2, 1).to_string();
    ws.merge_cells(topLeft + ":" + topRight);
    ws.cell(col, 1).value(user.name);
    ws.cell(col, 1).alignment(centered);
    
    // In / Out sub-headers
    ws.cell(col, 2).value("In");
    ws.cell(col + 1, 2).value("Out");

        // Hrs column (single cell, no merge needed)
    ws.cell(col + 2, 2).value("Hrs");
    int startRow = 3;
    int totalRow = getDayRow(Time::daysInMonth()) + 1;

    int endRow = totalRow - 1; // last row with daily values
    std::string hrsColLetter = xlnt::cell_reference(col + 2, 1).column().column_string();
    std::string range = hrsColLetter + std::to_string(startRow) + ":" + hrsColLetter + std::to_string(endRow);
    ws.cell(col + 2, totalRow).formula("=SUM(" + range + ")");
    try
    {
        wb.save(getTableFilename(time, type));
    }
    catch (const xlnt::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
std::string getTableFilename(const Time& time, TABLE_TYPE type)
{
    std::string typestr = type == BAR_TABLE? "Bar" : "Kitchen";
    return "workingHours" + typestr + time.mmyyyy() + ".xlsx";
}

std::string getTableName(const Time& time, TABLE_TYPE type)
{
    std::string typestr = type == BAR_TABLE? "Bar" : "Kitchen";
    return "workingHours" + typestr + time.mmyyyy();
}
std::string getLastMonthTableFilename(const Time& time, TABLE_TYPE type)
{
    std::string typestr = type == BAR_TABLE? "Bar" : "Kitchen";
    int prevMonth = time.month - 1;
    int year = time.year;

    if (prevMonth == 0) { // If January, go back to December of previous year
        prevMonth = 12;
        year -= 1;
    }

    // Format month to always have two digits (e.g., "01", "02", ..., "12")
    std::ostringstream oss;
    oss << "workingHours"  << typestr << std::setw(2) << std::setfill('0') << prevMonth << '.' << year << ".xlsx";

    return oss.str();
}

EXIT_CODE tableCheckin(const User& user, const Time& time)
{
    xlnt::workbook wb;
    TABLE_TYPE type = user.area == BAR? BAR_TABLE : KITCHEN_TABLE;
    wb.load(getTableFilename(time, type));
    xlnt::worksheet ws = wb.active_sheet();

    int column = getNameColumn(user.name, time, type);
    if (column == -1)
    {
        return EXIT_CODE::USER_NOT_FOUND;
    }
    int row = getDayRow(time.day);


    if(ws.cell(column, row).has_value())
    {
        return EXIT_CODE::MULTIPLE_CHECKIN;
    }
    auto cell = ws.cell(column, row);
    writeTimeToCell(cell, time);

    try
    {
        wb.save(getTableFilename(time, type));
        return EXIT_CODE::CHECKIN_SUCCESS;
    }
    catch (const xlnt::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_CODE::DEFAULT_ERROR;
}
std::pair<EXIT_CODE, Time> tableCheckout(const User& user, const Time& time)
{
    xlnt::workbook wb;
    TABLE_TYPE type = user.area == BAR? BAR_TABLE : KITCHEN_TABLE;
    wb.load(getTableFilename(time, type));
    xlnt::worksheet ws = wb.active_sheet();

    int column = getNameColumn(user.name, time, type);
    if (column++ == -1)
    {
        return {EXIT_CODE::USER_NOT_FOUND, Time()};
    }
    int row = getDayRow(time.day);

    if(ws.cell(column, row).has_value())
    {
        return {EXIT_CODE::MULTIPLE_CHECKOUT, Time()};
    }

    if (!ws.cell(column - 1, row).has_value())
    {
        return {EXIT_CODE::NO_CHECKIN, Time()};
    }
    Time inTime = readCellTime(ws.cell(column - 1, row));
    double hours = Time::hoursDiff(time, inTime);
    
    if(time.hour <= 8 && (inTime.hour >= 23 || inTime.hour <= 5))
    {
        return {EXIT_CODE::CHECKOUT_TIME_INCORRECT, Time()};
    }

    if(hours <= 0.1)
    {
        return {EXIT_CODE::SHIFT_TOO_SHORT, Time()};
    }

    auto cell = ws.cell(column, row);
    writeTimeToCell(cell, time);
    ws.cell(column + 1, row).value(std::round(hours * 100.0) / 100.0);

    try
    {
        wb.save(getTableFilename(time, type));
        return {EXIT_CODE::CHECKOUT_SUCCESS, inTime};
    }
    catch (const xlnt::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return {EXIT_CODE::DEFAULT_ERROR, Time()};
}
bool tableExists(const Time& time, TABLE_TYPE type)
{
    std::string filename = getTableFilename(time, type);
    return std::filesystem::exists(filename);
}
EXIT_CODE cancelTableCheckin(const User& user, const Time& time)
{
    xlnt::workbook wb;
    TABLE_TYPE type = user.area == BAR? BAR_TABLE : KITCHEN_TABLE;
    wb.load(getTableFilename(time, type));
    xlnt::worksheet ws = wb.active_sheet();

    int column = getNameColumn(user.name, time, type);
    if (column++ == -1)
    {
        return EXIT_CODE::USER_NOT_FOUND;
    }
    int row = getDayRow(time.day);

    if(!ws.cell(column - 1, row).has_value() && !ws.cell(column, row).has_value() && !ws.cell(column + 1, row).has_value())
    {
        return EXIT_CODE::NOTHING_TO_CANCEL;
    }
    ws.cell(column-1, row).clear_value();
    ws.cell(column, row).clear_value();
    ws.cell(column+1, row).clear_value();

    try
    {
        wb.save(getTableFilename(time, type));
        return EXIT_CODE::CANCEL_SUCCESS;
    }
    catch (const xlnt::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_CODE::DEFAULT_ERROR;
}