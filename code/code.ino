//=============================================================================
// Track Bitcoin prices automatically with an ESP32 and an OLED display
// Created on 08 Feb 2025
// Inspired by Lucas Fernando (https://www.youtube.com/@lucasfernandochannel)
// You are free to use this code the way you want
//=============================================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SH110X.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Replace with your CoinAPI key
const char* api_key = "";
const char* api_url = "https://rest.coinapi.io/v1/exchangerate/BTC/USD";

// Display constants
#define WHITE SH110X_WHITE  ///< Draw 'on' pixels
#define i2c_Address 0x3c

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     //   QT-PY / XIAO

// Define the waiting time between the API calls
const int waiting_time = 15;  // Wait for 15 minutes
const int smallDelay = 100;
const int time_shift = +2;  // Time shift / time zone

// Initialize the SH1106G display
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


WiFiClient wifiClient;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  oled.begin(i2c_Address, true);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setRotation(0);

  // Connect to Wi-Fi
  connectToWiFi();

  // Make API call and display the result
  displayBitcoinPrice();
}

void loop() {
  delay(waiting_time * 60 * 1000);
  displayBitcoinPrice();
}

void connectToWiFi() {
  oled.setCursor(0, 0);
  oled.setTextSize(2);
  oled.print("Connecting to Wi-Fi");
  oled.display();

  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  while (WiFi.status() != WL_CONNECTED) {
    delay(smallDelay);
    Serial.println("Connecting...");
  }
  oled.clearDisplay();
  oled.setCursor(0, 0);
  Serial.println("Connected to Wi-Fi");
  oled.print("Connected to Wi-Fi");
  oled.display();
}

String formatTime(String time) {
  // Extract date and time components
  String date = time.substring(0, 10);
  String timeOfDay = time.substring(11, 16);

  // Convert date components
  String year = date.substring(0, 4);   // Year
  String month = date.substring(5, 7);  // Month
  String day = date.substring(8, 10);   // Day

  // Convert the month number to the month name
  String monthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  int monthIndex = month.toInt() - 1;
  String monthName = monthNames[monthIndex];

  // Convert time from 24-hour to 12-hour format
  int hour = timeOfDay.substring(0, 2).toInt();
  String minute = timeOfDay.substring(3, 5);  // Minute
  String period = "AM";

  if (hour >= 12) {
    period = "PM";
    if (hour > 12) hour -= 12;
  } else if (hour == 0) {
    hour = 12;  // Midnight case
  }

  String formattedTime = monthName + " " + day + "," + year + ",";
  formattedTime += String(hour + time_shift) + ":" + minute + "" + period;

  return formattedTime;
}

void displayBitcoinPrice() {
  if (WiFi.status() == WL_CONNECTED) {

    // Load data
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.setTextSize(2);
    oled.print("Loading");

    HTTPClient http;
    http.begin(api_url);
    oled.print(".");
    oled.display();
    http.addHeader("X-CoinAPI-Key", api_key);
    delay(smallDelay);
    oled.print(".");
    oled.display();

    int httpCode = http.GET();  // Make the request
    delay(smallDelay);
    oled.print(".");
    oled.display();
    Serial.println(httpCode);
    String payload = http.getString();
    delay(smallDelay);
    Serial.println(payload);
    oled.print(".");
    oled.display();

    if (httpCode > 0) {
      Serial.println(payload);

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      Serial.print("deserializeJson() returned ");
      Serial.println(error.c_str());

      float price = doc.as<JsonObject>()["rate"].as<float>();
      String date = doc.as<JsonObject>()["time"].as<String>();
      Serial.println(price);
      Serial.println(date);

      // Display the price on the OLED
      oled.clearDisplay();

      oled.setCursor(8, 0);
      oled.setTextSize(1);
      oled.print("Bitcoin Price, USD");

      oled.setTextSize(3);

      if (price >= 100000) {
        oled.setCursor(0, 15);
      } else {
        oled.setCursor(10, 15);
      }

      oled.print(price / 1000, 3);

      String formatted_date = formatTime(date);

      oled.setTextSize(1);
      oled.setCursor(10, 45);
      oled.print(formatted_date);
      oled.setCursor(10, 55);
      oled.print("Upd. every " + String(waiting_time) + " mins");

    } else {
      Serial.println("Error on HTTP request");
      Serial.println(httpCode);

      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.print("Error on HTTP request");
      oled.setCursor(0, 25);
      oled.setTextSize(2);
      oled.print(httpCode);
    }
    http.end();  // End the request
  }
  oled.display();
}
