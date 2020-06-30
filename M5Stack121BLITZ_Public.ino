#include <ArduinoJson.h>

#include "BLITZSplash.c" //calls opennode logo

#include <M5Stack.h> 
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> 

//Wifi details
char wifiSSID[] = "TrickTopHat";
char wifiPASS[] = "0000000023";


//BLITZ DETAILS
const char*  server = "10.0.0.26"; 
const int httpsPort = 443;
const int lndport = 8080;
String pubkey;
String totcapacity;

String readmacaroon = "0201036C6E64028A01030A1075E0FAFD1DD4922E080E1FBAFD1675D81201301A0F0A07616464726573731204726561641A0C0A04696E666F1204726561641A100A08696E766F696365731204726561641A0F0A076D6573736167651204726561641A100A086F6666636861696E1204726561641A0F0A076F6E636861696E1204726561641A0D0A057065657273120472656164000006206009658194194DF6A601B0E412A992085C25A4E9AAD495FE3669B19BAC693753";
String invoicemacaroon = "0201036C6E640247030A1076E0FAFD1DD4922E080E1FBAFD1675D81201301A160A0761646472657373120472656164120577726974651A170A08696E766F69636573120472656164120577726974650000062078880F0CDBCD350E4156382BC60BB09CA3FDDD803E657D2419F1E38421C4033B";
const char* test_root_ca =   //SSL must be in this format, SSL for the node can be exported from yournode.com:8080 in firefox
     "-----BEGIN CERTIFICATE-----\n" \
    "MIICBTCCAaqgAwIBAgIQBSMZ9g3niBo1jyzK1DvECDAKBggqhkjOPQQDAjAyMR8w\n" \
    "HQYDVQQKExZsbmQgYXV0b2dlbmVyYXRlZCBjZXJ0MQ8wDQYDVQQDEwZSb29tNzcw\n" \
    "HhcNMTPBgNVHRMBAf8EBTADAQH/MHsGA1UdEQR0MHKCBMR8wHQYDVQQKExZsbmQg\n" \
    "YXV0b2dlbmVyYXRlZCBjZXJ0MQ8wDQYDVQQDEwZSb29tNzcwWTATBgcqhkjOPQIB\n" \
    "BggqhkjOPAAAAAAAAAAAAGHBMCoskqHEP6AAAAAAAAA+OWZzLshQoTUeV6FVKbFC\n" \
    "CC+fVGRQsXJx+GVUknnNEcJTt/fQ9CmM6stqGPjAo4GhMIGeMA4GA1UdDwEB/wQE\n" \
    "AwICpDAPBgNVHRMBAf8EBTADAQH/MHsGA1UdEQR0MHKCBlJvb203N4IJbG9jYWxo\n" \
    "b3N0ghVyb29tNzcucmFzcGlibGl0ei5jb22CBHVuaXiCCnVuaXhwYWNrZXSHBH8A\n" \
    "AAGHEAAAAAAAAAAAAAAAAAAAAAGHBMCoskqHEP6AAAAAAAAA+OWZZHfUV0qHBAAA\n" \
    "AAAwCgYIKoZIzj0EAwIDSQAwRgIhALKz3oScii3i+5ltMVQc9u2O38rgfnGCj5Lh\n" \
    "u9iwcAiZAiEA0BjRcisPUlG+SE/s+x6/A2NuT0gtIZ3PKd/GuM5T0jM=\n" \
    "-----END CERTIFICATE-----\n";

String invoiceamount = "100";
bool settle = false;
String payreq = "";
String hash = "";

void setup() {
  M5.begin();
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)BLITZSplash_map);
  
  WiFi.begin(wifiSSID, wifiPASS);   
  while (WiFi.status() != WL_CONNECTED) {
  }
  
  pinMode(26, OUTPUT);
     
     nodecheck();
}

void loop() {

  reqinvoice(invoiceamount);

  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.qrcode(payreq,45,0,240,10);
  delay(100);
  
  gethash(payreq);
  
  checkpayment(hash);
  int counta = 0;
  while (counta < 1000){
    if (settle == false){
      delay(300);
      checkpayment(hash);
      counta++;
    }
    else{     
      M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)BLITZSplash_map);
      digitalWrite(26, HIGH);
      delay(6000);
      digitalWrite(26, LOW);
      delay(1000);
      counta = 1000;
    }  
  }
  nodecheck();
}

////////////////////////// GET/POST REQUESTS///////////////////////////////

void reqinvoice(String value){

   WiFiClientSecure client;

  //client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
      return;   
  }

    
   String topost = "{\"value\": \""+ value +"\", \"memo\": \"Memo" + String(random(1,1000)) +"\", \"expiry\": \"1000\"}";
  
       client.print(String("POST ")+ "https://" + server + ":" + String(lndport) + "/v1/invoices HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "User-Agent: ESP322\r\n" +
                 "Grpc-Metadata-macaroon:" + invoicemacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close\r\n" +
                 "Content-Length: " + topost.length() + "\r\n" +
                 "\r\n" + 
                 topost + "\n");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
       
        break;
      }
    }
    
    String content = client.readStringUntil('\n');
  

    client.stop();
    
    const size_t capacity = JSON_OBJECT_SIZE(3) + 320;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    const char* r_hash = doc["r_hash"];
    hash = r_hash;
    const char* payment_request = doc["payment_request"]; 
    payreq = payment_request;
 
}



void gethash(String xxx){
  
   WiFiClientSecure client;

  //client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
       return;
  }
   

       client.println(String("GET ") + "https://" + server +":"+ String(lndport) + "/v1/payreq/"+ xxx +" HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "Grpc-Metadata-macaroon:" + readmacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close");
                 
       client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
       
        break;
      }
    }
    

    String content = client.readStringUntil('\n');

    client.stop();

    const size_t capacity = JSON_OBJECT_SIZE(7) + 270;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    const char* payment_hash = doc["payment_hash"]; 
    hash = payment_hash;
}


void checkpayment(String xxx){
  
   WiFiClientSecure client;

  //client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
       return;
  }

       client.println(String("GET ") + "https://" + server +":"+ String(lndport) + "/v1/invoice/"+ xxx +" HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "Grpc-Metadata-macaroon:" + readmacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close");
                 
       client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {

        break;
      }
    }
    

    String content = client.readStringUntil('\n');

    client.stop();
    
    const size_t capacity = JSON_OBJECT_SIZE(9) + 460;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    settle = doc["settled"]; 
    
  
}


void page_qrdisplay(String xxx)
{  

  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.qrcode(payreq,45,0,240,10);
  delay(100);

}
void nodecheck(){
  bool checker = false;
  while(!checker){
  WiFiClientSecure client;
   //client.setCACert(test_root_ca);
  if (!client.connect(server, lndport)){

    M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(20, 80);
     M5.Lcd.setTextSize(3);
     M5.Lcd.setTextColor(TFT_RED);
     M5.Lcd.println("NO NODE DETECTED");
     delay(1000);
  }
  else{
    checker = true;
  }
  }
  
}
