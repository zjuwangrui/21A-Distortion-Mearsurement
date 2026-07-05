/*
 * esp8266.h
 *
 *  Created on: 2021Äê11ÔÂ5ÈÕ
 *      Author: DoctorX
 */
#ifndef _ESP8266_H_
#define _ESP8266_H_


void esp8266_init(void);
void tcp_send(uint8_t id, uint8_t* pstr, uint32_t len) ;

#endif
