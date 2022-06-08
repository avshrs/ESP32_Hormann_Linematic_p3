#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>


#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
unsigned long currentMillis;

WiFiClient espClient;
PubSubClient client(espClient);
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

unsigned long old_mils = 60000;



String make_discover(char* dev_type_, char* dev_name_, char *dev_name_ha, char * sens_name, char * unique_id, String entity_settings)
{
String md = (String)"{\"avty\":{\"topic\":\"avshrs/devices/" + (String)dev_name_ ;
md += (String)"/status/connected\",\"payload_available\":\"true\",\"payload_not_available\":\"false\"},\"~\":\"avshrs/devices/"+(String)dev_name_+"\",";
md += (String)"\"device\":{\"ids\":\"" + (String)dev_name_ + (String)"\",\"mf\":\"Avshrs\",\"name\":\""+ (String)dev_name_ha + (String)"\",\"sw\":\"0.0.1\"},";
md += (String)"\"name\":\""+ (String)sens_name + (String)"\",\"uniq_id\":\""+ (String)unique_id + "\",\"qos\":0," ;
md += (String)entity_settings;
return md;
}


void prepare_conf()
{
    String s1 = "\"command_topic\":\"~/set/gate\",\"state_topic\":\"~/state/gate\",\"device_class\":\"gate\"}";
    String s1_ = make_discover("cover", "hormann_gate_01", "Linematic-01", "Gate Main", "hormann_gate_main_01",s1);
    client.publish("homeassistant/cover/hormann_gate_main_01/config", s1_.c_str(), true);

    String s5 = "\"command_topic\":\"~/set/walk_in\",\"state_topic\":\"~/state/walk_in\",\"payload_on\":\"walk_in\",\"payload_off\":\"close\",\"state_on\":\"ON\",\"state_off\":\"OFF\"}";
    String s5_ = make_discover("switch", "hormann_gate_01", "Linematic-01", "Gate Walk In", "hormann_gate_walk_in_button_01",s5);
    client.publish("homeassistant/switch/hormann_gate_walk_in_switch_01/config", s5_.c_str(), true);

    String s3 = "\"state_topic\":\"~/state/state\"}";
    String s3_ = make_discover("sensor", "hormann_gate_01", "Linematic-01", "Gate State", "hormann_gate_state_01",s3);
    client.publish("homeassistant/sensor/hormann_gate_state_01/config", s3_.c_str(), true);

    String s4 = "\"command_topic\":\"~/set/light\",\" payload_press\":\"light\"}";
    String s4_ = make_discover("button", "hormann_gate_01", "Linematic-01", "Gate Light", "hormann_gate_light_01",s4);
    client.publish("homeassistant/button/hormann_gate_light_01/config", s4_.c_str(), true);
}

void reconnect() 
{
  // Loop until we're reconnected
    if (!client.connected())
    { 
        if (millis() - old_mils > 60000)
        {
            old_mils = millis();
            Serial.print("Attempting MQTT connection...");
            // Create a random client ID
            String clientId = "ESP_hormann_gate";
            
            clientId += String(random(0xffff), HEX);
            // Attempt to connect
            if (client.connect(clientId.c_str(),"mqttuser", "mqttuser")) 
            {
                Serial.println("connected to MQTT server");
                // MQTT subscription
                client.subscribe("avshrs/devices/hormann_gate_01/set/gate");                
                client.subscribe("avshrs/devices/hormann_gate_01/set/light");
                client.subscribe("avshrs/devices/hormann_gate_01/set/walk_in");

                client.subscribe("avshrs/devices/hormann_gate_01/esp_led");

                client.subscribe("avshrs/devices/hormann_gate_01/set/delay_msg");
                client.subscribe("avshrs/devices/hormann_gate_01/set/debug");
                prepare_conf();

            } 
        }
    }
}


String uptime(unsigned long milli)
{
  static char _return[32];
  unsigned long secs=milli/1000, mins=secs/60;
  unsigned int hours=mins/60, days=hours/24;
  milli-=secs*1000;
  secs-=mins*60;
  mins-=hours*60;
  hours-=days*24;
  sprintf(_return,"Uptime %d days %2.2d:%2.2d:%2.2d.%3.3d", (byte)days, (byte)hours, (byte)mins, (byte)secs, (int)milli);
  String ret =  _return;
  return ret;
}


void wifi_status()
{
    String ip = IpAddress2String(WiFi.localIP());
    client.publish("avshrs/devices/hormann_gate_01/status/wifi_ip", ip.c_str());
    String mac = WiFi.macAddress();
    client.publish("avshrs/devices/hormann_gate_01/status/wifi_mac", mac.c_str());
    snprintf (msg, MSG_BUFFER_SIZE, "%i", WiFi.RSSI());
    client.publish("avshrs/devices/hormann_gate_01/status/wifi_rssi", msg);
    int signal = 2*(WiFi.RSSI()+100);
    snprintf (msg, MSG_BUFFER_SIZE, "%i", signal);
    client.publish("avshrs/devices/hormann_gate_01/status/wifi_signal_strength", msg);
    
    client.publish("avshrs/devices/hormann_gate_01/status/uptime", uptime(currentMillis).c_str());
}


void setup_wifi() 
{
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
