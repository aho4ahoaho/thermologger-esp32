#include <M5AtomS3.h>
#include "M5_ENV.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

SHT3X sht30(0x44, 2);
QMP6988 qmp6988;

#define SSID "IODATA-194080-2G"
#define WiFi_PASS "Fm7JL95950907"

#define HOST "http://192.168.1.16:8080/api/post"

WiFiMulti WiFiMulti;

float tmp = 0.0;
float hum = 0.0;
float pressure = 0.0;

void setup() {
  M5.begin(true, true, false, false);
  //センサ初期化
  Wire.begin(2, 1);
  qmp6988.init();
  M5.Lcd.println(F("ENVIII Unit(SHT30 and QMP6988)"));
  USBSerial.println(F("ENVIII Unit(SHT30 and QMP6988)"));

  //WiFi接続処理
  delay(10);
  USBSerial.print("Waiting for WiFi... ");
  M5.Lcd.print("Waiting for WiFi... ");
  WiFiMulti.addAP(SSID, WiFi_PASS);
  //接続待機
  while (WiFiMulti.run() != WL_CONNECTED) {
    USBSerial.print(".");
    M5.Lcd.print(".");
    delay(500);
  }
  //接続表示
  USBSerial.println("IP address: ");
  M5.Lcd.print("IP address: ");
  USBSerial.println(WiFi.localIP());
  M5.Lcd.print(WiFi.localIP());

  delay(100);
}

void loop() {
  pressure = qmp6988.calcPressure();
  if (sht30.get() == 0) {  // Obtain the data of shT30.  获取sht30的数据
    tmp = sht30.cTemp;     // Store the temperature obtained from shT30.
                           // 将sht30获取到的温度存储
    hum = sht30.humidity;  // Store the humidity obtained from the SHT30.
                           // 将sht30获取到的湿度存储
  } else {
    tmp = 0, hum = 0;
  }
  USBSerial.println("-----");
  USBSerial.printf(
    "Temp: %2.1f\tHumi: %2.1f%%\tPressure:%2.0fPa\n", tmp,
    hum, pressure);
  view_data();
  delay(send_data());
}

void view_data() {
  //画面の消去
  M5.Lcd.fillScreen(TFT_WHITE);

  //温度描画
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_RED);
  M5.Lcd.setTextSize(5);
  M5.Lcd.printf("%2.1f", tmp);
  M5.Lcd.setCursor(100, 50);
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("C");
  M5.Lcd.fillEllipse(92, 50, 6, 6, TFT_RED);
  M5.Lcd.fillEllipse(92.75, 50.75, 3, 3, TFT_WHITE);

  //その他情報
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.setCursor(0, 70);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("%2.0f%%\n%2.0fhPa", hum, pressure / 100);
}

int send_data() {
  int intervalTime = 2000;
  if (WiFiMulti.run() == WL_CONNECTED) {
    //接続先を作成
    char url[255];
    sprintf(url, "%s?temperature=%2.2f&humidity=%2.1f&pressure=%.0f", HOST, tmp, hum, pressure);
    //USBSerial.println(url);

    //HTTPクライアントの作成
    HTTPClient http;
    http.begin(url);

    //応答の処理
    int16_t httpCode = http.GET();
    if (httpCode > 0) {
      USBSerial.printf("%d %s\n", httpCode, url);
      if (httpCode == HTTP_CODE_OK) {
        String res = http.getString();
        //USBSerial.println(res);
        StaticJsonDocument<96> data;
        DeserializationError err = deserializeJson(data, res);
        if (!err) {
          intervalTime = data["interval"];
          //USBSerial.println(intervalTime);
        } else {
          USBSerial.print("deserializeJson() failed: ");
          USBSerial.println(err.c_str());
        }
      }
    } else {
      USBSerial.printf("Connection Failed\n");
    }
    http.end();
  }
  return intervalTime;
}