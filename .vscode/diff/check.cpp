#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>  // For system()

namespace fs = std::filesystem;

void logMessage(const std::string& message) {
    std::ofstream logFile("log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.close();
    } else {
        std::cerr << "Unable to open log file." << std::endl;
    }
}

void backupDataFile() {
    std::string fileName = "data.txt";
    
    // Check if file exists in the present working directory
    if (fs::exists(fileName)) {
        std::string backupFileName = "data_backup.txt";
        
        // Create a backup by copying the file
        fs::copy(fileName, backupFileName, fs::copy_options::overwrite_existing);
        logMessage("Backup created for data.txt as data_backup.txt.");
        std::cout << "Backup created successfully." << std::endl;
    } else {
        logMessage("data.txt file does not exist.");
        std::cout << "data.txt file does not exist. Log entry created." << std::endl;
    }
}

int main() {
    backupDataFile();
    return 0;
}
