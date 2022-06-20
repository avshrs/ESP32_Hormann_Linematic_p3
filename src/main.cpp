#include <string.h>
#include <Wire.h>
#include "passwd.h"
#include "wifi_mqtt.h"
#include "hoermann.h"



int d = 1; 
unsigned long previousMillis = 0;  
unsigned long previousMillis2 = 0;  
Hoermann hoermann; 
String state = "n/a";


void callback(char* topic, byte* payload, unsigned int length) 
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String st;
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        st +=(char)payload[i];
    }
    Serial.println();
    
    if (strcmp(topic,"avshrs/devices/hormann_gate_01/set/gate") == 0)  
    {   
        if (st == "open" || st == "OPEN" || st == "ON")
        {
            hoermann.gate_open();
        }
        else if (st == "close" || st == "CLOSE" || st == "OFF" )
        {
            hoermann.gate_close();
        }
        else if (st == "stop" || st == "STOP"  )
        {
            hoermann.gate_stop();
        }    
        else if (st == "walk_in" || st == "WALK_IN"  )
        {
            hoermann.gate_walk_in();
        } 
        else if (st == "press" || st == "PRESS"  )
        {
            hoermann.gate_toggle_light();
        }  
    } 
    else if (strcmp(topic,"avshrs/devices/hormann_gate_01/set/walk_in") == 0)  
    {   
        if (st == "walk_in" || st == "WALK_IN"|| st == "ON" )
        {
            hoermann.gate_walk_in();
        }
        else if (st == "close" || st == "CLOSE" || st == "OFF" )
        {
            hoermann.gate_close();
        }
    } 
    else if (strcmp(topic,"avshrs/devices/hormann_gate_01/set/light") == 0)  
    {   
        if (st == "PRESS" )
        {
            hoermann.gate_toggle_light();
        }   
    }    
    else if (strcmp(topic,"avshrs/devices/hormann_gate_01/set/debug") == 0)  
    {   
        hoermann.enable_debug(atoi((char *)payload));
    }
    else if (strcmp(topic,"avshrs/devices/hormann_gate_01/set/delay_msg") == 0)  
    {   
        Serial.print("delay_msg: ");
        Serial.println(st);
        hoermann.set_delay(st.toInt());
    } 

    else if (strcmp(topic,"avshrs/devices/hormann_gate_01/esp_led") == 0 && (char)payload[0] == '1') 
    {
        Serial.println("BUILTIN_LED_low");
        digitalWrite(BUILTIN_LED, LOW);   
    } 
    else if (strcmp(topic,"avshrs/devices/hormann_gate_01/esp_led") == 0 && (char)payload[0] == '0') 
    {
        Serial.println("BUILTIN_LED_high");
        digitalWrite(BUILTIN_LED, HIGH); 
    }


}

void setup() 
{
    int EnTxPin =  18;
    Wire.begin();
    hoermann.init(EnTxPin);

    Serial.begin(1000000);
    
    pinMode(EnTxPin, OUTPUT);  
    digitalWrite(EnTxPin, LOW);
    pinMode(BUILTIN_LED, OUTPUT);  
    digitalWrite(BUILTIN_LED, LOW);   
    setup_wifi();

    client.setServer(mqtt_server, 1883);
    client.setBufferSize(1024);
    client.setCallback(callback);
    reconnect();
    prepare_conf();
}

void gate_position(boolean force)
{
    if (state != hoermann.get_state() || force ) 
    {
        state = hoermann.get_state();
        
        if (state == "open")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", state.c_str());
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "ON");
            client.publish("avshrs/devices/hormann_gate_01/state/state", state.c_str());
        }
        else if (state == "opening")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", state.c_str());
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "ON");
            client.publish("avshrs/devices/hormann_gate_01/state/state", state.c_str());
        }
        else if (state == "closed")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", state.c_str());
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "OFF");
            client.publish("avshrs/devices/hormann_gate_01/state/state", state.c_str());
        }
        else if (state == "closing")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", state.c_str());
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "OFF");
            client.publish("avshrs/devices/hormann_gate_01/state/state", state.c_str());
        }
        else if (state == "stoped")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", "open");
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "on");
            client.publish("avshrs/devices/hormann_gate_01/state/state", state.c_str());
        }
        else if (state == "walk_in")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", "open");
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "ON");
            client.publish("avshrs/devices/hormann_gate_01/state/state", "walk_in");
        }
        else if (state == "closing_error")
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", "open");
            client.publish("avshrs/devices/hormann_gate_01/state/walk_in", "ON");
            client.publish("avshrs/devices/hormann_gate_01/state/state", "closing_error");
        }
        else
        {
            client.publish("avshrs/devices/hormann_gate_01/state/gate", hoermann.get_state().c_str());
        }
        String data = "hex state: " + hoermann.get_state_hex();
        client.publish("avshrs/devices/hormann_gate_01/status/gate", data.c_str());
    }
}

void loop() 
{
    currentMillis = millis();

    if (!client.connected()) 
    {
        reconnect();
    }
    client.loop();
    hoermann.run_loop();
    
    if (currentMillis - previousMillis >= 60000) 
    {
        previousMillis = currentMillis;

        wifi_status();

        snprintf (msg, MSG_BUFFER_SIZE, "true");
        client.publish("avshrs/devices/hormann_gate_01/status/connected", msg);

        client.publish("avshrs/devices/hormann_gate_01/status/master_sending_broadcast", hoermann.is_broadcast_recv().c_str());
        hoermann.reset_broadcast();

        client.publish("avshrs/devices/hormann_gate_01/status/master_is_scanning", hoermann.is_scanning().c_str());
        hoermann.reset_scanning();

        client.publish("avshrs/devices/hormann_gate_01/status/response_to_master", hoermann.is_connected().c_str());
        hoermann.reset_connected();

        snprintf (msg, MSG_BUFFER_SIZE, "%i", hoermann.get_scan_resp_time());
        client.publish("avshrs/devices/hormann_gate_01/status/scan_resp_time", msg);
        
        snprintf (msg, MSG_BUFFER_SIZE, "%i", hoermann.get_req_resp_time());
        client.publish("avshrs/devices/hormann_gate_01/status/req_resp_time", msg);
        client.publish("avshrs/devices/hormann_gate_01/state/light", "OFF");
        gate_position(true);        
    }

    gate_position(false);        
    
}