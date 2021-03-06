/*
 *  ser_pkt.cpp
 *  Created by Tim O'Brien
 *  
 *  used to encode, send, and decode serilized data
 *  created for UAVs and RC projects
 *  
 *  ideas and some code from microgear.googlecode.com
 */


/*  Packets look like this:
 *  
 *  /------------------------Header--------------------------/ /--data--/ /-------checksum-------/
 *  | Start uint8_t0 | Start1 | packet ID | packet Type | length | **data** | Checksum0 | Checksum1 |
 *  
 *  the data section is variable length is specified (in __ uint8_ts long).
 *  all uint8_ts are sent as uint8_t (unsigned 8 bit ints)
 */

#include "ser_pkt.h"
#include "drivers/uart.h"

#define START_OF_MSG0 0x93
#define START_OF_MSG1 0xE0



void console_write (uint8_t*,uint8_t);
void ugear_cksum (const uint8_t,const uint8_t,const uint8_t*,const uint8_t,uint8_t*,uint8_t* );


/*****************************************/
/*---------      send data      ---------*/
/*****************************************/

// Input:
//	- packet id,        - packet type,         -- 4x 16 bit int (only first one is required)
void send_int16_packet(uint8_t pktID, uint8_t pktTYPE, int16_t in0,int16_t in1,int16_t in2,int16_t in3){
	uint8_t data[8] = "";
	uint8_t * buf = data;	// copy the pointer
	
	buf[0] = in0;
	buf[1] = (in0 >> 8);
	buf[2] = in1;
	buf[3] = (in1 >> 8);
	buf[4] = in2;
	buf[5] = (in2 >> 8);
	buf[6] = in3;
	buf[7] = (in3 >> 8);

	/*
	*(int16_t *)buf = in0; buf += 2;
	*(int16_t *)buf = in1; buf += 2;
	*(int16_t *)buf = in2; buf += 2;
	*(int16_t *)buf = in3; buf += 2;
	*/
	
	send_packet(pktID, pktTYPE, data, (buf-data) );		//buf-data is the length of the datastream
}



void send_some_int16s(uint8_t pktID, uint8_t pktTYPE, int16_t * values, uint8_t number){
	uint8_t data[128/2] = ""; //max number of 8bit chars to send = 128
	unsigned int i=0;
	for(unsigned int j=0; j < number; j++){
		data[i++] = values[j];
		data[i++] = (values[j] >> 8);
	}

	send_packet(pktID, pktTYPE, data, i );
}


// Input:
//	- packet id,      - packet type,      - 8 bit int
void send_uint8_t_packet(uint8_t pktID, uint8_t pktTYPE, uint8_t in0){
	uint8_t data[1] = "";
	data[0] = in0;
	send_packet(pktID, pktTYPE, data, 1 );		//buf-data is the length of the datastream
}

// Input:
//	- packet id,     - packet type,      - 2x float (only first one is required)
void send_float_packet(uint8_t pktID, uint8_t pktTYPE, float d0, float d1){
	uint8_t data[10] = "";
	uint8_t * buf = data;	// copy the pointer
	
	int16_t transmit_x = (int16_t)(d0 * 100);
	*(int16_t *)buf = transmit_x; buf += 2;

	int16_t transmit_y = (int16_t)(d1 * 100);
	*(int16_t *)buf = transmit_y; buf += 2;
	
	send_packet(pktID, pktTYPE, data, (buf-data) );	//buf-data is the length of the datastream
}


/*****************************************/
/*---------     decode data     ---------*/
/*****************************************/

// takes serialized array of data and breaks it into vars
//	- very spesific structure, must be correctly decoded
//	- Input: pointer to encoded array. the length is part of the datatype spec.
//	- Returns: ...
ACCtelem decode_acc_data( uint8_t * buf ) {
    ACCtelem telem;
	
	int16_t recieved_x = *(int16_t *)buf; buf += 2;
	int16_t recieved_y = *(int16_t *)buf; buf += 2;
	
	telem.x = (double) recieved_x/100.0;
	telem.y = (double) recieved_y/100.0;

	return telem;
}

FourU16 decode_4xint16( uint8_t * buf ) {
	FourU16 recieved;
	
	recieved.d0 = buf[0] | (buf[1] << 8);
	recieved.d1 = buf[2] | (buf[3] << 8);
	recieved.d2 = buf[4] | (buf[5] << 8);
	recieved.d3 = buf[6] | (buf[7] << 8);

	return recieved;
}

void decode_some_int16s( uint8_t * buf, int16_t * values, uint8_t number ) {
	for(unsigned int i=0, j=0; j < number; ){
		values[j++] = buf[i] | (buf[i+1] << 8);
		i += 2;
	}
}

void decode_4xint16_alt( uint8_t * buf, unsigned int* d0, unsigned int* d1, unsigned int* d2, unsigned int*d3) {
	*d0 = buf[0] | (buf[1] << 8);
	*d1 = buf[2] | (buf[3] << 8);
	*d2 = buf[4] | (buf[5] << 8);
	*d3 = buf[6] | (buf[7] << 8);
}


/*****************************************/
/*---------       get data      ---------*/
/*****************************************/
/* process_incoming_packet
 * input: packet data array pointer to store to
 * input: index we're at in this data array
 * returns: 
 *    0: DONE, successful read.
 *    1: not done reading yet
 *    2: header0 fail
 *    3: header1 fail
 *    4: packet too long
 *    5: checksum fail.
 */
uint8_t process_incoming_packet(uint8_t * data, uint8_t * position)
{
    if (! uart_available() ) return 1;  //no new data, not done.
    
    data[ *position ] = uart_getchar();  //increment position at end of function
    
    /* ---- header ---- */
    if ((*position == 0)&&(data[0] != START_OF_MSG0)) { *position = 0; return 2; } //error, downloaded packet header missmatch.
    if ((*position == 1)&&(data[1] != START_OF_MSG1)) { *position = 0; return 3; } //error, downloaded packet header missmatch.
    
    if ((*position == 4) && (data[4] > 128)) { *position = 0; return 4; } //error, packet too long.
    
    if ((*position > 4) && (*position >= (5+data[4]+2) )) //we've read all data!! huzzah!
    {
        uint8_t cksum[2];
        ugear_cksum( data[2], data[3], data+5, data[4], &cksum[0], &cksum[1] );
        if ((cksum[0] != data[5+(data[4])+0]) || (cksum[1] != data[5+(data[4])+1])) {
            *position = 0;
            return 5; //error, checksum fail.
        }
        else {*position = 0; return 0; } //finally, sweet success.
    }
    //otherwise we're not done reading, increment the index.
    *position = *position + 1;
    return 1;
}

// sends the given data 
//	- works for any data structure that's already serialized
//	- Input: pointer to serialized (see packetize functions) data array
//	- Returns: nothing. sends data over serial channel.
void send_packet(uint8_t pktID, uint8_t pktTYPE, uint8_t *buf, uint8_t size ) {
	uint8_t header[5];
	uint8_t cksum[2];
	
	// start of message sync uint8_ts
	header[0] = START_OF_MSG0;
	header[1] = START_OF_MSG1;
	// packet id (1 uint8_t)
	header[2] = pktID;
	header[3] = pktTYPE;
	// packet size (1 uint8_t)
	header[4] = size;
	

	console_write(header , 5);

	// packet data
	console_write( buf, size );
	
	// check sum (2 uint8_ts)
	ugear_cksum( pktID, pktTYPE, buf, size, &cksum[0], &cksum[1] );
	console_write( cksum, 2 );

}

// prints some data array to serial
//	- Input:	-some data array
//				-data array length
//	- Returns: actually sends serial data only.
void console_write(uint8_t * data, uint8_t length){
	for (uint8_t i=0; i<length; i++) {
		uart_putchar(data[i]);
		//printf("%X ", data[i] );		
	}
}

// checksums an almost ready-to-go packet
//	- Input:	-packet type, both uint8_ts
//				-packet payload
//				-packet payload length
//				-address-of the places to store the 2 checksum values
// from microgear.googlecode.com
void ugear_cksum( const uint8_t hdr1, const uint8_t hdr2,
				 const uint8_t *buf, const uint8_t size,
				 uint8_t *cksum0, uint8_t *cksum1 )
{
    uint8_t c0 = 0;
    uint8_t c1 = 0;
	
    c0 += hdr1;
    c1 += c0;
	
    c0 += hdr2;
    c1 += c0;
	
    for ( uint8_t i = 0; i < size; i++ ) {
        c0 += (uint8_t)buf[i];
        c1 += c0;
    }
	
    *cksum0 = c0;
    *cksum1 = c1;
}

