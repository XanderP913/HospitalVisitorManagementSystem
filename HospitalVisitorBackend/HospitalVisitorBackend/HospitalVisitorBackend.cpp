#include <iostream>
#include <string>
#include <limits>
#include "DBManager.h"

int main() {
    DBManager db;
    if (!db.connect()) {
        std::cerr << "❌ Failed to connect to DB\n";
        return -1;
    }

    while (true) {
        std::cout << "\n=== Hospital Visitor Management ===\n"
            << "1) Check In Visitor\n"
            << "2) Check Out Visitor\n"
            << "3) List Active Visitors\n"
            << "4) Exit\n"
            << "Select an option: ";

        int choice;
        if (!(std::cin >> choice)) break;
        // now that <limits> is included, this compiles:
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1) {
            std::string fn, ln, phone, room;
            std::cout << "First name: ";   std::getline(std::cin, fn);
            std::cout << "Last name:  ";   std::getline(std::cin, ln);
            std::cout << "Phone:      ";   std::getline(std::cin, phone);
            std::cout << "Room (e.g. 2D-10): ";
            std::getline(std::cin, room);

            if (db.checkInVisitor(fn, ln, phone, room))
                std::cout << "✔ Checked in.\n";
            else
                std::cout << "✖ Check-in failed.\n";

        }
        else if (choice == 2) {
            int id;
            std::cout << "VisitorID to check out: ";
            std::cin >> id;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (db.checkOutVisitor(id))
                std::cout << "✔ Checked out.\n";
            else
                std::cout << "✖ Check-out failed.\n";

        }
        else if (choice == 3) {
            db.listActiveVisitors();

        }
        else if (choice == 4) {
            break;

        }
        else {
            std::cout << "Invalid option.\n";
        }
    }

    std::cout << "Goodbye!\n";
    return 0;
}
