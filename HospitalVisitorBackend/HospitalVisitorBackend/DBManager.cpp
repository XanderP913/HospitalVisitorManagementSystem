#include "DBManager.h"
#include <iostream>
#include <sstream>

DBManager::DBManager() { }
DBManager::~DBManager() { disconnect(); }

bool DBManager::connect() {
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_) != SQL_SUCCESS) return false;
    SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_) != SQL_SUCCESS) return false;

    SQLCHAR connStr[] =
        "DRIVER={ODBC Driver 17 for SQL Server};"
        "SERVER=(localdb)\\MSSQLLocalDB;"
        "DATABASE=HospitalVisitorDB;"
        "Trusted_Connection=yes;";
    SQLCHAR out[1024]; SQLSMALLINT outLen;
    if (!SQL_SUCCEEDED(SQLDriverConnectA(dbc_, nullptr, connStr, SQL_NTS,
        out, sizeof(out), &outLen,
        SQL_DRIVER_COMPLETE))) {
        extractError("SQLDriverConnect", dbc_, SQL_HANDLE_DBC);
        return false;
    }
    return true;
}

bool DBManager::checkInVisitor(const std::string& firstName,
    const std::string& lastName,
    const std::string& phone,
    const std::string& room) {
    SQLHANDLE stmt = nullptr;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt))) {
        extractError("AllocStmt", dbc_, SQL_HANDLE_DBC);
        return false;
    }

    std::ostringstream ss;
    ss << "INSERT INTO Visitors "
        "(FirstName, LastName, PhoneNumber, PatientRoom, CheckInTime) "
        "VALUES ('" << firstName << "','" << lastName << "','"
        << phone << "','" << room << "', GETDATE());";

    SQLRETURN ret = SQLExecDirectA(stmt, (SQLCHAR*)ss.str().c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extractError("SQLExecDirect", stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool DBManager::checkOutVisitor(int visitorId) {
    SQLHANDLE stmt = nullptr;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt))) {
        extractError("AllocStmt", dbc_, SQL_HANDLE_DBC);
        return false;
    }

    std::ostringstream ss;
    ss << "UPDATE Visitors SET CheckOutTime = GETDATE() WHERE VisitorID = " << visitorId << ";";

    SQLRETURN ret = SQLExecDirectA(stmt, (SQLCHAR*)ss.str().c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extractError("SQLExecDirect", stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool DBManager::listActiveVisitors() {
    SQLHANDLE stmt = nullptr;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt))) {
        extractError("AllocStmt", dbc_, SQL_HANDLE_DBC);
        return false;
    }

    const char* query =
        "SELECT VisitorID, FirstName, LastName, PatientRoom, CheckInTime "
        "FROM Visitors WHERE CheckOutTime IS NULL;";
    SQLRETURN ret = SQLExecDirectA(stmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        extractError("SQLExecDirect", stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    std::cout << "\nActive Visitors:\n"
        << "ID | First Name | Last Name | Room   | Check-In Time\n"
        << "------------------------------------------------------\n";

    SQLINTEGER id;
    SQLCHAR fn[51], ln[51], room[64];
    SQL_TIMESTAMP_STRUCT ts;
    SQLBindCol(stmt, 1, SQL_C_SLONG, &id, 0, nullptr);
    SQLBindCol(stmt, 2, SQL_C_CHAR, fn, sizeof(fn), nullptr);
    SQLBindCol(stmt, 3, SQL_C_CHAR, ln, sizeof(ln), nullptr);
    SQLBindCol(stmt, 4, SQL_C_CHAR, room, sizeof(room), nullptr);
    SQLBindCol(stmt, 5, SQL_C_TYPE_TIMESTAMP, &ts, 0, nullptr);

    while (SQLFetch(stmt) == SQL_SUCCESS) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
            ts.year, ts.month, ts.day, ts.hour, ts.minute);
        std::cout << id << "  | "
            << fn << "        | "
            << ln << "       | "
            << room << " | "
            << buf << "\n";
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool DBManager::getActiveVisitorsJson(nlohmann::json& outJson) {
    SQLHANDLE stmt = nullptr;
    if (SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt) != SQL_SUCCESS) {
        extractError("AllocStmt", dbc_, SQL_HANDLE_DBC);
        return false;
    }

    const char* qry =
        "SELECT VisitorID, FirstName, LastName, PatientRoom, CheckInTime "
        "FROM Visitors WHERE CheckOutTime IS NULL;";
    if (!SQL_SUCCEEDED(SQLExecDirectA(stmt, (SQLCHAR*)qry, SQL_NTS))) {
        extractError("SQLExecDirect", stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    outJson = nlohmann::json::array();
    SQLINTEGER id;
    SQLCHAR fn[51], ln[51], room[64];
    SQL_TIMESTAMP_STRUCT ts;
    SQLBindCol(stmt, 1, SQL_C_SLONG, &id, 0, nullptr);
    SQLBindCol(stmt, 2, SQL_C_CHAR, fn, sizeof(fn), nullptr);
    SQLBindCol(stmt, 3, SQL_C_CHAR, ln, sizeof(ln), nullptr);
    SQLBindCol(stmt, 4, SQL_C_CHAR, room, sizeof(room), nullptr);
    SQLBindCol(stmt, 5, SQL_C_TYPE_TIMESTAMP, &ts, 0, nullptr);

    SQLRETURN fetchRet;
    while ((fetchRet = SQLFetch(stmt)) == SQL_SUCCESS_WITH_INFO || fetchRet == SQL_SUCCESS) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
            ts.year, ts.month, ts.day, ts.hour, ts.minute);

        outJson.push_back({
            {"visitorId", id},
            {"firstName",  std::string((char*)fn)},
            {"lastName",   std::string((char*)ln)},
            {"room",       std::string((char*)room)},
            {"checkIn",    buf}
            });
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

void DBManager::disconnect() {
    if (dbc_) { SQLDisconnect(dbc_); SQLFreeHandle(SQL_HANDLE_DBC, dbc_); }
    if (env_) { SQLFreeHandle(SQL_HANDLE_ENV, env_); }
}

void DBManager::extractError(const char* fn, SQLHANDLE handle, SQLSMALLINT type) {
    SQLCHAR sqlstate[6], msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native; SQLSMALLINT len; SQLRETURN ret; SQLSMALLINT i = 1;
    std::cerr << "[" << fn << "] ODBC error:\n";
    while ((ret = SQLGetDiagRecA(type, handle, i++, sqlstate, &native,
        msg, sizeof(msg), &len)) != SQL_NO_DATA) {
        std::cerr
            << "  SQLSTATE=" << sqlstate
            << ", Code=" << native
            << ", Msg=" << msg << "\n";
    }
}
