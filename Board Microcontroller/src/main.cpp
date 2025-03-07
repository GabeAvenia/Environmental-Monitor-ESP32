#define FILE_PATH "/config/data.txt"
#define BUFFER_SIZE 128#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"

// Global flag to track if filesystem is available
bool filesystemAvailable = false;
// Counter for indexing serial prints
uint32_t printIndex = 0;

// Create an instance of the SHT4x sensor
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// Function declarations
void handleCommand(String command);
void writeFile(fs::FS &fs, const char *path, const char *message);
void readFile(fs::FS &fs, const char *path);
void listFiles(fs::FS &fs);
void readSensor();

// Variable to track time for sensor readings
unsigned long lastSensorReadTime = 0;
const unsigned long sensorReadInterval = 1000; // 1 second in milliseconds

void setup() {
    // USB Mass Storage should be disabled via platformio.ini settings
    
    Serial.begin(115200);
    delay(2000);  // Delay for 2 seconds to allow serial connection
    
    // Initialize LittleFS with the specified parameters
    if (!LittleFS.begin(true, "/litlefs", 10, "ffat")) {
        Serial.println("LittleFS Mount Failed. Continuing without file system...");
        filesystemAvailable = false;
    } else {
        Serial.println("LittleFS mounted successfully");
        filesystemAvailable = true;
    }
    
    // Initialize the second I2C bus (Wire1) for STEMMA QT connector
    // STEMMA QT pins on Adafruit QT Py ESP32-S3 are 40 (SCL1) and 41 (SDA1)
    Wire1.begin(41, 40);
    
    Serial.println("SHT41 Temperature and Humidity Sensor Test");
    
    // Initialize the SHT4x sensor on Wire1
    if (!sht4.begin(&Wire1)) {
        Serial.println("Couldn't find SHT41 sensor on the STEMMA QT connector!");
        // Continue anyway, as the sensor might be connected later
    } else {
        Serial.println("SHT41 sensor found!");
        
        // Set precision to high
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        Serial.println("Precision set to HIGH");
        
        // Disable heater
        sht4.setHeater(SHT4X_NO_HEATER);
        Serial.println("Heater disabled");
    }
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
    
    // Read sensor at regular intervals if serial is open
    unsigned long currentMillis = millis();
    if (Serial && (currentMillis - lastSensorReadTime >= sensorReadInterval)) {
        readSensor();
        lastSensorReadTime = currentMillis;
    }
}

// Read temperature and humidity from the SHT41 sensor
void readSensor() {
    // Read temperature and humidity
    sensors_event_t humidity, temp;
    
    // Check if sensor read was successful
    if (sht4.getEvent(&humidity, &temp)) {
        // Print with index and formatted temperature and humidity
        Serial.print("INDEX:");
        Serial.print(printIndex++);
        Serial.print(" | TEMP:");
        Serial.print(temp.temperature, 2);
        Serial.print("C | HUM:");
        Serial.print(humidity.relative_humidity, 2);
        Serial.println("%");
    } else {
        Serial.println("Failed to read from SHT41 sensor!");
    }
}

// Process SCPI commands
void handleCommand(String command) {
    command.trim();  // Remove leading and trailing spaces

    if (command.startsWith(":FILE:WRITE ")) {
        if (!filesystemAvailable) {
            Serial.println("ERROR: Filesystem not available");
            return;
        }
        
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
        if (!filesystemAvailable) {
            Serial.println("ERROR: Filesystem not available");
            return;
        }
        readFile(LittleFS, FILE_PATH);
    
    } else if (command == ":FILE:LIST?") {
        if (!filesystemAvailable) {
            Serial.println("ERROR: Filesystem not available");
            return;
        }
        listFiles(LittleFS);
    
    } else if (command == ":SENSOR:READ?") {
        readSensor();
        
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