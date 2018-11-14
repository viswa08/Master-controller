//#############################################
//# THE MAIN MASTER PROGRAM TO CONTROL ALL GLCD AND DEVICES
//#############################################
//#############################################

// PENDING - CURRENT, KWH, STORAGE OF DATA


//###########    INCLUSIONS ######################

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <SoftwareSerial.h>
#include "ACS712.h"

//###########    DECLARATIONS ######################
#define TankTemp_Pin            2
#define RoomTemp_Pin            3

#define MasterAddress           8
#define GLCDArduinoAddress      18

#define VOLTAGE_AddressPin      A1
#define CURRENT_AddressPin      A2

#define Relay_Pin_Address       4
#define Buzzer_Pin_Address      5

#define CLOCK_ADDRESS 0x68

OneWire oneWire(TankTemp_Pin);
//OneWire oneWire(RoomTemp_Pin);
DallasTemperature TempSensor (&oneWire);
ACS712 CurrentSensor(ACS712_20A, CURRENT_AddressPin);

SoftwareSerial mySerial(6, 7);

//###########    CONSTANTS ######################
      String TextToSend,ReadCloudData;   // the strings to store the temperature data 1 and 2 and other 
      char SData[300];                    // the char to store and write to I2C

      char* Mth[] = {"Month","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};  
      int TrfrVariable;
      String T1,TimeString, CurrentDate;
      float ChillerOffTemp,ChillerOnTemp;
      int CurrentTime;
  
      // Variables for Measuring current//
      double FVariant;
      boolean FirstStart = true;
      

      //------------------------------------------
      // CLOCK SETTINGS SETTINGS
      //------------------------------------------*/
      byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
      int SystemStartTime;
     //------------------------------------------
      // EEPROM MEMORY LOCATIONS USED IN THE PROGRAM
      //------------------------------------------*/
      #define  MEMLOC_FirstTimeRun              0
      #define  MEMLOC_KWH                       10
      #define  MEMLOC_CurrentWatts              15
      #define  MEMLOC_CurrentRunningTime        20
      #define  MEMLOC_TotalNoOfONs              25
      #define  MEMLOC_TotalONTime               30
      #define  MEMLOC_TotalNoOfOFFs             35
      #define  MEMLOC_TotalOFFTime              40

      #define  MEMLOC_AverageONTime             45
      #define  MEMLOC_AverageOFFTime            50

      #define  MEMLOC_ONAndOFFTimeLocations     60  
      #define  MEMLOC_DeviceTempMaxSettings     65
      #define  MEMLOC_DeviceTempMinSettings     68
      #define  MEMLOC_DeviceStartTime           70
      #define  MEMLOC_DeviceStopTime            75
      #define  MEMLOC_DeviceONOrOFF             80

      #define  MEMLOC_StartTime_Location        85
      #define  MEMLOC_StopTime_Location         90

      #define  MEMLOC_Store_StartTime           100
      #define  MEMLOC_Store_StopTime            500
      
      
      //------------------------------------------
      // VARIABLES USED IN THE PROGRAM
      //------------------------------------------*/
      float KWH;                                                    // TOTAL KWH COUNTING
      float CurrentWatts = 0 ;                                      // CURRENT USAGE WATTS
      float CurrentRunningTime = 0;                                 // CURRENT RUNNING TIME
      float TotalNoOfONs;                                           // TOTAL TIMES THE DEVICE HAS TURNED ON
      float TotalONTime;                                            //  TOTAL RUN TIME 
      float TotalNoOfOFFs;                                          // TOTAL NO OF TIMES THE DEVICE IS TURNED OFF
      float TotalOFFTime;                                           // TOTAL TIME THE DEVICE IS TURNED OFF
      float AverageONTime;                                          // AVERAGE ON TIME 
      float AverageOFFTime;                                         // AVERAGE OFF TIME
      int ONTimeIndex=0,OFFTimeIndex=0;                             // THE LOCATION OF DATA AT ON AND OFF
      int MinimumTemp =2400, MaximumTemp=3000;                      //  THE MINIMUM AND MAXIMUM TempS = CAN BE TEMP OR PH
      int CurrentTemp;                                              //  CURRENT Temp - CAN BE TEMP OR PH
      int DeviceStartTemp,DeviceStopTemp;                           //  THE VALUE AT WHICH THE DEVICE TURNS ON AND OFF, VALUE CAN BE PH OR TEMP      
      int DeviceStartTime,DeviceStopTime,DevicePreviousStopTime;    // TIME AT WHICH THE EQUIPMENT STARTS AND ENDS
      int Voltage;                                                  // VOLTAGE STORING AREA
      int Current,Current_Previous;                                 // Current STORING AREA
      float Amp;
      int cloudMin,cloudMax,previousCloudMin=0,previousCloudMax=0;
      boolean DeviceONOrOFF = false ;                               // IS THE DEVICE TURNED ON OR OFF

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////   WATCHDOG TIMER RESET /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//**************************************************
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
      void get_mcusr(void) \
      __attribute__((naked)) \
      __attribute__((section(".init3")));
      
      void get_mcusr(void)
      {
      mcusr_mirror = MCUSR;
      MCUSR = 0;
      // We always need to make sure the WDT is disabled immediately after a 
      // reset, otherwise it will continue to operate with default values.
       
      wdt_disable();
}
//***********************************************/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//###########    SETUP ######################

void setup() {

  Serial.begin(9600);
  mySerial.begin(9600);
  
  Wire.begin(MasterAddress); 
  
  TempSensor.begin();
  CurrentSensor.calibrate();
  
 // ERASE ALL MEMORY STORED  
  //EEPROM.write(MEMLOC_FirstTimeRun,0);
  
  Initialize();
  
 // UNCHECH THE NULLIFY TO RESTART SYSTEM TO ZERO VALUES FROM BEGINNING 
  //NullifyMemory();
  
  GetStoredVariablesFromMemory();  // GET ALL THE STORED VARIABLES FROM MEMORY 
  

  Serial.println("System Restarted");
  
 // SetClock();
 
  
  delay(1000);
/////////////////  SET THE WATCHDOG .......................
      
     wdt_enable(WDTO_8S); 

  
}

//###########    LOOPING ######################

void loop() {
//GET THE DATE AND TIME  

wdt_reset ();

//DateTime now = TheClock.now();

//GET THE DATE AND TIME AND SEND IT 
TextToSend = "DATEA:";

GetClock(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

//Serial.print("Current Time:"); Serial.print(hour); Serial.print(":"); Serial.println(minute);
     
      if (second > 0 && second < 61) {
          CurrentDate = (String) dayOfMonth +"-"+ (String)  Mth[month] + "-"+ (String) (year%100);

////////////////////  SEND THE DATE AND TIME
          if (second%2==0) {
                TextToSend = "DT&TI:";
                if (CurrentDate.length() < 9) { 
                    CurrentDate = "0"+ CurrentDate;   
                  }
                  TextToSend+=CurrentDate;  TextToSend+="," ;  TrfrVariable = hour;
                  
                  TimeString = TrfrVariable;
                  if (TrfrVariable < 10) { TimeString = "0"+ (String) TrfrVariable;}
                  TimeString += ":";
                    TrfrVariable =  minute;  
                    if (TrfrVariable < 10) { TimeString+= "0"+ (String) TrfrVariable;}
                    else {
                      TimeString += (String) TrfrVariable;
                    }
                    TextToSend+= TimeString;
                    TextToSend.toCharArray(SData, 300);
                    SendDataViaWire();
              }

////////////////////  GET THE TEMPERATURE AND SEND IT VIA WIRE
                    if ( second%5 == 0 ) {
                            TempSensor.requestTemperatures(); // Send the command to get temperature readings
                            TextToSend = "TEMPA:";
                            float GTemp = TempSensor.getTempCByIndex(0);
                            CurrentTemp = (int) ((GTemp*100)/100) *100 + (int) (GTemp*100)%100;;
                            TextToSend+= CurrentTemp;
                            TextToSend.toCharArray(SData, 300);
                            if ( CurrentTemp > 10)
                            {                              
                              SendDataViaWire();
                              SendDataToESP8266();
                            }
                            CurrentTemp = CurrentTemp/100*100 + CurrentTemp%100;    // CONVERT THE DECIMALS INTO FULL DIGITS
                        }
                        
////////////////////  TO SEND THE STATUS OF THE DEVICE WHETHER IT IS ON OR OFF BASED ON TEMPERATURE                     
                    if ( second%10 == 0 ) {
                            getMinValue();
                            getMaxValue();
                            if  (CurrentTemp >=  DeviceStartTemp){// || CurrentTemp > DeviceStopTemp) ) {  //IF THE TEMPERATURE IS BETWEEN ST VALUES, OR DEVICE IS ALREADY ON
                              // CURRENT TEMP IS WITHIN DEVICE TURNING ON                              
                              DeviceActivity(true);
                            }
                            
                            if (CurrentTemp <=  DeviceStopTemp )   
                              //IF THE TEMPERATURE IS BETWEEN ST VALUES, OR DEVICE IS ALREADY ON
                             {                              
                              DeviceActivity(false);
                            }
                    }
                    
////////////////////  GET THE VOLTAGE AND SEND IT VIA WIRE 
                        if ( second%15 == 0 ) { 
                               GetVoltage();
                               if (Voltage > 0 ) {
                                   TextToSend = "VOLTS:";
                                   TextToSend += Voltage;
                                   TextToSend.toCharArray(SData, 300);
                                   SendDataViaWire();
                                   SendDataToESP8266();//TempToEsp);
                               }
                         }
////////////////////  GET THE AMPS AND SEND IT VIA WIRE
                          if ( second%20 == 0 ) {
                              Amp = CurrentSensor.getCurrentAC();
                              Serial.println(String("Current As per Sensor :")+Amp+" Amps"); 
                              TextToSend = "AMPS :";
                                  if (DeviceONOrOFF == true) {//SEND THE CURRENT ONLYWHEN THE CHILLER IS ON                                    
                                      TextToSend += Amp;                                  
                                      TextToSend.toCharArray(SData, 300);
                                      SendDataViaWire();
                                      SendDataToESP8266();
                                  }
                                  else { // WHEN THE DEVICE IS TURNED OFF SEND ZERO                                    
                                      TextToSend += 0.0;                                  
                                      TextToSend.toCharArray(SData, 300);
                                      SendDataViaWire();
                                      SendDataToESP8266();
                                  }
                                  
                          }
              
                        if ( second%25 == 0 ) { 
                             TextToSend = "MINT :";
                             //TextToSend += DeviceStopTemp;
                             TextToSend += AverageONTime;
                             TextToSend.toCharArray(SData, 300);
                             SendDataViaWire();
                        }
                 
////////////////////    TO SEND THE SYSTEM MAX TEMP TO THE DISPLAY 
                        if ( second%30 == 0 ) { 
                                TextToSend = "MAXT :";
                            //    TextToSend += DeviceStartTemp;
                                TextToSend += AverageOFFTime;
                                TextToSend.toCharArray(SData, 300);
                                SendDataViaWire();
                        }              
     
////////////////////    TO SEND THE SYSTEM START TIME TO THE DISPLAY 
                        if ( second%35 == 0 ) { 
                               if (FirstStart == true) {
                                    SystemStartTime = hour*100 + minute;
                                    FirstStart = false;
                                }
                                TextToSend = "MSON :";
                                TextToSend += SystemStartTime;
                                TextToSend.toCharArray(SData, 300);
                                SendDataViaWire();
                        }   
              ///////////////////////////////////////////////////////
              // GET AND SEND DATA FROM/TO CLOUD VIA ESP8266
              ///////////////////////////////////////////////////////
               wdt_reset ();
               ReadCloudData="";
               
                while (mySerial.available() > 0) {
                      char inChar = mySerial.read();
                      if (inChar !='\n') {  
                        ReadCloudData += inChar;                        
                        }
                }

               switch((char)ReadCloudData[0]){
                case 'M':
                        T1 = ReadCloudData.substring(8,12);
                        cloudMin = T1.toInt();
                        //Serial.print("cloud min-");Serial.println(cloudMin);
                        if(cloudMin > 0)
                        {
                          storeMinValue(cloudMin);
                          previousCloudMin = cloudMin;
                        }
                        break;

                case 'X':
                        T1 = ReadCloudData.substring(8,12);
                        cloudMax = T1.toInt();
                        //Serial.print("cloud max-");Serial.println(cloudMax);
                        if(cloudMax > 0)
                        {
                          storeMaxValue(cloudMax);
                          previousCloudMax = cloudMax;
                        }
                        break;
               }
                delay(100);
//                Serial.print("Cloud Data :"); Serial.println(ReadCloudData);
                              
                              //SEND DEVICESTOPTEMP TO SERIAL.WRITE
                              //getMinValue();
                              //getMaxValue(); 
                                if (DeviceStopTemp !=0) {  //value read from max eeprom is sent to esp
                                    T1 = "MIN-EEPROM:";
                                    T1+= DeviceStopTemp;
                                    T1.toCharArray(SData,300);
                                    SendDataToESP8266(); //Serial.write(SData);
                                  //  delay(50);
                                }
                              
                                //SEND DEVICESTARTTEMP TO SERIAL.WRITE 
                                if (DeviceStartTemp !=0) {
                                      T1 = "XAM-EEPROM:";
                                      T1+= DeviceStartTemp;
                                      T1.toCharArray(SData,300);
                                      SendDataToESP8266(); //Serial.write(SData);
                                     // delay(50);
                                }
              
      }

}
        

  /////////////// WHEN THE DEVICE TURNS ON OR OFF WHAT TO DO....  ///////////////////////////////

          void DeviceActivity(boolean StatusOfDevice){
          //     DO ALL THE CALCULATIONS AND STORAGE OF VALUES WHEN THE DEVICE HAS STOPPED 
          //     DateTime now = TheClock.now();
          //     GetClock(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
               TextToSend = "ONOFF:";
                if (StatusOfDevice == true && DeviceONOrOFF == false) {
                        EEPROM.write(MEMLOC_DeviceONOrOFF, (byte) "1");   // STORE THE STATUS OF THE DEVICE AS ON
                        DeviceStartTime = (int) hour*100 + (int) minute;
                     //COMPUTE THE TOTAL OFF TIMES AND STORE IT....   
                        TotalNoOfOFFs=TotalNoOfOFFs + 1;
                        if (DeviceStartTime > DevicePreviousStopTime) { // ITS IN THE SAME DAY......
                            TotalOFFTime = TotalOFFTime + DeviceStartTime - DevicePreviousStopTime; // NEED TO INCORPORATE THE OVERNIGHT STUFF
                        }
                        else 
                        {
                            TotalOFFTime = TotalOFFTime + DeviceStartTime - DevicePreviousStopTime + 1440; // NEED TO INCORPORATE THE OVERNIGHT STUFF
                        }

                        
                        AverageOFFTime = TotalOFFTime/TotalNoOfOFFs; // THIS IS IN MINUTES
                        
                        // STORE THE TOTAL OFF TIMES AND NO OF TIMES OFF..... TO CALCULATE AVERAGE....
                        StoreCalculatedValue(TotalNoOfOFFs, MEMLOC_TotalNoOfOFFs);
                        StoreCalculatedValue(TotalOFFTime, MEMLOC_TotalOFFTime);
                        StoreCalculatedValue(AverageOFFTime, MEMLOC_AverageOFFTime);

                        
                    //    StoreCalculatedValue(DeviceStartTime, MEMLOC_DeviceStopTime);
                        //            Serial.print("Total OFF times : "); Serial.print(TotalNoOfOFFs);
                        //            Serial.print("   ,  Total OFF Time : "); Serial.println(TotalOFFTime);
                     /////////////////////////////////////////////////////////////////

////////////////////////////////SENDING THE CHILLER ON TIME TO THE SCREEN
                        TextToSend+= "ON"; DeviceONOrOFF = true; 
                        T1 = "MCON :";
                        T1+=DeviceStartTime;
                        T1.toCharArray(SData, 300);
                        SendDataViaWire();
/////////////////////////////////////////////////////////////////
                      
                        digitalWrite(Relay_Pin_Address,HIGH);
                }
                else if (StatusOfDevice == true ) {
                        TextToSend+= "ON";
                }
                
                else if (StatusOfDevice == false && DeviceONOrOFF == true) {
                        TextToSend+= "OFF";
     //                   EEPROM.write(MEMLOC_DeviceONOrOFF, (byte) "0");   // STORE THE STATUS OF THE DEVICE AS ON
                        DeviceStopTime = (int) hour*100 + (int) minute;
                        //            Serial.print("Device Stopped at : "); Serial.print(DeviceStopTime);
                        if (DeviceStopTime > DeviceStartTime) // INCORPORATE IF IT TURNS OFF OVER NIGHT.... NEED TO TAKE INTO CONSIDERATION
                        {
                                CurrentRunningTime = DeviceStopTime - DeviceStartTime;  
                        }
                        else {
                                CurrentRunningTime = DeviceStopTime - DeviceStartTime + 1440 ; // INCORPORATE THE MIDNIGHT TRANSFER OF TIME 
                        }
                                                   
                          //STORE THE NUMBER OF TIMES THE DEVICE HAS TURNED ON
                                  TotalNoOfONs = TotalNoOfONs + 1 ;//+ GetCalculatedValue(MEMLOC_TotalNoOfONs);//
                                  StoreCalculatedValue(TotalNoOfONs, MEMLOC_TotalNoOfONs);
                          //STORE THE NUMBER OF MINUTES THE DEVICE HAS TURNED ON
                                  TotalONTime = TotalONTime + CurrentRunningTime; // + GetCalculatedValue(MEMLOC_TotalONTime);
                                  StoreCalculatedValue(TotalONTime, MEMLOC_TotalONTime);
                          //STORE THE AVERAGE MINUTES THE DEVICE HAS TURNED ON
                                  AverageONTime = TotalONTime / TotalNoOfONs;
                                  StoreCalculatedValue(AverageONTime, MEMLOC_AverageONTime);
                                           
                          //STORE THE DEVICE STOP TIME TO RECALL IT NEXT TIME TO CALCULATE TOTAL STOPPED TIME.....
                                  DevicePreviousStopTime = DeviceStopTime;
                          //        Serial.print(" Device New Pr Stop Time  : "); Serial.println(DevicePreviousStopTime);

                          // CALCULATE THE CURRENT WATTS CONSUMED
                                  Serial.println("kwh calculation started");
                                  CurrentWatts = CurrentRunningTime/60*Voltage*Amp; // MINUTES * V * I;
                                  KWH+= CurrentWatts/1000;
                                  Serial.print("current run time");Serial.println(KWH);
                                  StoreCalculatedValue(KWH, MEMLOC_KWH);
 
                                  EEPROM.write(MEMLOC_DeviceStopTime,(byte) (DevicePreviousStopTime/100)) ; delay(25);
                                  EEPROM.write(MEMLOC_DeviceStopTime+1,(byte) (DevicePreviousStopTime%100)) ;                                   
/*                                   
                                   Serial.print(" Stored Time :100   : "); Serial.println((int) EEPROM.read(MEMLOC_DeviceStopTime));
                                    Serial.print(" Stored Time : 10   : "); Serial.println((int) EEPROM.read(MEMLOC_DeviceStopTime+1));
                                  DevicePreviousStopTime = (int) EEPROM.read(MEMLOC_DeviceStopTime)*100 + (int) EEPROM.read(MEMLOC_DeviceStopTime+1);   //THE VALUE AT WHICH THE DEVICE TURNS ONN
                                  Serial.print(" Stored Pr Stop Time  : "); Serial.println(DevicePreviousStopTime);
*////
////////////////////////////////SENDING THE CHILLER STOP TO THE SCREEN
                                  T1 = "MCOF :";
                                  T1+=DeviceStopTime;
                                  T1.toCharArray(SData, 300);
                                  SendDataViaWire();
                      
////////////////////////////////SENDING THE CHILLER KWH TO THE SCREEN
                                  T1 = "KWH :";
                                  T1+=KWH;
                                  T1.toCharArray(SData, 300);
                                  SendDataViaWire();
                                  SendDataToESP8266();
                                  DeviceONOrOFF = false;
                                  DeviceStartTime =0 ;
                                  digitalWrite(Relay_Pin_Address,LOW);    // TURN THE RELAY PIN OFF
          //                }
                  }
                  else if (StatusOfDevice == false ) {
                        TextToSend+= "OFF";
                  }

                  TextToSend.toCharArray(SData, 300);
                  SendDataViaWire();
                  SendDataToESP8266();

          }


//******************************************************************************************************************************************
//                                                      FUNCTIONS USED IN MAIN PROGRAMS
//******************************************************************************************************************************************

//###########    FUNCTIONS ######################
        
/////////////// SEND THE DATA VIA WIRE ///////////////////////////////
        
        void SendDataViaWire(){
                  Wire.beginTransmission(GLCDArduinoAddress); // transmit to device #1976
                  Wire.write(SData);
                  Wire.endTransmission();
                  delay(250);// stop transmitting
   
     }

        void SendDataToESP8266(){
                  mySerial.write(SData);
                  delay(100);
     //             Serial.print("Sending Data to ESP :" ) ; Serial.println(SData);
        }
        
/////////////// GET THE DATA STORED IN EEPROM INTO VARIABLES ///////////////////////////////
    
        void GetStoredVariablesFromMemory()
        {
            KWH = GetCalculatedValue(MEMLOC_KWH);                             // TOTAL KWH COUNTING
            TotalNoOfONs  = GetCalculatedValue(MEMLOC_TotalNoOfONs);               // TOTAL TIMES THE DEVICE HAS TURNED ON
            TotalONTime  = GetCalculatedValue(MEMLOC_TotalONTime);                // TOTAL TIME THE DEVICE HAS TURNED ON
            TotalNoOfOFFs  = GetCalculatedValue(MEMLOC_TotalNoOfOFFs);             // TOTAL TIMES THE DEVICE HAS TURNED OFF
            TotalOFFTime  = GetCalculatedValue(MEMLOC_TotalOFFTime);              // TOTAL TIME THE DEVICE HAS TURNED OFF
            AverageONTime  = GetCalculatedValue(MEMLOC_AverageONTime);             // TOTAL TIMES THE DEVICE HAS TURNED OFF
            AverageOFFTime  = GetCalculatedValue(MEMLOC_AverageOFFTime);              // TOTAL TIME THE DEVICE HAS TURNED OFF
            
    //        AverageONTime = TotalONTime/TotalNoOfOFFs;
    //        AverageOFFTime = TotalOFFTime / TotalNoOfOFFs;

            Serial.println(String(" No of ON times :")+TotalNoOfONs);
            Serial.println(String(" ON Time :")+TotalONTime);
            
            Serial.println(String(" No Of OFF Time :")+TotalNoOfOFFs);
            Serial.println(String(" OFF Time :")+TotalOFFTime);
            
            CurrentWatts  = 0;                                                    // CURRENT WATTS CONSUMED
            CurrentRunningTime  = 0;                                              // CURRENT RUNNING TIME

            DeviceStartTemp = (int) EEPROM.read(MEMLOC_DeviceTempMaxSettings)*100 + (int) EEPROM.read(MEMLOC_DeviceTempMaxSettings+1);   //THE VALUE AT WHICH THE DEVICE TURNS ONN
            DeviceStopTemp= (int) EEPROM.read(MEMLOC_DeviceTempMinSettings)*100 + (int) EEPROM.read(MEMLOC_DeviceTempMinSettings+1);   //THE VALUE AT WHICH THE DEVICE TURNS OFF

            //if (DeviceStopTemp < MinimumTemp) { DeviceStopTemp = MinimumTemp ;}
            //if (DeviceStartTemp > MaximumTemp || DeviceStartTemp < MinimumTemp) { DeviceStartTemp = MaximumTemp ;}
            Serial.print("Stop temperature :");Serial.println(DeviceStopTemp);
            Serial.print("Start temperature :");Serial.println(DeviceStartTemp);
                   
         }
/////////////// GET THE VARIOUS EEPROM AND GET THEM INTO THE FLOAT ///////////////////////////////
            
          float GetCalculatedValue(int MemoryLocation) {
            float CalculatedValue;
                CalculatedValue = (int) EEPROM.read(MemoryLocation) *10000 + (int) EEPROM.read(MemoryLocation+1)*100 + (int) EEPROM.read(MemoryLocation+2) +(int) EEPROM.read(MemoryLocation+3)/100;  
                delay(25);
            return CalculatedValue;
          }

/////////////// GET VARIOUS EEPROM AND GET THEM INTO THE FLOAT ///////////////////////////////

          void StoreCalculatedValue(float TheValue, int MemoryLocation) {
                int Lacs,Thousands,Hundreds,Decimals;
                    Lacs = (int) TheValue/10000;
                    Thousands = (int) (TheValue - Lacs)/100;
                    Hundreds = (int) (TheValue-Lacs-Thousands);
                    Decimals = (TheValue - (int) (TheValue))*100;
                    EEPROM.write(MemoryLocation, (byte) Lacs); delay(25);
                    EEPROM.write(MemoryLocation+1, (byte) Thousands); delay(25);
                    EEPROM.write(MemoryLocation+2, (byte) Hundreds); delay(25);
                    EEPROM.write(MemoryLocation+3, (byte) Decimals); delay(25);
          }

/////////////////  GET THE CURRENT AVERAGE VOLTAGE ///////////////////////////////
            
          float GetVoltage()
            { 
              int VValue;
              VValue = 0;
              for (int i=0;i<50;i++) {
                VValue+= analogRead(VOLTAGE_AddressPin);
                delay(50);
            }
                Voltage = map(VValue/50,0,350,0,260);
                if (Voltage < 0) { Voltage =0;} //
          }
          
/////////////////  GETTING CURRENT VALUE ///////////////////////////////////////////////////  
    
          float GetCurrent(){
            float result;
            int ReadValue;
            int MaxValue = 0;
            int MinValue = 1024;

            uint32_t start_time = millis();
            while ((millis()-start_time) < 1000)
                {
                  ReadValue = analogRead(CURRENT_AddressPin);
                  if (ReadValue > MaxValue)
                  {
                    MaxValue = ReadValue;
                  }
                  if (ReadValue < MaxValue)
                  {
                    MinValue = ReadValue;
                  }
                 
                }

              result = ((MaxValue - MinValue)*5.0)/1024.0;

              return result;
          }

            
/////////////////  STORE THE DATA STORED IN EEPROM FROM THE FLOAT MAX FORMAT 9,99,999.99 ///////////////////////////////
                 
         void StoreVariablesOntoMemory(){
                StoreCalculatedValue(KWH,MEMLOC_KWH);
                StoreCalculatedValue(CurrentWatts,MEMLOC_CurrentWatts);
                StoreCalculatedValue(CurrentRunningTime,MEMLOC_CurrentRunningTime);
                StoreCalculatedValue(TotalNoOfONs,MEMLOC_TotalNoOfONs);
                StoreCalculatedValue(TotalONTime,MEMLOC_TotalONTime);
                StoreCalculatedValue(TotalNoOfOFFs,MEMLOC_TotalNoOfOFFs);
                StoreCalculatedValue(TotalOFFTime,MEMLOC_TotalOFFTime);
                
                StoreCalculatedValue(AverageONTime,MEMLOC_AverageONTime);
                StoreCalculatedValue(AverageOFFTime,MEMLOC_AverageOFFTime);
                
                  
                EEPROM.write(MEMLOC_CurrentRunningTime, (byte) CurrentRunningTime/100);
                EEPROM.write(MEMLOC_CurrentRunningTime+1, (byte) CurrentRunningTime%100);
                //EEPROM.write(MEMLOC_DeviceTempSettings,(byte) DeviceStartTemp/100) ;
                //EEPROM.write(MEMLOC_DeviceTempSettings+1,(byte) DeviceStartTemp%100) ;
                //EEPROM.write(MEMLOC_DeviceTempSettings+2,(byte) DeviceStopTemp/100) ;
                //EEPROM.write(MEMLOC_DeviceTempSettings+3,(byte) DeviceStopTemp%100) ;
                    
         }

         void storeMinValue(int Min){
                //Serial.print("min");Serial.println(Min);
                EEPROM.put(MEMLOC_DeviceTempMinSettings, Min/100) ;
                EEPROM.put(MEMLOC_DeviceTempMinSettings+1, Min%100) ;
                
         }

         void storeMaxValue(int Max){
                EEPROM.put(MEMLOC_DeviceTempMaxSettings, Max/100) ;
                EEPROM.put(MEMLOC_DeviceTempMaxSettings+1, Max%100) ;
         }

         void getMinValue(){
          DeviceStopTemp= (int) EEPROM.read(MEMLOC_DeviceTempMinSettings)*100 + (int) EEPROM.read(MEMLOC_DeviceTempMinSettings+1);   //THE VALUE AT WHICH THE DEVICE TURNS OFF          
          Serial.print("ee min-");Serial.println(DeviceStopTemp);
         }

         void getMaxValue(){
          DeviceStartTemp = (int) EEPROM.read(MEMLOC_DeviceTempMaxSettings)*100 + (int) EEPROM.read(MEMLOC_DeviceTempMaxSettings+1);   //THE VALUE AT WHICH THE DEVICE TURNS ONN
          Serial.print("ee max-");Serial.println(DeviceStartTemp);
         }

         

         

                
         void NullifyMemory(){
                StoreCalculatedValue(0,MEMLOC_KWH);
                StoreCalculatedValue(0,MEMLOC_CurrentWatts);
                StoreCalculatedValue(0,MEMLOC_CurrentRunningTime);
                StoreCalculatedValue(0,MEMLOC_TotalNoOfONs);
                StoreCalculatedValue(0,MEMLOC_TotalONTime);
                StoreCalculatedValue(0,MEMLOC_TotalNoOfOFFs);
                StoreCalculatedValue(0,MEMLOC_TotalOFFTime);
                StoreCalculatedValue(0,MEMLOC_AverageONTime);
                StoreCalculatedValue(0,MEMLOC_AverageOFFTime);
       
                StoreCalculatedValue(0,MEMLOC_DeviceStartTime);
                StoreCalculatedValue(0,MEMLOC_DeviceStopTime);

                
         }
    

  /////////////// INITIALIZE THE FIRST FEW SETTINGS ///////////////////////////////
     
          void Initialize() {
                pinMode(Relay_Pin_Address,OUTPUT);
                pinMode(Buzzer_Pin_Address,OUTPUT);

                int MinVal = 0, i ;
                DeviceONOrOFF = false; 

                DeviceStartTime = (int) EEPROM.read(MEMLOC_DeviceStartTime)*100 + (int) EEPROM.read(MEMLOC_DeviceStartTime+1);   //THE VALUE AT WHICH THE DEVICE TURNS ONN
                DevicePreviousStopTime = (int) EEPROM.read(MEMLOC_DeviceStopTime)*100 + (int) EEPROM.read(MEMLOC_DeviceStopTime+1);   //THE VALUE AT WHICH THE DEVICE TURNS ONN
                              
                if ( DevicePreviousStopTime == 0) { 
                  DevicePreviousStopTime = (int) hour*100 + (int) minute ;
                }
                                  
              // IF THERE IS NO DATA ON STOP TIME, MAKE THE CURRENT TIME AS STOP TIME.....
                 
        /// RESETTING  ALL DATA STORAGED VALUES FOR THE FIRST TIME THE SYSTEM IS USED
                i = (int) EEPROM.read(MEMLOC_FirstTimeRun); 
                if (i != 234) { // PROGRAM MEMOERY HAS BEEN ERASED ONCE
                  for (int k = 0;k<EEPROM.length();k++){
                    EEPROM.write(k,0);
                  }
                  EEPROM.write(MEMLOC_FirstTimeRun,234);
                }

          }

          

          
//******************************************************************************************************************************************
//                                                      FUNCTIONS USED IN CLOCK READ AND WRITE FUNCTIONS
//******************************************************************************************************************************************

///////////////////////---CONNVERTS THE NORMAL DECIMAL NUMBERS TO BINARY CODED DECIMALS//**********************************************//
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

///////////////////////---CONVERTS BINARY CODED DECIMALS TO NORMAL DECIMAL NUMBERS//**********************************************//
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

///////////////////////---GETS THE DATE AND TIME FROM DS1307----/////////////////////////
void GetClock(byte *second,
          byte *minute,
          byte *hour,
          byte *dayOfWeek,
          byte *dayOfMonth,
          byte *month,
          byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(CLOCK_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}

///////////////////////---SETS THE DATE AND TIME OF THE CLOCK DS1307 ----/////////////////////////
void SetClock()          // 0-99
{
   Wire.beginTransmission(CLOCK_ADDRESS);
   Wire.write(0);
   Wire.write(decToBcd(0));    // 0 to bit 7 starts the clock
   Wire.write(decToBcd(53));      //  minutes
   Wire.write(decToBcd(22));      //  hours            // If you want 12 hour am/pm you need to set
                                  //  bit 6 (also need to change readDateDs1307)
   Wire.write(decToBcd(00));      //  seconds
   Wire.write(decToBcd(9));      //  day
   Wire.write(decToBcd(9));       //  month
   Wire.write(decToBcd(18));      //  year
   Wire.endTransmission();
}


void requestEvent() {
  
//  Serial.println("Request for Data ");
          
  T1 = "ESP:2334,2000,3000";
  T1.toCharArray(SData,300);
  //   Wire.beginTransmission(ESPAddress); // transmit to device #1976
      Wire.write("There We Go");
  //    Wire.endTransmission();    // stop transmitting
      delay(75);
}
