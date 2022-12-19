/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#define DELAY 15
#define DURATION 16
#define RELAY 9
// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

int maxDelay = 5000; //milliseconds
int minDelay = 500; //milliseconds
int maxDuration= 5000; //milliseconds
int minDuration = 500; //milliseconds
int maxReading = 1023;//maximum possible reading 

void setup()
{
  Serial.begin(115200);//start serial
  pinMode(RELAY, OUTPUT);//setup realy trigger pin
  pinMode(DURATION,INPUT);//setup duration potentiometer pin
  pinMode(DELAY,INPUT);//setup delay potentiometer pin
  digitalWrite(RELAY,LOW);//ensure that relay starts open/switch is not active

  //COnfigure and Start Bluefruit
  Bluefruit.autoConnLed(true);// Setup the BLE LED to be enabled on CONNECT
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);// Config the peripheral connection with maximum bandwidth 
  Bluefruit.begin();// start bluefruit
  Bluefruit.setTxPower(4);// Check bluefruit.h for supported values
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
  bledfu.begin();
  
  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();

}

int getDelay(){
  int delayReading = analogRead(DELAY);
  Serial.println(delayReading);
  return map(delayReading, 0, maxReading, minDelay, maxDelay);//scale the delay to fit into the maxDelay range map (value, fromLow, fromHigh,toLow, toHigh)
}

int getDuration(){
  int durationReading = analogRead(DURATION);
  Serial.println(durationReading);
  return map(durationReading, 0, maxReading, minDuration, maxDuration);//scale the duration to fit into the maxDuration range map (value, fromLow, fromHigh,toLow, toHigh)
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void loop()
{
  // Forward from BLEUART to HW Serial
  while ( bleuart.available() )
  {
    //read character
    uint8_t ch;
    ch = (uint8_t) bleuart.read();
    //get delay and duration from input potentiometers
    int delayTime = getDelay();
    int durationTime =  getDuration();
    //display input, delay time, and duration time for debugging
    //Serial.write(ch);
    Serial.println(delayTime);
    Serial.println(durationTime);
    digitalWrite(RELAY,HIGH);//activate the switch
    delay(durationTime);//wait for the duration
    digitalWrite(RELAY,LOW);//deactivate the switch
    delay(delayTime);//wait for the delay
    //clear rest of buffer to prevent double triggering
    while ( bleuart.available() )
      {ch = (uint8_t) bleuart.read();
      }
      
    }
  }


// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}
