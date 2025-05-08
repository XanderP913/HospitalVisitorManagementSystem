#pragma once

// Prevent windows.h macros from conflicting
#define NOMINMAX
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#undef max
#undef min

#include <nlohmann/json.hpp>
#include <string>

class DBManager {
public:
    DBManager();
    ~DBManager();

    bool connect();
    bool checkInVisitor(const std::string& firstName,
        const std::string& lastName,
        const std::string& phone,
        const std::string& room);

    bool checkOutVisitor(int visitorId);
    bool listActiveVisitors();
    bool getActiveVisitorsJson(nlohmann::json& outJson);

    void disconnect();

private:
    SQLHANDLE env_ = nullptr;
    SQLHANDLE dbc_ = nullptr;
    void extractError(const char* fn, SQLHANDLE handle, SQLSMALLINT type);
};
