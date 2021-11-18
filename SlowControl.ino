#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <BME280I2C.h>
#include <EnvironmentCalculations.h>
#include <ArxContainer.h>
#include <EEPROM.h>

const String version{"Version 1.0  <a href='https://github.com/RPClab/Slowcontrol'></a>"};
const String image{"<img src='https://raw.githubusercontent.com/flagarde/YAODAQ/main/docs/imgs/icon.png' alt='Link' style='width:80px;height:80px;'>"};

////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Options///////////////////////////////////
////////////////////////////////////////////////////////////////////////////
const int valve{7};     //use digital I/O pin 7
// Enter a MAC address and IP address anf port for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xEE };
IPAddress ip_server(192,168,0,179);
int port{80};
// Where to send the values
int    HTTP_PORT{80};
String HTTP_METHOD{"GET"};
char   HOST_NAME[]{"192.168.0.200"}; // change to your PC's IP address
String PATH_NAME{"/SlowControlFiller.php"};
//BME options
BME280I2C::Settings settings_gas(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
);

BME280I2C::Settings settings_atmospheric(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x77 // I2C address. I2C specific.
);

//VEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRRRRRRRRYYYYYYYYYYYYYYYYYYYYY IMPORTANT
// This will be the sensor_id with sensor_id = BoardNumber*100+I2C port
// Start with 1 for better check !!!!!
int BoardNumber{3};

// I2C port
int gas_number{76};
int atmospheric_number{77};
// Sensor name
String gasName{"Mixer "+String(BoardNumber)+" gas"};
String atmosphericName{"Mixer "+String(BoardNumber)+" atmosphere"};

// Default value for webpage
int number_measurements{15};
int mdelay{20000}; //ms between data taking (20s by default)
float defaultHumidity{40.};

//Title of the webpage
String title{"Mixer "+String(BoardNumber)};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


#define MaxHeaderLength 512
String HttpHeader = String(MaxHeaderLength);
BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_hPa);
EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;

bool ValseIsOn{false};

class Data
{
public:
  Data(){}
  Data(const float T,const float P,const float H) : m_T(T), m_P(P), m_H(H) {}
  float getT() const { return m_T; }
  float getP() const { return m_P; }
  float getH() const { return m_H; }
private:
  float m_T{0.};
  float m_P{0.};
  float m_H{0.};
};

class MeanRMS
{
public:
  MeanRMS(){}
  MeanRMS(const arx::vector<Data>& data)
  {
    CalculateMeans(data);
    CalculateRMS(data);
  }
  void CalculateMeans(const arx::vector<Data>& data)
  {
    for(size_t i=0;i!=data.size();++i)
    {
      mean_temperature+=data[i].getT();
      mean_pressure+=data[i].getP();
      mean_humidity+=data[i].getH();
    }
    mean_temperature/=data.size();
    mean_pressure/=data.size();
    mean_humidity/=data.size();
  }
  void CalculateRMS(const arx::vector<Data>& data)
  {
    for(size_t i=0;i!=data.size();++i)
    {
      rms_temperature+=(data[i].getT()-mean_temperature)*(data[i].getT()-mean_temperature);
      rms_pressure+=(data[i].getP()-mean_pressure)*(data[i].getP()-mean_pressure);
      rms_humidity+=(data[i].getH()-mean_humidity)*(data[i].getH()-mean_humidity);
    }
    rms_temperature=sqrt(rms_temperature/(data.size()-1));
    rms_pressure=sqrt(rms_pressure/(data.size()-1));
    rms_humidity=sqrt(rms_humidity/(data.size()-1));
  }
  float meanT() const { return mean_temperature; }
  float meanP() const { return mean_pressure; }
  float meanH() const { return mean_humidity; }
  float RMST() const { return rms_temperature; }
  float RMSP() const { return rms_pressure; }
  float RMSH() const { return rms_humidity; }
private:
  float mean_temperature{0.};
  float mean_pressure{0.};
  float mean_humidity{0.};
  float rms_temperature{0.};
  float rms_pressure{0.};
  float rms_humidity{0.};
};


EthernetServer server(port);
EthernetClient client;
EthernetClient client2;

bool gas_connected{false};
bool atmospheric_connected{false};

BME280I2C gas;
BME280I2C atmospheric;

Data BME280Data(Stream* client,BME280I2C& sensor)
{
   float temp(0.);
   float hum(0.);
   float pres(0.);
   sensor.read(pres, temp, hum, tempUnit, presUnit);
   /*float dewPoint = EnvironmentCalculations::DewPoint(temp, hum, envTempUnit);
   float absHum = EnvironmentCalculations::AbsoluteHumidity(temp, hum, envTempUnit);
   client->print("\nTemp: ");
   client->print(temp);
   client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
   client->print("\nHumidity: ");
   client->print(hum);
   client->print("% RH");
   client->print("\nPressure: ");
   client->print(pres);
   client->print("hPa");
   client->print("\nDew point: ");
   client->print(dewPoint);
   client->print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   client->print("\nAbsolute Humidity: ");
   client->print(absHum);
   client->print("\nHeat Index: ");
   float heatIndex = EnvironmentCalculations::HeatIndex(temp, hum, envTempUnit);
   client->print(heatIndex);
   client->print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   client->print("\n");*/
   return Data(temp,pres,hum);
}


void checkHumidity(const float& humidity, const float& default_humidity = defaultHumidity)
{
  if(defaultHumidity < humidity)
  { 
    if(ValseIsOn==true)
    {
      digitalWrite(valve,LOW);
      Serial.println("TURNING VALSE OFF");
      ValseIsOn = false;
    } 
  }
  else
  {
    
    if(ValseIsOn==false)
    {
      digitalWrite(valve,HIGH);
      ValseIsOn = true;
      Serial.println("TURNING VALSE ON");
    }
  }
}

arx::vector<Data> gas_data_old;
arx::vector<Data> atmospheric_data_old;
MeanRMS meanRMSgas_old;
MeanRMS meanRMSatmospheric_old;

void setup()
{
  pinMode(valve,OUTPUT);   //set pin 7 to be an output output
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while(!Serial) {;}

  Wire.begin();
  //Setup BME280s
  gas.setSettings(settings_gas);
  atmospheric.setSettings(settings_atmospheric);
  if(gas.begin()) gas_connected = true;
  if(atmospheric.begin()) atmospheric_connected =true;
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip_server);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  if(client.connect(HOST_NAME, HTTP_PORT)) Serial.println("connected 1 to NAS");
  if(client2.connect(HOST_NAME, HTTP_PORT)) Serial.println("connected 2 to NAS");

  //One round to print value when starting
  for(size_t i=0; i!=number_measurements; ++i)
  { 
    if(gas_connected)
    {
      Data dat = BME280Data(&Serial,gas);
      gas_data_old.push_back(dat);
    }
    if(atmospheric_connected)
    {
      Data dat = BME280Data(&Serial,atmospheric);
      atmospheric_data_old.push_back(dat);
    }
    delay(200);
  }
  meanRMSgas_old= MeanRMS(gas_data_old);
  meanRMSatmospheric_old= MeanRMS(atmospheric_data_old);
  
}

void printTable(EthernetClient& client,const String& title,const arx::vector<Data>& data,const MeanRMS& meanRMS)
{
  client.println("<div class='table-wrapper'>");
  client.println("<table border='1' style='float: left;margin-right:10px' class='fl-table'>");
  client.println("<caption>"+String(title)+"</caption>");
  client.println("<thead>");
  client.println("<tr>");
  
  client.println("<th align='center' valign='center'>");
  client.println("Temperature (&#x2103)");
  client.println("</th>");

  client.println("<th align='center' valign='center'>");
  client.println("Pressure (hPa)");
  client.println("</th>");

  client.println("<th align='center' valign='center'>");
  client.println("Relative Humidity (%)");
  client.println("</th>");
  
  client.println("</tr>");
  client.println("</thead>");
  client.println("<tbody>");
  for(size_t i=0;i!=data.size();++i)
  {
    client.println("<tr>");
    client.println("<td align='center' valign='center'>");
    client.println(data[i].getT());
    client.println("</td>");
    client.println("<td align='center' valign='center'>");
    client.println(data[i].getP());
    client.println("</td>");
    client.println("<td align='center' valign='center'>");
    client.println(data[i].getH());
    client.println("</td>");
    client.println("</tr>");
  }
  client.println("</tbody>");

  client.println("<tfoot>");
  client.println("<tr>");
  client.println("<th align='center' valign='center'>");
  client.println("Mean temperature (&#x2103)");
  client.println("</th>");

  client.println("<th align='center' valign='center'>");
  client.println("Mean pressure (hPa)");
  client.println("</th>");

  client.println("<th align='center' valign='center'>");
  client.println("Mean relative Humidity (%)");
  client.println("</th>");
  client.println("<tr>");


    client.println("<tr>");
    client.println("<td align='center' valign='center'>");
    client.println(String(meanRMS.meanT())+"&plusmn"+String(meanRMS.RMST()));
    client.println("</td>");
    client.println("<td align='center' valign='center'>");
    client.println(String(meanRMS.meanP())+"&plusmn"+String(meanRMS.RMSP()));
    client.println("</td>");
    client.println("<td align='center' valign='center'>");
    client.println(String(meanRMS.meanH())+"&plusmn"+String(meanRMS.RMSH()));
    client.println("</td>");
    client.println("</tr>");
  
  client.println("</tfoot>");


  
  client.println("</table>");
  client.println("</div>");
}

String returnValue(const String& header,const String key)
{
  size_t Begin = header.indexOf(key);
  if(Begin==-1) return "";
  else
  {
    //Begin +length of the key +1(for =)
    size_t value_begin = Begin+key.length()+1;
    size_t value_end = header.indexOf("&",value_begin);
    if(value_begin==-1) return header.substring(value_begin);
    else return header.substring(value_begin,value_end);
  }
}

arx::vector<Data> gas_data;
arx::vector<Data> atmospheric_data;

unsigned long myTime1{millis()};
unsigned long myTime2{0};


void loop()
{
  //CHECK GAS HUMIDITY AND OPEN/CLOSE VALVE
  Data dat = BME280Data(&Serial,gas);
  checkHumidity(dat.getH());
  
  if(gas_data.size()==number_measurements || atmospheric_data.size()==number_measurements)
  {
    MeanRMS meanRMSgas(gas_data);
    MeanRMS meanRMSatmospheric(atmospheric_data);
    meanRMSgas_old= meanRMSgas;
    meanRMSatmospheric_old= meanRMSatmospheric;
    gas_data_old=gas_data;
    atmospheric_data_old=atmospheric_data;
    if(gas_connected==true)
    {
      
      String query_gas = "?sensor_id="+String(BoardNumber*100+gas_number)+"&sensor_name="+gasName+"&number_measurements="+number_measurements+"&mean_temperature="+meanRMSgas.meanT()+"&mean_pressure="
      +meanRMSgas.meanP()+"&mean_humidity="+meanRMSgas.meanH()+"&rms_temperature="+meanRMSgas.RMST()+"&rms_pressure="+meanRMSgas.RMSP()+"&rms_humidity="+meanRMSgas.RMSH();
       if(client.connect(HOST_NAME, HTTP_PORT)) {
      // if connected:
      Serial.println("Connected to server");
      // make a HTTP request:
      // send HTTP header
      client.println(HTTP_METHOD + " " + PATH_NAME + query_gas + " HTTP/1.1");
      client.println("Host: " + String(HOST_NAME));
      client.println("Connection: close");
      client.println(); // end HTTP header

      while(client.connected()) {
        if(client.available()){
          // read an incoming byte from the server and print it to serial monitor:
          char c = client.read();
          Serial.print(c);
        }
      }

      // the server's disconnected, stop the client:
      client.stop();
    }
    else Serial.println("connection failed");
    }
    if(atmospheric_connected==true)
    {
      
      String query_atmospheric = "?sensor_id="+String(BoardNumber*100+atmospheric_number)+"&sensor_name="+atmosphericName+"&number_measurements="+number_measurements+"&mean_temperature="+meanRMSatmospheric.meanT()
      +"&mean_pressure="+meanRMSatmospheric.meanP()+"&mean_humidity="+meanRMSatmospheric.meanH()+"&rms_temperature="+meanRMSatmospheric.RMST()+"&rms_pressure="+meanRMSatmospheric.RMSP()+"&rms_humidity="+meanRMSatmospheric.RMSH();
      if(client2.connect(HOST_NAME, HTTP_PORT)) {
      // if connected:
      Serial.println("Connected to server");
      // make a HTTP request:
      // send HTTP header
      client2.println(HTTP_METHOD + " " + PATH_NAME + query_atmospheric + " HTTP/1.1");
      client2.println("Host: " + String(HOST_NAME));
      client2.println("Connection: close");
      client2.println(); // end HTTP header

      while(client2.connected()) {
        if(client2.available()){
          // read an incoming byte from the server and print it to serial monitor:
          char c = client2.read();
          Serial.print(c);
        }
     }

      // the server's disconnected, stop the client:
      client2.stop();
    }
    else Serial.println("connection failed");
    }
    
    gas_data.clear();
    atmospheric_data.clear();
    
  }
  else
  {
    if(myTime2-myTime1 >= mdelay)
    {
      
      myTime1=millis();
      if(gas_connected)
      {
        Data dat = BME280Data(&Serial,gas);
        gas_data.push_back(dat);
      }
      if(atmospheric_connected)
      {
        Data dat = BME280Data(&Serial,atmospheric);
        atmospheric_data.push_back(dat);
      }
    }
  }
  //Server stufs
  // listen for incoming clients
  EthernetClient client3 = server.available();
  if(client3)
  {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while(client3.connected())
    {
      if(client3.available())
      {
        char c = client3.read();
        //read MaxHeaderLength number of characters in the HTTP header
        //discard the rest until \n
        if(HttpHeader.length() < MaxHeaderLength)
        {
           //store characters to string
           HttpHeader = HttpHeader + c;
        }
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {

          /////////////////////PARSING HEADER ///////////////////////////
          size_t begin = HttpHeader.indexOf("GET /?")+6;
          size_t end = HttpHeader.indexOf(" HTTP/");
          HttpHeader=HttpHeader.substring(begin,end);
          String delay_header = returnValue(HttpHeader,"delay");
          if(delay_header!="") mdelay=delay_header.toInt();

          String measurement_header = returnValue(HttpHeader,"number_measurements");
          if(measurement_header!="") number_measurements=measurement_header.toInt();

          String gas_humidity_header = returnValue(HttpHeader,"gas_humidity");
          if(gas_humidity_header!="") defaultHumidity=gas_humidity_header.toFloat();
          HttpHeader="";
          ///////////////////////////////////////////////////////////////
          
          // send a standard http response header
          client3.println("HTTP/1.1 200 OK");
          client3.println("Content-Type: text/html");
          client3.println("Connection: close");
          client3.println();
          client3.println("<!DOCTYPE HTML><html><head>");

          
          client3.println("<style>");
          client3.println(".table-wrapper{margin: 0px 0px 0px 0px;box-shadow: 0px 0px 0px 0px;}");
          client3.println(".fl-table {border-radius: 5px;font-size: 12px;font-weight: normal;border: none;border-collapse: collapse;width: 49%;max-width: 49%;white-space: nowrap;background-color: white;}");

          client3.println(".fl-table td, .fl-table th {text-align: center;padding: 8px;}");

          client3.println(".fl-table td {font-size: 12px;}");

          client3.println(".fl-table thead th {background: #24cce5;}");
          client3.println(".fl-table thead th:nth-child(odd) {background: #49c197;}");
          client3.println(".fl-table tfoot th {background: #24cce5;}");
          client3.println(".fl-table tfoot th:nth-child(odd) {background: #49c197;}");
          client3.println(".fl-table tr:nth-child(even) { background: #F8F8F8;}");

          /* Responsive */

          client3.println("@media (max-width: 767px) {.fl-table {display: block;width: 49%;}");
          client3.println(".table-wrapper:before{content: 'Scroll horizontally >';text-align: center;font-size: 11px;color: white;padding: 0 0 10px;}");
          client3.println(".fl-table thead, .fl-table tbody, .fl-table thead th {display: block;}");
          client3.println(".fl-table thead {float: left;}");
          client3.println(".fl-table tbody {width: auto;position: block;overflow-x: auto;}");
          client3.println(".fl-table td, .fl-table th {padding: 20px .625em .625em .625em;height: 60px;box-sizing: border-box; overflow-x: hidden; overflow-y: auto; width: 120px; font-size: 13px;text-overflow: ellipsis; }");
          client3.println(".fl-table thead th { text-align: center;}");
          client3.println(".fl-table tbody tr { display: table-cell;}");
          client3.println(".fl-table tbody tr:nth-child(odd) { background: none; }");
          client3.println(".fl-table tr:nth-child(even) {background: transparent; }");
          client3.println(".fl-table tr td:nth-child(odd) {background: #F8F8F8;}");
          client3.println(".fl-table tbody td {display: block;text-align: center;}");


          client3.println("</style>");
          client3.println(String("<title>")+title+String("</title>"));
          client3.println("<meta http-equiv='refresh' content='10'>");
          client3.println("</head>");
          client3.println("<body style='background-color:#6291a7;'>");
          client3.println("<center>");

          client3.println(image);
          client3.println(String("<h1>")+title+String("</h1>"));


          client3.println("<form action='.'>");
          client3.println("Number measurements:<br><input type='number' name='number_measurements' value='"+String(number_measurements)+"' min='1' max='16'/><br>");
          client3.println("Delay between measurements (ms):<br><input type='number' name='delay' min='100' value='"+String(mdelay)+"'/><br>");
          client3.println("Gas humidity to obtain:<br><input type='number' name='gas_humidity' min='0.' max='100.' value='"+String(defaultHumidity)+"'/><br>");
          client3.println("<br><input class='button' type='submit' value='Submit'><br>");
          client3.println("</form>");

          
          if(gas_connected)
          {
            printTable(client3,"<h1>Gas values</h1>",gas_data_old,meanRMSgas_old);
          }
          if(atmospheric_connected)
          {
            printTable(client3,"<h1>Atmospheric values</h1>",atmospheric_data_old,meanRMSatmospheric_old);
          }
          client3.println("</center>"+String(version)+"</body></html>");
          break;
        }
        if(c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if(c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client3.stop();
  }
  myTime2=millis();
}
