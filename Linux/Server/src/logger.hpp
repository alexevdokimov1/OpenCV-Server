#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
public:
    Logger(const std::string& filename)
    {
        logFile.open(filename, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error opening log file.\n";
        }
    }

    ~Logger() { logFile.close(); }

    void log(LogLevel level, const std::string& message)
    {
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        char timestamp[20];
        
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        std::ostringstream logEntry;
        logEntry << "[" << timestamp << "] " << levelToString(level) << ": " << message << "\n";
        std::cout << logEntry.str();
        if (logFile.is_open()) {
            logFile << logEntry.str();
            logFile.flush();
        }
    }

private:
    std::ofstream logFile;

    std::string levelToString(LogLevel level)
    {
        switch (level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
        }
    }
};