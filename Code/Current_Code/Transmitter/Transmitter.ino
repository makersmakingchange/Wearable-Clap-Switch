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

#include <bluefruit.h>//bluetooth library
#include <Adafruit_CircuitPlayground.h>
#include <Wire.h>
#include <SPI.h>

BLEClientBas  clientBas;  // battery client
BLEClientDis  clientDis;  // device information client
BLEClientUart clientUart; // bleuart client


#define NEOPIX_PIN    A2
#define NUM_PIXELS    5
#define PEAK_DEBOUNCE_MS     500
#define BUTTON_DEBOUNCE_MS   20
Adafruit_CPlay_NeoPixel strip = Adafruit_CPlay_NeoPixel(NUM_PIXELS, NEOPIX_PIN, NEO_GRB + NEO_KHZ800);


int CLICKTHRESHHOLD = 70;
int MINTHRESHOLD =10;
int MAXTHRESHOLD= 110;

bool tap =false;

int calculateLevel(){
  int level = (CLICKTHRESHHOLD*10)/(MAXTHRESHOLD-MINTHRESHOLD);//10 was chosen to get from 0.00-1.00 percent to 00.0 to 10.0 percentage, since there are 10 lights
  if(level>9){
    level=9;
  }
  return level;
}


void tapTime(void) {
  tap=true;
      Serial.print("Tap! ");
   Serial.println(millis()); // the time
}

void changeSensitivity(){
  CircuitPlayground.setAccelTap(1, CLICKTHRESHHOLD);
//  for(int e = 0; e<=calculateLevel();e++){
//    CircuitPlayground.setPixelColor(e, 50, 235, 200);
//  }
  for(int e = 9; e>=calculateLevel();e--){
    CircuitPlayground.setPixelColor(e, 50, 235, 200);
  }
  strip.show();
  delay(500);
  for(int e = 9; e>=calculateLevel();e--){
    CircuitPlayground.setPixelColor(e, 0, 0, 0);
  }
  strip.show();
  }
void tapRecieved(){
 
    if ( Bluefruit.Central.connected() )//if connected
  {
    if ( clientUart.discovered() )//and the device has UART
    {
      Serial.print("Tap! ");
   Serial.println(millis()); // the time
   for(int e = 0;e<=9;e++){
    Serial.println(e);
    CircuitPlayground.setPixelColor(e, 50, 235, 200);
    
   }
   
        char str =  '!' ;
        clientUart.print(str, 1);//write a single character to the reciever
        Serial.println("write to toy controller");
   //Serial.println("Lights on");
  delay(100);
  CircuitPlayground.clearPixels();
  //Serial.println("Lights off");
        delay(1000);//wait
      
      delay(200);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  
  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 1);//start bluetooth
  
  while(!CircuitPlayground.begin());
  
  CircuitPlayground.setAccelRange(LIS3DH_RANGE_8_G);   // 2, 4, 8 or 16 G!
  

  attachInterrupt(digitalPinToInterrupt(CPLAY_LIS3DH_INTERRUPT), tapTime, FALLING);
  strip.begin();
  Bluefruit.setName("Bluefruit52 Central");//set name

  // Configure Battyer client
  clientBas.begin();//start battery level service

  // Configure DIS client
  clientDis.begin();//start device information service

  // Init BLE Central Uart Serivce
  clientUart.begin();//start UART service
  clientUart.setRxCallback(bleuart_rx_callback);//set data recieve callback

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(250); //change led blink interval

  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(connect_callback);//set connect callback
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);//set disconnect callback

  /* Start Central Scanning
     - Enable auto scan if disconnected
     - Interval = 100 ms, window = 80 ms
     - Don't use active scan
     - Start(timeout) with timeout = 0 will scan forever (until connected)
  */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);//start scanning again after disconnect
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);                   // // 0 = Don't stop scanning after n seconds
}

/**
   Callback invoked when scanner pick up an advertising data
   @param report Structural advertising data
*/
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  // Check if advertising contain BleUart service
  if ( Bluefruit.Scanner.checkReportForService(report, clientUart) )
  {
    Serial.print("BLE UART service detected. Connecting ... ");

    // Connect to device with bleuart service in advertising
    Bluefruit.Central.connect(report);
  } else
  {
    // For Softdevice v6: after received a report, scanner will be paused
    // We need to call Scanner resume() to continue scanning
    Bluefruit.Scanner.resume();
  }
}

void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");

  Serial.print("Dicovering Device Information ... ");
  if ( clientDis.discover(conn_handle) )
  {
    Serial.println("Found it");
    char buffer[32 + 1];

    // read and print out Manufacturer
    memset(buffer, 0, sizeof(buffer));
    if ( clientDis.getManufacturer(buffer, sizeof(buffer)) )
    {
      Serial.print("Manufacturer: ");
      Serial.println(buffer);
    }

    // read and print out Model Number
    memset(buffer, 0, sizeof(buffer));
    if ( clientDis.getModel(buffer, sizeof(buffer)) )
    {
      Serial.print("Model: ");
      Serial.println(buffer);
    }

    Serial.println();
  } else
  {
    Serial.println("Found NONE");
  }

  Serial.print("Dicovering Battery ... ");
  if ( clientBas.discover(conn_handle) )
  {
    Serial.println("Found it");
    Serial.print("Battery level: ");
    Serial.print(clientBas.read());
    Serial.println("%");
  } else
  {
    Serial.println("Found NONE");
  }

  Serial.print("Discovering BLE Uart Service ... ");
  if ( clientUart.discover(conn_handle) )
  {
    Serial.println("Found it");

    Serial.println("Enable TXD's notify");
    clientUart.enableTXD();

    Serial.println("Ready to receive from peripheral");
  } else
  {
    Serial.println("Found NONE");

    // disconnect since we couldn't find bleuart service
    Bluefruit.disconnect(conn_handle);
  }
}

/**
   Callback invoked when a connection is dropped
   @param conn_handle
   @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

void bleuart_rx_callback(BLEClientUart& uart_svc)
{//we never write expect to recieve anything from the peripheral unit
}

void loop()
{
if(tap){
    tapRecieved();
    tap=false;
  }
  if(CircuitPlayground.leftButton()){
    CLICKTHRESHHOLD-=10;;
    changeSensitivity();
    while(CircuitPlayground.leftButton());
    }
  if(CircuitPlayground.rightButton()){
    CLICKTHRESHHOLD+=10;
    changeSensitivity();
    while(CircuitPlayground.rightButton());
    }
  delay(100);
}
