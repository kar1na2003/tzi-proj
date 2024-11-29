#include <iostream>
#include <string>
#include <map>
#include <winsock2.h>
#include "sqlite/sqlite3.h"
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

std::string generateCode() {
    const std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string code;
    for (int i = 0; i < 6; ++i) {
        code += charset[rand() % charset.size()];
    }
    return code;
}

std::string getFirstPart(const std::string& input) {
    size_t plusIndex = input.find('+');

    if (plusIndex != std::string::npos) {
        return input.substr(0, plusIndex);
    }

    return input;
}

std::string urlDecode(const std::string& str) {
    std::string result;
    size_t len = str.length();

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len) {
            int value;
            std::stringstream ss;
            ss << std::hex << str.substr(i + 1, 2);
            ss >> value;
            result += static_cast<char>(value);
            i += 2;  // Skip the next two characters
        }
        else if (str[i] == '+') {
            result += ' ';
        }
        else {
            result += str[i];
        }
    }

    return result;
}

void parseFormData(const std::string& request, std::map<std::string, std::string>& formData) {
    size_t bodyStart = request.find("\r\n\r\n");  
    if (bodyStart != std::string::npos) {
        std::string body = request.substr(bodyStart + 4);  

        std::istringstream bodyStream(body);
        std::string keyValue;
        while (std::getline(bodyStream, keyValue, '&')) {
            size_t equalPos = keyValue.find('=');
            if (equalPos != std::string::npos) {
                std::string key = keyValue.substr(0, equalPos);
                std::string value = keyValue.substr(equalPos + 1);
                formData[key] = urlDecode(value);  
            }
        }
    }
}

std::string decodeEmail(const std::string& encodedEmail) {
    std::string decodedEmail = encodedEmail;


    size_t pos = decodedEmail.find("%40");
    while (pos != std::string::npos) {
        decodedEmail.replace(pos, 3, "@");
        pos = decodedEmail.find("%40", pos + 1);
    }

    return decodedEmail;
}

std::map<std::string, std::string> parseFormData(std::string& body) {
    std::map<std::string, std::string> formData;
    size_t pos = 0;
    while ((pos = body.find("&")) != std::string::npos) {
        std::string token = body.substr(0, pos);
        size_t equalPos = token.find("=");
        if (equalPos != std::string::npos) {
            formData[token.substr(0, equalPos)] = token.substr(equalPos + 1);
        }
        body.erase(0, pos + 1);
    }
    size_t equalPos = body.find("=");
    if (equalPos != std::string::npos) {
        formData[body.substr(0, equalPos)] = body.substr(equalPos + 1);
    }
    return formData;
}

void handleClient(SOCKET clientSocket, sqlite3* db) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving data from client." << std::endl;
        return;
    }

    buffer[bytesReceived] = '\0';
    std::string request(buffer);

    std::cout << "Request received:\n" << request << std::endl;

    if (request.find("GET / ") != std::string::npos) {
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        response += R"(
            <html>
                <head>
                    <meta charset="UTF-8">
                    <meta name="viewport" content="width=device-width, initial-scale=1.0">
                    <title>Register and Login</title>
                    <style>
                        body {
                            font-family: Arial, sans-serif;
                            background-color: #f4f4f9;
                            margin: 0;
                            padding: 0;
                            display: flex;
                            justify-content: center;
                            align-items: center;
                            height: 100vh;
                        }
                        .container {
                            background-color: #ffffff;
                            border: 1px solid #ddd;
                            border-radius: 8px;
                            padding: 20px;
                            width: 300px;
                            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
                        }
                        h1 {
                            font-size: 1.5em;
                            color: #333;
                            margin-bottom: 15px;
                            text-align: center;
                        }
                        form {
                            margin-bottom: 20px;
                        }
                        input[type="text"], input[type="password"] {
                            width: 100%;
                            padding: 8px;
                            margin: 8px 0;
                            border: 1px solid #ccc;
                            border-radius: 4px;
                        }
                        input[type="submit"] {
                            width: 100%;
                            padding: 10px;
                            background-color: #007bff;
                            color: #ffffff;
                            border: none;
                            border-radius: 4px;
                            cursor: pointer;
                            font-size: 1em;
                        }
                        input[type="submit"]:hover {
                            background-color: #0056b3;
                        }
                        hr {
                            border: 0;
                            height: 1px;
                            background: #ddd;
                            margin: 20px 0;
                        }
                    </style>
                </head>
                <body>
                    <h1>Register</h1>
                    <br>
                    <form action="/adduser" method="POST">
                        Email: <input type="text" name="email"><br>
                        Password: <input type="password" name="password"><br>
                        <input type="submit" value="Register">
                    </form>
                    <hr>
                    <h1>Login</h1>
                    <form action="/adduser" method="POST">
                        Email: <input type="text" name="email"><br>
                        Password: <input type="password" name="password"><br>
                        <input type="submit" value="Login">
                    </form>
                </body>
            </html>
        )";
        send(clientSocket, response.c_str(), response.length(), 0);
    }

    else if (request.find("POST /adduser") != std::string::npos) {
        size_t contentPos = request.find("\r\n\r\n");
        if (contentPos != std::string::npos) {
            std::string body = request.substr(contentPos + 4);
            std::map<std::string, std::string> formData = parseFormData(body);

            auto emailIt = formData.find("email");
            auto passwordIt = formData.find("password");

            if (emailIt != formData.end() && passwordIt != formData.end()) {
                std::string email = emailIt->second;
                std::string password = passwordIt->second;


                std::string authCode = generateCode();
                email = decodeEmail(email);
                std::string command = "python main.py " + email + " " + authCode;
                int result = system(command.c_str());

                if (result == 0) {
                    std::cout << "Email sent successfully!" << std::endl;
                }
                else {
                    std::cout << "Error sending email." << std::endl;
                }



                const char* insertSQL = "INSERT INTO users (email, password, auth_code) VALUES (?, ?, ?);";
                sqlite3_stmt* stmt;
                if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
                    std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << std::endl;
                    return;
                }

                sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, authCode.c_str(), -1, SQLITE_STATIC);

                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    std::cerr << "Error executing SQL statement: " << sqlite3_errmsg(db) << std::endl;
                }

                sqlite3_finalize(stmt);

                std::string response = "HTTP/1.1 302 Found\r\nLocation: /authcode?code=" + authCode + "\r\n\r\n";
                send(clientSocket, response.c_str(), response.length(), 0);
            }
        }
    }
    else if (request.find("GET /authcode") != std::string::npos) {
        size_t codePos = request.find("code=");
        if (codePos != std::string::npos) {
            std::string code = request.substr(codePos + 5);
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

            response += R"(
        <html>
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Register and Login</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    background-color: #f9f9f9;
                    margin: 0;
                    padding: 20px;
                }
                h1 {
                    font-size: 1.5em;
                    color: #333;
                    margin-bottom: 10px;
                }
                form {
                    background-color: #fff;
                    border: 1px solid #ddd;
                    border-radius: 5px;
                    padding: 15px;
                    margin-bottom: 20px;
                    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
                }
                input[type="text"], input[type="password"] {
                    width: 100%;
                    padding: 8px;
                    margin: 10px 0;
                    border: 1px solid #ccc;
                    border-radius: 4px;
                }
                input[type="submit"] {
                    width: 100%;
                    padding: 10px;
                    background-color: #007bff;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    cursor: pointer;
                    font-size: 1em;
                }
                input[type="submit"]:hover {
                    background-color: #0056b3;
                }
                hr {
                    border: 0;
                    height: 1px;
                    background: #ddd;
                    margin: 20px 0;
                }
            </style>
        </head>
        <body>
            <form action="/verifycode" method="POST">
                Enter Code: <input type="text" name="usercode"><br>
                <input type="hidden" name="expectedcode" value=")" + code + R"(">
                <input type="submit" value="Verify Code">
            </form>
        </body></html>
        )";
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }
    else if (request.find("POST /verifycode") != std::string::npos) {
        size_t contentPos = request.find("\r\n\r\n");
        if (contentPos != std::string::npos) {
            std::string body = request.substr(contentPos + 4);
            std::map<std::string, std::string> formData = parseFormData(body);

            auto userCodeIt = formData.find("usercode");
            auto expectedCodeIt = formData.find("expectedcode");

            if (userCodeIt != formData.end() && expectedCodeIt != formData.end()) {
                std::string userCode = userCodeIt->second;
                std::string expectedCode = expectedCodeIt->second;
                expectedCode = getFirstPart(expectedCode);

                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                response += R"(
                <!DOCTYPE html>
                <html lang="en">
                <head>
                    <meta charset="UTF-8">
                    <meta name="viewport" content="width=device-width, initial-scale=1.0">
                    <title>Code Verification</title>
                    <style>
                        body {
                            font-family: Arial, sans-serif;
                            background-color: #f9f9f9;
                            margin: 0;
                            padding: 20px;
                            display: flex;
                            justify-content: center;
                            align-items: center;
                            height: 100vh;
                        }
                        .container {
                            text-align: center;
                            background-color: #fff;
                            border: 1px solid #ddd;
                            border-radius: 8px;
                            padding: 20px;
                            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
                            width: 300px;
                        }
                        h1 {
                            font-size: 1.5em;
                            color: #333;
                            margin-bottom: 15px;
                        }
                        p {
                            font-size: 1em;
                            color: #555;
                        }
                        .success {
                            color: #28a745;
                        }
                        .error {
                            color: #dc3545;
                        }
                    </style>
                </head>
                )";
                response += "<body>";
                response += R"(<div class="container">)";
                if (userCode == expectedCode) {
                    response += "<h1 class='success'>Code Verified!</h1>";
                    response += "<p>The code you entered matches.</p>";
                }
                else {
                    response += "<h1 class='error'>Verification Failed</h1>";
                    response += "<p>The code you entered is incorrect.</p>";
                }
                response += "</div>";
                response += "</body></html>";

                send(clientSocket, response.c_str(), response.length(), 0);
            }
            else {
                std::cerr << "Error: Missing usercode or expectedcode" << std::endl;
            }
        }
    }
}

void createDatabase() {
    sqlite3* db;
    if (sqlite3_open("users.db", &db) != SQLITE_OK) {
        std::cerr << "Error creating database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            email TEXT UNIQUE,
            password TEXT,
            auth_code TEXT
        );
    )";

    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error creating table: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_close(db);
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    createDatabase();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing WinSock." << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is running on port 8080..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error accepting client connection." << std::endl;
            continue;
        }

        sqlite3* db;
        if (sqlite3_open("users.db", &db) != SQLITE_OK) {
            std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
            
            continue;
        }

        handleClient(clientSocket, db);
        
        sqlite3_close(db);
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
