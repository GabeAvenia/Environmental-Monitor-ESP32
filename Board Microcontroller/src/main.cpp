#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true
#define FILE_PATH "/config/data.txt"
#define BUFFER_SIZE 128

void handleCommand(String command);
void writeFile(fs::FS &fs, const char *path, const char *message);
void readFile(fs::FS &fs, const char *path);

void setup() {
    Serial.begin(115200);
    delay(2000);  // Delay for 2 seconds
    // Initialize LittleFS
    LittleFS.begin(true, "/littlefs", 10, "ffat");
}

void loop() {
    static String inputBuffer = "";

    // Read characters from Serial and process when a newline is received
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                handleCommand(inputBuffer);
                inputBuffer = "";
            }
        } else {
            inputBuffer += c;
        }
    }
}

// Process SCPI commands
void handleCommand(String command) {
    command.trim();  // Remove leading and trailing spaces

    if (command.startsWith(":FILE:WRITE ")) {
        // Extract message enclosed in quotes
        int firstQuote = command.indexOf('"');
        int lastQuote = command.lastIndexOf('"');

        if (firstQuote != -1 && lastQuote != -1 && firstQuote < lastQuote) {
            String message = command.substring(firstQuote + 1, lastQuote);
            writeFile(LittleFS, FILE_PATH, message.c_str());
            Serial.println("OK");
        } else {
            Serial.println("ERROR: Invalid syntax. Use :FILE:WRITE \"message\"");
        }

    } else if (command == ":FILE:READ?") {
        readFile(LittleFS, FILE_PATH);

    } else {
        Serial.println("ERROR: Unknown command");
    }
}

// Function to write data to a file
void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\n", path);

    // Extract directory from the path ("/config/data.txt" -> "/config/")
    String directory = String(path);
    int lastSlash = directory.lastIndexOf('/');
    if (lastSlash > 0) {
        directory = directory.substring(0, lastSlash);
        if (!fs.exists(directory.c_str())) {
            Serial.printf("Creating missing directory: %s\n", directory.c_str());
            fs.mkdir(directory.c_str());
        }
    }

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("ERROR: Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written successfully.");
    } else {
        Serial.println("ERROR: Write failed");
    }
    file.close();
}

void listFiles(fs::FS &fs) {
    Serial.println("Listing files in LittleFS:");
    File root = fs.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("ERROR: Failed to open LittleFS root directory.");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        Serial.printf("  FILE: %s\tSIZE: %d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char *path) {
    Serial.printf("Attempting to read file: %s\n", path);

    listFiles(fs);  // List files before reading

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("ERROR: Failed to open file for reading");
        return;
    }

    Serial.println("DATA:");
    while (file.available()) {
        Serial.write(file.read());
    }
    Serial.println();
    file.close();
}