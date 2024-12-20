//ライブラリぼ取得
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h" 
#include <ArduinoJson.h>

//ピンアサイン
const int MOTOR1_PWM = 32;
const int MOTOR1_DIR = 33;
const int MOTOR2_PWM = 25;
const int MOTOR2_DIR = 26;
const int MOTOR3_PWM = 27;
const int MOTOR3_DIR = 14;

//各モータのデューティーを引数とし、モータにのGPIO出力をする関数
void action(int v_motor1, int v_motor2, int v_motor3){
  digitalWrite(MOTOR1_DIR,v_motor1 < 0 ? HIGH : LOW);
  digitalWrite(MOTOR2_DIR,v_motor2 < 0 ? HIGH : LOW);
  digitalWrite(MOTOR3_DIR,v_motor3 < 0 ? HIGH : LOW);
  
  analogWrite(MOTOR1_PWM,abs(v_motor1));
  analogWrite(MOTOR2_PWM,abs(v_motor2));
  analogWrite(MOTOR3_PWM,abs(v_motor3));
}

//サーバの設定用の変数を作る
const char ssid[] = "ESP32AP-TEST";
const char pass[] = "12345678";       // パスワードは8文字以上
const IPAddress ip(192,168,123,45);
const IPAddress subnet(255,255,255,0);
AsyncWebServer server(80);            // ポート設定

// Jsonオブジェクトの初期化
StaticJsonDocument<512> doc;

//GUIから取得した値を格納する変数を作る
int x_status;
int y_status;
int left_spin;
int right_spin;

void setup() {
  Serial.begin(115200); //シリアルポートを設定
  pinMode(MOTOR1_DIR,OUTPUT); //出力ピンに指定
  pinMode(MOTOR2_DIR,OUTPUT);
  pinMode(MOTOR3_DIR,OUTPUT);

  // SPIFFSのセットアップ
  if(!SPIFFS.begin(true)){
    return;
  }
  
  WiFi.softAP(ssid, pass); // SSIDとパスの設定
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet); // IPアドレス、ゲートウェイ、サブネットマスクの設定
  
  IPAddress myIP = WiFi.softAPIP(); //WiFi.softAPIP()でWiFi起動

  // rootにアクセスされた時のレスポンス
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  // style.cssにアクセスされた時のレスポンス
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // モータの制御変数の変更リクエスト
  server.on("/post_test", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      String resjson = ""; //jsonで送られてきた文字列を格納する
 
      for (size_t i = 0; i < len; i++) {
        resjson.concat(char(data[i]));
      }
 
      Serial.println(resjson);
      DeserializationError error = deserializeJson(doc, resjson);

      if(error){
        request->send(400);
        action(0,0,0);
      }
      else{
        //それぞれのパラメータを変数に格納
        x_status = doc["X_STICK_STATUS"];
        y_status = doc["Y_STICK_STATUS"];
        left_spin = doc["SPIN_LEFT"];
        right_spin = doc["SPIN_RIGHT"];

        y_status = -1*y_status; //動かしたときに逆だったので補正

        //モータへの出力の変数を作る
        int motor1_duty;
        int motor2_duty;
        int motor3_duty;
       //以下の2つのパラメータを変えることで移動速度が変わる
        const double duty_gain = 1;
        const int spin_gain = 100;

　　　　　//3輪オムニホイールのモータのデューティーを出す関数
        //デッドゾーンを5px分に指定
        if (abs(x_status) > 5 || abs(y_status) > 5 || left_spin ==1 || right_spin == 1){
          motor1_duty = duty_gain * x_status + spin_gain*left_spin - spin_gain*right_spin;
          motor2_duty = -1 * duty_gain * 0.5 * x_status - duty_gain * 0.86602540378 * y_status + spin_gain*left_spin - spin_gain*right_spin;
          motor3_duty = duty_gain * -0.5 * x_status + duty_gain * 0.86602540378 * y_status + spin_gain*left_spin - spin_gain*right_spin;
        }else{
          motor1_duty = 0;
          motor2_duty = 0;
          motor3_duty = 0;
        }
　　　　　　action(motor1_duty,motor2_duty,motor3_duty);　//モータを回転する関数に引数を渡す

        request->send(200);
      }
  });

  // サーバースタート
  server.begin();
}

void loop() {}
