#include <iostream>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "DBManager.h"

int main() {
    DBManager db;
    if (!db.connect()) {
        std::cerr << "❌ DB connection failed\n";
        return 1;
    }

    httplib::Server svr;
    svr.set_mount_point("/", "./public");

    // Health-check endpoint
    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
        });

    // Check-in endpoint
    svr.Post("/checkin", [&](const httplib::Request& req, httplib::Response& res) {
        try {
        auto j = nlohmann::json::parse(req.body);
        bool ok = db.checkInVisitor(
            j.at("firstName"), j.at("lastName"),
            j.at("phone"), j.at("room")
        );
        res.status = ok ? 200 : 500;
        res.set_content(ok ? "Checked in" : "Error", "text/plain");
    }
    catch (...) {
        res.status = 400;
        res.set_content("Bad request", "text/plain");
    }
        });

    // Check-out endpoint
    svr.Post("/checkout", [&](const httplib::Request& req, httplib::Response& res) {
        try {
        auto j = nlohmann::json::parse(req.body);
        bool ok = db.checkOutVisitor(j.at("visitorId").get<int>());
        res.status = ok ? 200 : 500;
        res.set_content(ok ? "Checked out" : "Error", "text/plain");
    }
    catch (...) {
        res.status = 400;
        res.set_content("Bad request", "text/plain");
    }
        });

    // Active visitors endpoint
    svr.Get("/active", [&](const httplib::Request&, httplib::Response& res) {
        nlohmann::json out;
    if (db.getActiveVisitorsJson(out)) {
        res.set_content(out.dump(2), "application/json");
    }
    else {
        res.status = 500;
        res.set_content("Error querying DB", "text/plain");
    }
        });

    std::cout << "Server running on http://[::]:18080\n";
    svr.listen("::", 18080);
    return 0;
}
