// ライブラリのインクルード
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal.h>

// wi-fiの設定
const char* ssid = "";
const char* password = "";

// 変数の初期化
const String url = "";
char *point[7] = {"sun","mon","tue","wed","thu","fri","sat"};
int weeks[7][2];
int start_time[2];
int end_time[2];
int mo=0;

// PINの設定
int button_pin = 14;
LiquidCrystal lcd(22,23,5,18,19,21);

void setup(){
  Serial.begin(115200);
  // wifiの接続
  lcd.begin(16, 0);
  lcd.print("connecting");
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    lcd.print(".");
  }
  // ピンの設定
  pinMode(button_pin, INPUT_PULLUP);
  String da = get_timer("name");
}

void loop(){
  // 時間を取得
  configTime(9*3600,0,"pool.ntp.org");
  struct tm timeInfo;
  getLocalTime(&timeInfo);
  int w = timeInfo.tm_wday;
  int h = timeInfo.tm_hour;
  int m = timeInfo.tm_min;
  lcd.begin(16, 0);
  lcd.clear();
  if(m<10){
    lcd.print(String(h)+":0"+String(m));
  }else{
    lcd.print(String(h)+":"+String(m));
  }
  // ボタンを検知
  int button_status = digitalRead(button_pin);
  if (button_status==0 and mo==0){
    start_time[0]=h;
    start_time[1]=m;
    Serial.println("ON");
    mo=1;
  }else if(button_status==1 and mo==1){
    // 寝た時間と起きた時間から睡眠時間を求める
    Serial.println("OFF");
    end_time[0]=h;
    end_time[1]=m;
    String user = "name";
    int sleep_time = calc_time(start_time, end_time);
    String result = post_sleep_time(user,sleep_time);
    lcd.begin(16,1);
    lcd.clear();
    lcd.print(sleep_time);
    delay(1000);
    mo=0;
  }
  // nn:00になったときにタイマーの時間を取得
  if (m==0){
    String da = get_timer("name");
    Serial.print(weeks[w][0]+":"+weeks[w][1]);
  }
  // 設定したタイマーと現在の時間が一致したときにアラームを鳴らす
  if(h==weeks[w][0] and m==weeks[w][1]){
    Serial.println("alarm ON!!!");
  }
  delay(500);
}

// タイマーの時間を取得
String get_timer(String user){
  const String key = "get_timer?user=" + user;
  if ((WiFi.status()==WL_CONNECTED)){
    HTTPClient http;
    http.begin(url+key);
    // アクセスしたときの結果の状態を保存する変数(ミスのときはマイナスの値)
    int httpCode = http.GET();
    if (httpCode > 0){
      String payload = http.getString();
      http.end();
      JSONVar json = JSON.parse(payload);
      for (int i=0; i<=6; i++){
        String key = point[i];
        int value = int(json[key]);
        weeks[i][0]=value/100;
        weeks[i][1]=value%100;
        Serial.println(key+"="+value);
      }
      return payload;
    }else{
      http.end();
      return "error";
    }
  }
}

// 睡眠時間を記録
String post_sleep_time(String user,int t){
  const String key = "post_sleep_time";
  String json = "{\"user\":\""+user+"\",\"time\":\""+t+"\"}";
  if(WiFi.status()==WL_CONNECTED){
    HTTPClient http;
    http.begin(url+key);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(json);
    if(httpCode==200){
      http.end();
      return "clear";
    }else{
      http.end();
      return "eroor:"+httpCode;
    }
  }
}

int calc_time(int s[],int e[]){
  int ho=0;
  int mi=0;
  if(s[0]>e[0]){
    ho=(24-s[0]-1)+e[0];
    mi=(60-s[1])+e[1];
  }else if(s[0]<e[0]){
    ho=e[0]-s[0];
    mi=(60+e[1])-s[1];
  }else{
    ho= 0;
    mi=e[1]-s[1];
  }
  int sum = (ho*60) + mi;
  return sum;
}
