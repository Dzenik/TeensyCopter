

#include <inttypes.h>
#include "pid.h"
#include "FlightData.h"
#include "drivers/eeprom.h"
#include <avr/interrupt.h>

/* ---- constructor ---- */
FlightData::FlightData( void ){
    armed = 0;
        
    SENSOR_DATA zd = {0}; //make a temporary zeroed version
    zero_data = zd; //copy it.
    
    for (uint8_t i = 0; i<4; i++) {
        tx_values[i] = last_tx[i] = MID_CONTROL;
    }
    tx_values[tx_throttle] = last_tx[tx_throttle] = MIN_CONTROL;
    
    command_used_number = 0;
    please_update_sensors = 0;
    user_feedback_i = user_feedback_m = 0;
    
    /*--configs--*/
    config.flying_mode = X_MODE; //X_MODE or PLUS_MODE
    config.led_mode = 0;
    config.pitch_roll_tx_scale = 4;
    config.yaw_tx_scale = 4;
}

/*--load_eeprom_config--*/
void FlightData::load_from_eeprom( void ){
    cli(); //turn off interrupts
    roll.p = EEPROM_read_16(0);
    roll.i = EEPROM_read_16(2);
    roll.d = EEPROM_read_16(4);
    pitch.p = EEPROM_read_16(6);
    pitch.i = EEPROM_read_16(8);
    pitch.d = EEPROM_read_16(10);
    yaw.p = EEPROM_read_16(12);
    yaw.i = EEPROM_read_16(14);
    yaw.d = EEPROM_read_16(16);
    alt.p = EEPROM_read_16(18); //added altitude pid
    alt.i = EEPROM_read_16(20);
    alt.d = EEPROM_read_16(22);
    
    config.flying_mode = (FlyingMode)EEPROM_read(24); //X_MODE or PLUS_MODE
    config.led_mode = EEPROM_read(25); 
    config.pitch_roll_tx_scale = EEPROM_read_16(26);
    config.yaw_tx_scale = EEPROM_read_16(28);
    
    zero_data.pitch = EEPROM_read_16(30);
    zero_data.roll = EEPROM_read_16(32);
    zero_data.yaw = EEPROM_read_16(34);
    sei(); //re-enable interrupts
}

/*--store the config to eeprom--*/
void FlightData::store_to_eeprom( void ){
    cli(); //turn off interrupts
    EEPROM_write_16(0, roll.p);
    EEPROM_write_16(2, roll.i);
    EEPROM_write_16(4, roll.d);
    EEPROM_write_16(6, pitch.p);
    EEPROM_write_16(8, pitch.i);
    EEPROM_write_16(10, pitch.d);
    EEPROM_write_16(12, yaw.p);
    EEPROM_write_16(14, yaw.i);
    EEPROM_write_16(16, yaw.d);
    EEPROM_write_16(18, alt.p);
    EEPROM_write_16(20, alt.i);
    EEPROM_write_16(22, alt.d);
    
    EEPROM_write(24, config.flying_mode); //X_MODE or PLUS_MODE
    EEPROM_write(25, config.led_mode); 
    EEPROM_write_16(26, config.pitch_roll_tx_scale);
    EEPROM_write_16(28, config.yaw_tx_scale);
    sei(); //re-enable interrupts
}
 
 void FlightData::store_eeprom_zero_data( void ){
     cli(); //turn off interrupts
     EEPROM_write_16(30, zero_data.pitch );
     EEPROM_write_16(32, zero_data.roll );
     EEPROM_write_16(34, zero_data.yaw );
     sei(); //re-enable interrupts
 }
 
