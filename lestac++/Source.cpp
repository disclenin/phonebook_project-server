#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

const int PORT = 8080;
const std::string DATA_FILE = "phonebook.txt";

struct Record {
    std::string firstName;
    std::string lastName;
    std::string phoneNumber;
    std::string note;
};

std::map<std::string, Record> phonebook;

void loadPhonebook() {
    std::ifstream file(DATA_FILE);
    if (file) {
        std::string line;
        while (getline(file, line)) {
            std::istringstream iss(line);
            Record rec;
            if (iss >> rec.firstName >> rec.lastName >> rec.phoneNumber >> std::ws && std::getline(iss, rec.note)) {
                phonebook[rec.firstName + " " + rec.lastName] = rec;
            }
        }
        file.close();
    }
}

void savePhonebook() {
    std::ofstream file(DATA_FILE);
    if (file) {
        for (auto& entry : phonebook) {
            file << entry.second.firstName << " " << entry.second.lastName << " "
                << entry.second.phoneNumber << " " << entry.second.note << "\n";
        }
        file.close();
    }
}

std::string processRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string command, firstName, lastName, phoneNumber, note;
    iss >> command >> firstName >> lastName >> phoneNumber >> std::ws;
    std::getline(iss, note);
    std::string key = firstName + " " + lastName;

    if (command == "add") {
        phonebook[key] = { firstName, lastName, phoneNumber, note };
        savePhonebook();
        return "Record added successfully.";
    }
    else if (command == "delete") {
        if (phonebook.erase(key)) {
            savePhonebook();
            return "Record deleted successfully.";
        }
        else {
            return "Record not found.";
        }
    }
    else if (command == "search") {
        if (phonebook.find(key) != phonebook.end()) {
            const Record& rec = phonebook[key];
            return "Found: " + rec.firstName + " " + rec.lastName + ", Phone: " + rec.phoneNumber + ", Note: " + rec.note;
        }
        else {
            return "Record not found.";
        }
    }
    else if (command == "view") {
        if (phonebook.find(key) != phonebook.end()) {
            const Record& rec = phonebook[key];
            return "Record: " + rec.firstName + " " + rec.lastName + ", Phone: " + rec.phoneNumber + ", Note: " + rec.note;
        }
        else {
            return "Record not found.";
        }
    }
    return "Invalid command.";
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };

    loadPhonebook();

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    while ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) >= 0) {
        read(new_socket, buffer, 1024);
        std::string response = processRequest(std::string(buffer));
        send(new_socket, response.c_str(), response.length(), 0);
        close(new_socket);
    }
    return 0;
}
