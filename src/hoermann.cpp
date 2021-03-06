#include <stdint.h>
#include <stdbool.h>
#include "hoermann.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <unistd.h>
#include <algorithm> // std::fill
#include <sstream>
#include <ctime> // localtime
#include <stdlib.h>
#include <ostream> 

void Hoermann::init(int tx_pin)
{
    ser.serial_open(tx_pin);
}

void Hoermann::run_loop(void)
{
    RX_Buffer rx_buf;
    TX_Buffer tx_buf;
    unsigned long start;
    unsigned long s;
    ser.serial_read(rx_buf);

    s = micros();
    start = s;

    if (is_frame_corect(rx_buf))
    {   
        logy(buffer_to_string(rx_buf.buf,rx_buf.size), 5);
        if (is_slave_query(rx_buf))
        { 
            logy(buffer_to_string(rx_buf.buf,rx_buf.size), 3);
            if (is_slave_scan(rx_buf))
            {
                make_scan_responce_msg(rx_buf, tx_buf);
                while (true)
                {
                    if ((micros() - start) > (delay_msg))
                    {
                        if ((micros() - start) > max_frame_delay)
                        {
                            break;
                        }
                        scanning = true;

                        logy((String)("Pre Scan Response Time: "+(String)(micros() -s)), 4);

                        ser.serial_send(tx_buf);
                        scan_resp_time = micros() -s ;
                        logy(buffer_to_string(tx_buf.buf,tx_buf.size), 3);
                        logy((String)("Scan Response Time: "+ (String)(scan_resp_time)), 3);
                        break;
                    }
                }
            }
            else if (is_slave_status_req(rx_buf))
            {   
                
                make_status_req_msg(rx_buf, tx_buf);

                while (true)
                {
                    
                    if ((micros() - start) > (delay_msg))
                    {
                        if ((micros() - start) > max_frame_delay)
                        {
                            break;
                        }
                        connected = true;    
                        
                        logy((String)("Pre Req Response Time: "+(String)(micros() -s)), 4);
  
                        ser.serial_send(tx_buf);
                        req_resp_time = micros() -s;
                        logy(buffer_to_string(tx_buf.buf,tx_buf.size), 3);
                        logy((String)("Req Response Time: "+(String)req_resp_time), 3);
                        
                        break;
                    }

                }
            }
        }
        else if (is_broadcast(rx_buf))
        {
            if (is_broadcast_lengh_correct(rx_buf))
            {
                update_broadcast_status(rx_buf);
                broadcast_recv = true;
                logy((String)("State Update Time: "+(String)(micros() -s)), 5);
            }
        }
        
    }
}

int Hoermann::set_delay(int delay_)
{
    delay_msg = delay_;
}


void Hoermann::logy(String msg, int level)
{
    if (level == debud_level)
    {
        Serial.println(msg.c_str());
    }
}

void Hoermann::enable_debug(int level)
{
    debud_level = level;
}

String Hoermann::is_connected()
{
    if (connected)
    {
        return (String)"1";
    }
    else
    {
        return (String)"0";
    }
}
String Hoermann::is_scanning()
{
    if (scanning)
    {
        return (String)"1";
    }
    else
    {
        return (String)"0";
    }
}


String Hoermann::is_broadcast_recv()
{
    if (broadcast_recv)
    {
        return (String)"1";
    }
    else
    {
        return (String)"0";
    }
}     

void Hoermann::reset_broadcast()
{
    broadcast_recv = false;
}

void Hoermann::reset_connected()
{
    connected = false;
}

void Hoermann::reset_scanning()
{
    scanning = false;
}

int Hoermann::get_scan_resp_time()
{
    return scan_resp_time;
}

int Hoermann::get_req_resp_time()
{
    return req_resp_time;
}

void Hoermann::update_broadcast_status(RX_Buffer &buf)
{
    if (static_cast<uint8_t>(broadcast_status) != buf.buf[2])
    {
        broadcast_status = static_cast<uint16_t>(buf.buf[2]);
    }
}

uint8_t Hoermann::get_length(RX_Buffer &buf)
{
    if (buf.size > 2)
    {
        return buf.buf[1] & 0x0F;
    }
    else
        return 0x00;
}

uint8_t Hoermann::get_counter(RX_Buffer &buf)
{
    if (buf.size > 2)
    {
        return (buf.buf[1] & 0xF0) + 0x10;
    }
    else
        return 0x00;
}

bool Hoermann::is_broadcast(RX_Buffer &buf)
{
    if (buf.size == 5)
    {
        if (buf.buf[0] == BROADCAST_ADDR)
        {
            return true;
        }
        else
            return false;
    }
    else
    {
        return false;
    }
}

bool Hoermann::is_slave_query(RX_Buffer &buf)
{
    if (buf.size > 3 && buf.size < 6)
    {
        if (buf.buf[0] == UAP1_ADDR)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}
bool Hoermann::is_frame_corect(RX_Buffer &buf)
{
    if (buf.size > 3 && buf.size < 6)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Hoermann::is_slave_scan(RX_Buffer &buf)
{
    if (buf.size == 5)
    {
        if (is_broadcast_lengh_correct(buf) && (buf.buf[2] == CMD_SLAVE_SCAN))
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}

bool Hoermann::is_slave_status_req(RX_Buffer &buf)
{
    if (buf.size == 4)
    {
        if (is_req_lengh_correct(buf) && (buf.buf[2] == CMD_SLAVE_STATUS_REQUEST))
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}

bool Hoermann::is_broadcast_lengh_correct(RX_Buffer &buf)
{
    if (get_length(buf) == broadcast_lengh)
        return true;
    else
        return false;
}

bool Hoermann::is_req_lengh_correct(RX_Buffer &buf)
{
    if (get_length(buf) == reguest_lengh)
        return true;
    else
        return false;
}

String Hoermann::buffer_to_string(uint8_t *buf, int size )
{

    String stream;
    
    if (size > 0)
    {
        stream += size;
        stream += " | ";
        for (int i = 0; i < (int)size; i++)
        {
            stream += " ";
            char str[10];
            sprintf(str,"%x",buf[i]); //converts to hexadecimal base.
            stream += str;
        }
        return stream.c_str();
    }
    return " ";
}

void Hoermann::print_buffer(TX_Buffer &buf)
{

    if (buf.size > 0)
    {
        Serial.print((int)buf.size, DEC);
        Serial.print(" | ");
        for (int i = 0; i < (int)buf.size; i++)
        {

            Serial.print(" ");
            Serial.print(buf.buf[i], HEX);
        }
        Serial.println();
    }
}

uint8_t Hoermann::get_master_address()
{
    return master_address;
}

void Hoermann::make_scan_responce_msg(RX_Buffer &rx_buf, TX_Buffer &tx_buf)
{
    tx_buf.buf[0] = get_master_address();
    tx_buf.buf[1] = 0x02 | get_counter(rx_buf);
    tx_buf.buf[2] = UAP1_TYPE;
    tx_buf.buf[3] = UAP1_ADDR;
    tx_buf.buf[4] = calc_crc8(tx_buf.buf, 4);
    tx_buf.timeout = 1;
    tx_buf.size = 5;
}

void Hoermann::make_status_req_msg(RX_Buffer &rx_buf, TX_Buffer &tx_buf)
{
    tx_buf.buf[0] = get_master_address();
    tx_buf.buf[1] = 0x03 | get_counter(rx_buf);

    tx_buf.buf[2] = CMD_SLAVE_STATUS_RESPONSE;
    if (slave_respone_data == SEND_STOP)
    {
        tx_buf.buf[3] = 0x00;
        tx_buf.buf[4] = 0x00;
    }
    else
    {
        tx_buf.buf[3] = static_cast<uint8_t>(slave_respone_data);
        tx_buf.buf[4] = 0x10;
    }
    slave_respone_data = RESPONSE_DEFAULT;
    tx_buf.buf[5] = calc_crc8(tx_buf.buf, 5);
    tx_buf.timeout = 1;
    tx_buf.size = 6;
}


String Hoermann::get_state()
{
    if ((broadcast_status) == RESPONSE_DEFAULT)
    {
        String stat = "default";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_OPEN)
    {
        String stat = "open";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_CLOSED)
    {
        String stat = "closed";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_OPENING)
    {
        String stat = "opening";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_CLOSING)
    {
        String stat = "closing";
        return stat;
    }  
    else if ((broadcast_status) == RESPONSE_STOP) // stoped ?????????
    {
        String stat = "stoped";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_WALK_IN)
    {
        String stat = "walk_in";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_OPTIONAL_RELAY)
    {
        String stat = "optional_relay";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_LIGHT_RELAY)
    {
        String stat = "light_relay";
        return stat;
    }
    else if ((broadcast_status) == RESPONSE_CLOSING_ERROR)
    {
        String stat = "closing_error";
        return stat;
    }    
    else
    {
        String stat = "error";
        return stat;
    }
    
}
String Hoermann::get_state_hex()
{
    uint8_t buf[1] = {(uint8_t)broadcast_status}; 
    String status = buffer_to_string(buf, 1);
    return status;
}

void Hoermann::set_state(String action)
{
    if (action == "stop" || action == "STOP")
    {
        slave_respone_data = SEND_STOP;
    }
    else if (action == "open" || action == "OPEN")
    {
        slave_respone_data = SEND_OPEN;
    }
    else if (action == "close" || action == "CLOSE")
    {
        slave_respone_data = SEND_CLOSE;
    }
    else if (action == "toggle" || action == "TOGGLE")
    {
        slave_respone_data = SEND_TOGGLE;
    }
    else if (action == "walk_in" || action == "WALK_IN")
    {
        slave_respone_data = SEND_WALK_IN;
    }
    else if (action == "light" || action == "LIGHT")
    {
        slave_respone_data = SEND_TOGGLE_LIGHT;
    }

}

uint8_t Hoermann::calc_crc8(uint8_t *p_data, uint8_t len)
{
    size_t i;
    uint8_t crc = CRC8_INITIAL_VALUE;
    while (len--)
    {
        crc ^= *p_data++;
        for (i = 0; i < 8; i++)
        {
            if (crc & 0x80)
            {
                crc <<= 1;
                crc ^= 0x07;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return (crc);
}

void Hoermann::gate_open()
{
    set_state("open");
}

void Hoermann::gate_close()
{
    set_state("close");
}

void Hoermann::gate_walk_in()
{
    set_state("walk_in");
}

void Hoermann::gate_toggle_light()
{
    set_state("light");
}

void Hoermann::gate_stop()
{
    set_state("stop");
}
