/*
 * Module:		$RCSfile: controllerAddresses.hpp $ - header file
 *				$Revision: 1.9 $
 *
 * Author:		john ulshoeffer
 *
 * Affiliation:	Teradyne, Inc.
 *
 * Copyright:	This  is  an  unpublished  work.   Any  unauthorized
 *				reproduction  is  PROHIBITED.  This unpublished work
 *				is protected by Federal Copyright Law.  If this work
 *				becomes published, the following notice shall apply:
 *
 *				(c) COPYRIGHT 2000, TERADYNE, INC.
 *
 * Discussion:
 *
 * Date:		April. 29, 2000
 *
 */

#if !defined(_CONTROLLERADDRESSES_HPP__INCLUDED_)
#define _CONTROLLERADDRESSES_HPP__INCLUDED_


enum {STOP, RUN};

/*******************************************************************************/
/* controller C51134 addresses                                                 */
/*******************************************************************************/
// btye addressing
#define DOME_RESET_134     0x0001
#define RUN_134            0x0002
#define BEGIN_SEQUENCE_134 0x0003   //[7:0] begin_sequence
#define END_SEQUENCE_134   0x0004   //[7:0] end_sequence set to 1 + the last strobe defn sequence.

#define CAMDELAY_134       0x0005	//[9:0] camera_delay_reg  w
#define LIGHTDELAY_134     0x0006	//[9:0] light_delay_reg   w
#define CAMERAONTIME_134   0x0007	//[9:0] camera on-time    w

#define XPOSITION1_134     0x0008
#define XPOSITION2_134     0x0009
#define WAIT_POS_0_134     0x000A   //[15:0] current dome wait position low byte
#define WAIT_POS_2_134     0x000B   //[15:0] current dome wait position low byte
#define PAGE_134           0x000C
#define OPTO1              0x0020
#define OPTO1DIR           0x0021
#define OPTO1MASK          0x0022
#define OPTO1STATUS        0x0024

#define OPT0_DATA_134      0x0000
#define OPT0_DIR_134       0x0001
#define OPT0_MASK_134      0x0002
#define OPT0_ACTION_134    0x0003
#define OPT0_STATUS_134    0x0004

#define OPTO2              0x0028
#define OPTO2DIR           0x0029
#define OPTO2MASK          0x002A
#define OPTO2STATUS        0x002C

#define LIGHT_DEFN_ADDRESS 0x1000
#define STRB_DEFN_ADDRESS  0x1800
#define TEMP_LIGHT_DEFN_ADDRESS	0x17F8		//last possible lighting definition location
#define TEMP_STRB_DEFN_ADDRESS  0x1FFc		//last possible strobe definition location

/*******************************************************************************/
/* controller C51140 addresses                                                 */
/*******************************************************************************/
// word addressing
#define IOBITS_INVALID     0x00F0   // unused address on controller (used like NULL)

#define CONTROL_TYPE       0x0000   // [15:0] RO = read only
#define CONTROL_FIRM_MAJOR 0x0001   // [15:0] RO
#define CONTROL_FRIM_MINOR 0x0002   // [15:0] RO

#define REG_PAGE           0x0003   // [15:0] page register
#define CONTROLLER_RESET   0x0004   // [0]    1 == reset,     0 == normal;      [15:1]  undefined

#define WD_TIMEOUT   0x0010      // [11:0] mil sec
#define WD_MASK            WD_TIMEOUT + 1 // [0] 1 == enabled    0 == disabled;   [15:1]  undefined
#define WD_STATUS          WD_TIMEOUT + 3 // [0] 1 == interrupt, 0 == normal;     [15:1]  undefined
#define WD_ENABLE          WD_TIMEOUT + 4 // [0] 1 == enabled    0 == disabled;   [15:1]  undefined

#define PRETIMEOUT         0x0018         // unused
// #define PTO_MASK           PRETIMEOUT + 1 // [15:0]  undefined // These Registers do not exist 
// #define PTO_ACTION         PRETIMEOUT + 2 // [15:0]  undefined //  ""
#define PTO_STATUS         PRETIMEOUT + 3 // [0] 1 == interrupt, 0 == normal;  [15:1]  undefined

#define RUN_SEQ            0x0020   // [0] 0x01 == run, 0x00 == stop;          [15:1]  undefined
#define CURRENT_SEQ        0x0021   // [15:0] Strobe Definition position
#define END_SEQ            0x0022   // [15:0] CURRENT_SEQ + 1
#define REG_CAMERA_DELAY   0x0023   // [9:0];                                  [15:10] undefined
#define REG_LIGHT_DELAY    0x0024   // [9:0];                                  [15:10] undefined
#define REG_CAMERA_ONTIME  0x0025   // [9:0];                                  [15:10] undefined

#define CURRENT_X_POS_1    0x0026   // [15:0] lsb, must be read first when reading
#define CURRENT_X_POS_2    0x0027   // [15:0] msb
#define CURRENT_Y_POS_1    0x0028   // [15:0] lsb, must be read first when reading
#define CURRENT_Y_POS_2    0x0029   // [15:0] msb
#define STROBE_POS_1       0x002A   // [15:0] lsb, must be read first when reading: RO
#define STROBE_POS_2       0x002B   // [15:0] msb; RO

#define ENCODER_DIR        0x002B   // [0] 0 == X,  1 == Y                     [15:10] undefined

#define LT_RED             0x0040   // [0] 3 == On, 2 = P2, 1 = P1, 0 == off;  [15:2]  undefined
#define LT_YELLOW          0x0041   // [0] 3 == On, 2 = P2, 1 = P1, 0 == off;  [15:2]  undefined
#define LT_GREEN           0x0042   // [0] 3 == On, 2 = P2, 1 = P1, 0 == off;  [15:2]  undefined
#define LT_BLUE            0x0043   // [0] 3 == On, 2 = P2, 1 = P1, 0 == off;  [15:2]  undefined
#define LT_WHITE           0x0044   // [0] 3 == On, 2 = P2, 1 = P1, 0 == off;  [15:2]  undefined
#define LT_BEEPER1         0x0045   // [0] 3 == On,                 0 == off;  [15:2]  undefined
#define LT_BEEPER2         0x0046   // [0] 3 == On,                 0 == off;  [15:2]  undefined
#define LT_DRIVERS         0x0047   // [0] 3 == On,                 0 == off;  [15:2]  undefined
                                    // LT_24V must be turned 'on' prior to all LT actions

#define LT_FLASH_DURATION  0x0048	// [32:16]([15:0] implied 0000) flash duration in usec.

// Fans;  AUX0-3 12V, AUX4-7 24V
#define AUXFAN0            0x0060   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN1            0x0061   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN2            0x0062   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN3            0x0063   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN4            0x0064   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN5            0x0065   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN6            0x0066   // [0] 1 == On, 0 == off;                  [15:1]  undefined
#define AUXFAN7            0x0067   // [0] 1 == On, 0 == off;                  [15:1]  undefined

// operator pannel interrupts
// load button
#define LOAD_DATA          0x0070         // [0] 1 == On,        0 == off;     [15:1]  undefined
#define LOAD_ENABLE        LOAD_DATA + 1  // [0] 1 == enabled    0 == disabled;[15:1]  undefined
#define LOAD_ACTION        LOAD_DATA + 2  // [0] 1 == rising,    0 == falling; [15:1]  undefined
#define LOAD_STATUS        LOAD_DATA + 3  // [0] 1 == interrupt, 0 == normal;  [15:1]  undefined
// inspect button
#define INSPECT_DATA       0x0074
#define INSPECT_ENABLE     INSPECT_DATA + 1
#define INSPECT_ACTION     INSPECT_DATA + 2
#define INSPECT_STATUS     INSPECT_DATA + 3
// eject button
#define EJECT_DATA         0x0078
#define EJECT_ENABLE       EJECT_DATA + 1
#define EJECT_ACTION       EJECT_DATA + 2
#define EJECT_STATUS       EJECT_DATA + 3
// cancel button
#define CANCEL_DATA        0x007C
#define CANCEL_ENABLE      CANCEL_DATA + 1
#define CANCEL_ACTION      CANCEL_DATA + 2
#define CANCEL_STATUS      CANCEL_DATA + 3
// OPT1 button
#define OPT1_DATA          0x0080
#define OPT1_ENABLE        OPT1_DATA + 1
#define OPT1_ACTION        OPT1_DATA + 2
#define OPT1_STATUS        OPT1_DATA + 3
// OPT2 button
#define OPT2_DATA          0x0084
#define OPT2_ENABLE        OPT2_DATA + 1
#define OPT2_ACTION        OPT2_DATA + 2
#define OPT2_STATUS        OPT2_DATA + 3
// OPT3 button
#define OPT3_DATA          0x0088
#define OPT3_ENABLE        OPT3_DATA + 1
#define OPT3_ACTION        OPT3_DATA + 2
#define OPT3_STATUS        OPT3_DATA + 3
// 7 button
#define AUX1_DATA          0x008C
#define AUX1_ENABLE        AUX1_DATA + 1
#define AUX1_ACTION        AUX1_DATA + 2
#define AUX1_STATUS        AUX1_DATA + 3
// 8 button
#define AUX2_DATA          0x0090
#define AUX2_ENABLE        AUX2_DATA + 1
#define AUX2_ACTION        AUX2_DATA + 2
#define AUX2_STATUS        AUX2_DATA + 3
// 9 button
#define AUX3_DATA          0x0094
#define AUX3_ENABLE        AUX3_DATA + 1
#define AUX3_ACTION        AUX3_DATA + 2
#define AUX3_STATUS        AUX3_DATA + 3
// button status
#define PANEL_STATUS       0x0098   // [9:0] interrupt status, switches 9 - 0; [15:10] undefined, RO; 

// panel leds
#define LOAD_LED           0x00A0   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define INSPECT_LED        0x00A1   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define EJECT_LED          0x00A2   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define CANCEL_LED         0x00A3   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define OPT1_LED           0x00A4   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define OPT2_LED           0x00A5   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define OPT3_LED           0x00A6   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define AUX1_LED           0x00A7   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define AUX2_LED           0x00A8   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define AUX3_LED           0x00A9   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define PANEL_LED_DRV      0x00AF   // [0] 1 == On,          0 == off;         [15:1]  undefined

// power interrupts
// panel led driver interrupt
#define LED_DRV_DATA       0x00B0           // [0] 1 == On,        0 == off;     [15:1]  undefined
#define LED_DRV_ENABLE     LED_DRV_DATA + 1 // [0] 1 == enabled    0 == disabled;[15:1]  undefined
#define LED_DRV_ACTION     LED_DRV_DATA + 2 // [0] 1 == rising,    0 == falling; [15:1]  undefined
#define LED_DRV_STATUS     LED_DRV_DATA + 3 // [0] 1 == interrupt, 0 == normal;  [15:1]  undefined
// ltower driver interrupt
#define LT_DRV_DATA        0x00B4
#define LT_DRV_ENABLE      LT_DRV_DATA + 1
#define LT_DRV_ACTION      LT_DRV_DATA + 2
#define LT_DRV_STATUS      LT_DRV_DATA + 3
// 12V driver interrupt
#define V12_DRV_DATA       0x00B8
#define V12_DRV_ENABLE     V12_DRV_DATA + 1
#define V12_DRV_ACTION     V12_DRV_DATA + 2
#define V12_DRV_STATUS     V12_DRV_DATA + 3
// 24V driver interrupt
#define V24_DRV_DATA       0x00BC
#define V24_DRV_ENABLE     V24_DRV_DATA + 1
#define V24_DRV_ACTION     V24_DRV_DATA + 2
#define V24_DRV_STATUS     V24_DRV_DATA + 3

// ALL interrupt status
#define ALL_STATUS         0x00C0   // [15]  opto_39_32,        [14]  opto_32_16,     [13] opto_15_0 
                                    // [12]  dome_error_strobe, [11]  panel,          [10] interlock,
                                    // [09]  watch_dog,         [08]  driver_status,  [07] P/S, 
									// [06]  extra,             [05]  system timeout, [04] Pre timeout
									// [00]  diagnostic			[3:1] undefined; RO;

#define DRV_STATUS         0c00C1   // [3] panel_driver_int, [2] light_driver_int, [1] aux12_int,
                                    // [0] aux24_int; [15:4]  undefined

#define DAC_ONE            0x00C2
#define DAC_TWO            0x00C3

#define DIAGNOSTIC_DATA    0x00C4
#define DIAGNOSTIC_ENABLE  DIAGNOSTIC_DATA + 1 // [0] 1 == enabled    0 == disabled;[15:1]  undefined
#define DIAGNOSTIC_LOADEN  DIAGNOSTIC_DATA + 2 // [0] 1 == rising,    0 == falling; [15:1]  undefined
#define DIAGNOSTIC_STATUS  DIAGNOSTIC_DATA + 3 // [0] 1 == interrupt, 0 == normal;  [15:1]  undefined

// PS error interrupt
#define PS_DATA            0x00D0      // [0] 1 == On,        0 == off;     [15:1]  undefined
#define PS_ENABLE          PS_DATA + 1 // [0] 1 == enabled    0 == disabled;[15:1]  undefined
#define PS_LOADEN          PS_DATA + 2 // [0] 1 == rising,    0 == falling; [15:1]  undefined
#define PS_STATUS          PS_DATA + 3 // [0] 1 == interrupt, 0 == normal;  [15:1]  undefined

#define LASER_POLARITY     0x002D   // [0] 1 == active high,  0 == low;     [15:1]  undefined
// laser PS switch
#define LASER_DATA         0x00D4   // [0] 1 == On,           0 == off;     [15:1]  undefined

// done LED drver timeout
#define DOME_DRV_ENABLE    0x00D5   // [0] 1 == enabled       0 == disabled;[15:1]  undefined
#define DOME_DRV_STATUS    0x00D7   // [0] 1 == interrupt,    0 == normal;  [15:1]  undefined

#define INTERLOCK_DATA     0x00D8
#define INTERLOCK_ENABLE   INTERLOCK_DATA + 1
#define INTERLOCK_ACTION   INTERLOCK_DATA + 2
#define INTERLOCK_STATUS   INTERLOCK_DATA + 3
#define EXTRA_DATA         0x00DC
#define EXTRA_ENABLE       EXTRA_DATA + 1
#define EXTRA_ACTION       EXTRA_DATA + 2
#define EXTRA_STATUS       EXTRA_DATA + 3

// opto controls, interrupts
#define DATA_ADDR            0x0000   // [0] 1 == On,          0 == off;         [15:1]  undefined
#define ENABLE_ADDR          0x0001   // [0] 1 == enabled,     0 == disabled;    [15:1]  undefined
#define ACTION_ADDR          0x0002   // [0] 1 == rising edge, 0 == falling edge;[15:1]  undefined
#define STATUS_ADDR          0x0003   // [0] 1 == interrupt,   0 == normal;      [15:1]  undefined
#define DIR_ADDR             0x0004   // [0] 1 == On,          0 == off;         [15:1]  undefined

#define OPTO_BASE_ADDR       0x0200   // opto 0 - 39
#define OPTO_0_DATA          OPTO_BASE_ADDR
#define OPTO_1_DATA          OPTO_BASE_ADDR + 0x008
#define OPTO_2_DATA          OPTO_BASE_ADDR + 0x010
#define OPTO_3_DATA          OPTO_BASE_ADDR + 0x018
#define OPTO_4_DATA          OPTO_BASE_ADDR + 0x020
#define OPTO_5_DATA          OPTO_BASE_ADDR + 0x028
#define OPTO_6_DATA          OPTO_BASE_ADDR + 0x030
#define OPTO_7_DATA          OPTO_BASE_ADDR + 0x038
#define OPTO_8_DATA          OPTO_BASE_ADDR + 0x040
#define OPTO_9_DATA          OPTO_BASE_ADDR + 0x048
#define OPTO_10_DATA         OPTO_BASE_ADDR + 0x050
#define OPTO_11_DATA         OPTO_BASE_ADDR + 0x058
#define OPTO_12_DATA         OPTO_BASE_ADDR + 0x060
#define OPTO_13_DATA         OPTO_BASE_ADDR + 0x068
#define OPTO_14_DATA         OPTO_BASE_ADDR + 0x070
#define OPTO_15_DATA         OPTO_BASE_ADDR + 0x078

#define MOTORSTAGING_DATA    OPTO_4_DATA
#define MOTORSTAGING_ENABLE  MOTORSTAGING_DATA + 1
#define MOTORSTAGING_ACTION  MOTORSTAGING_DATA + 2
#define MOTORSTAGING_STATUS  MOTORSTAGING_DATA + 3
#define MOTORSTAGING_DIR     MOTORSTAGING_DATA + 4

#define AIRINSTALLED_DATA    OPTO_5_DATA
#define AIRINSTALLED_ENABLE  AIRINSTALLED_DATA + 1
#define AIRINSTALLED_ACTION  AIRINSTALLED_DATA + 2
#define AIRINSTALLED_STATUS  AIRINSTALLED_DATA + 3
#define AIRINSTALLED_DIR    AIRINSTALLED_DATA + 4

#define LOWAIR_DATA          OPTO_6_DATA
#define LOWAIR_ENABLE        LOWAIR_DATA + 1
#define LOWAIR_ACTION        LOWAIR_DATA + 2
#define LOWAIR_STATUS        LOWAIR_DATA + 3
#define LOWAIR_DIR           LOWAIR_DATA + 4

#define CONVYR_BYPASS_DATA   OPTO_7_DATA
#define CONVYR_BYPASS_ENABLE CONVYR_BYPASS_DATA + 1
#define CONVYR_BYPASS_ACTION CONVYR_BYPASS_DATA + 2
#define CONVYR_BYPASS_STATUS CONVYR_BYPASS_DATA + 3
#define CONVYR_BYPASS_DIR    CONVYR_BYPASS_DATA + 4

#define MOTOR_TRIGGER_DATA   OPTO_8_DATA
#define MOTOR_TRIGGER_ENABLE MOTOR_TRIGGER_DATA + 1
#define MOTOR_TRIGGER_ACTION MOTOR_TRIGGER_DATA + 2
#define MOTOR_TRIGGER_STATUS MOTOR_TRIGGER_DATA + 3
#define MOTOR_TRIGGER_DIR    MOTOR_TRIGGER_DATA + 4

#define OPTO_15_0_STATUS     0x027D   // [15:0] interrupt status, opto 15 thru 0;  RO

// opto 16-31
#define OPTO_16_DATA         OPTO_BASE_ADDR + 0x080  // 0x280
#define OPTO_17_DATA         OPTO_BASE_ADDR + 0x088  // 0x288
#define OPTO_18_DATA         OPTO_BASE_ADDR + 0x090  // 0x290
#define OPTO_19_DATA         OPTO_BASE_ADDR + 0x098  // 0x298
#define OPTO_20_DATA         OPTO_BASE_ADDR + 0x0A0  // 0x2A0
#define OPTO_21_DATA         OPTO_BASE_ADDR + 0x0A8  // 0x2A8
#define OPTO_22_DATA         OPTO_BASE_ADDR + 0x0B0  // 0x2B0
#define OPTO_23_DATA         OPTO_BASE_ADDR + 0x0B8  // 0x2B8
#define OPTO_24_DATA         OPTO_BASE_ADDR + 0x0C0  // 0x2C0
#define OPTO_25_DATA         OPTO_BASE_ADDR + 0x0C8  // 0x2C8
#define OPTO_26_DATA         OPTO_BASE_ADDR + 0x0D0  // 0x2D0
#define OPTO_27_DATA         OPTO_BASE_ADDR + 0x0D8  // 0x2D8
#define OPTO_28_DATA         OPTO_BASE_ADDR + 0x0E0  // 0x2E0
#define OPTO_29_DATA         OPTO_BASE_ADDR + 0x0E8  // 0x2E8
#define OPTO_30_DATA         OPTO_BASE_ADDR + 0x0F0  // 0x2F0
#define OPTO_31_DATA         OPTO_BASE_ADDR + 0x0F8  // 0x2F8

//#define 
#define CONV_MOTOR_DIR     0x0280	// direction conveyor moves,        [0] 1 == L2R, 0 == R2L; [15:1]  undefined
#define CONV_MOTOR_ENABLE  0x0281	// conveyor motors enable           [0] 1 == ON,  0 == OFF; [15:1]  undefined
#define CONV_CLAMP_ENABLE  0x0282	// conveyor clamp motors enable     [0] 1 == ON,  0 == OFF; [15:1]  undefined
#define CONV_AW_ENABLE     0x0283	// conveyor AutoWidth motors enable [0] 1 == ON,  0 == OFF; [15:1]  undefined
#define CONV_LIFTER_ENABLE 0x0284	// conveyor center lifter enable    [0]  Lifter 1, [1] Lifter 2  == ON,  0 == OFF; [15:1]  undefined
#define CONV_SENSOR_POLTY  0x0285	// sensor trigger [0] 0 == on detection, 1 == on clear; [15:1]  undefined
#define CONV_RUN           0x0286   // tells conveyor to RUN, like run register (20)

// action (mutually exclusive, no internal checking)
/*
#define 0x0287    ramp		
#define 0x0288	  walk 
#define 0x0289	  set_step_walk  // walks set amount of steps.
#define 0x028A	  start_sensor   //currently unused
#define 0x028B    end_sensor  // {9h'0, autowidth_far, auto_home, extension, right, middle, stop, left}
#define 0x028C    max_velocity [15:0] 
#define 0x028D	  num_steps  [15:0]
#define 0x028E	   
#define 0x028F	  read sensors  //indicates which sensors are hit	   
#define 0x0290 	  dir_pos_passthrough  //decides direction in Passthrough mode
#define 0x0292	  max_velocity passthrough   
#define 0x0293 	  num_steps passthrough
#define 0x0294 	  up_avail_polarity_passthrough
#define 0x0295 	  dwnstrm_polarity_passthrough
*/

#define OPTO_31_16_STATUS    0x029D   // [15:0] interrupt status, opto 31 thru 16; RO [0x29D]
#define OPTO_39_32_STATUS    0x0340   // [15:0] interrupt status, opto 39 thru 32; RO [0x340]

#define CONTROL_EPROM_WRITE  0x0399
#define CONTROL_EPROM_ID     0x0400
#define CONTROL_EPROM_BD_MSB 0x0401
#define CONTROL_EPROM_BD_LSB 0x0402
#define CONTROL_EPROM_SN_MSB 0x0403
#define CONTROL_EPROM_SN_LSB 0x0404
#define CONTROL_EPROM_RV_PCA 0x0405
#define CONTROL_EPROM_RV_PCB 0x0406

//
//#define ADC

// other defines
#define MAX_IO               0x0100
#define LGHTNG_DEF_BASE_ADDR 0x2000
#define STROBE_DEF_BASE_ADDR 0x3000
#define LAST_LIGHTDEFADDR    0x27f8
#define LAST_STROBEDEFADDR   0x3bfc
#define LAST_MEMORYADDR      0xffff

#define	STRB_DEFN_SIZE	     0x0004     //8 bytes or 4 words
#define LIGHT_DEFN_SIZE      0x0008     //16 bytes or 8 words		
#define MAX_LIGHT_DEFNS      0x00ff     //leave the last one for temporary storage
#define MAX_STRB_DEFNS       0x02ff     //leave the last one for temporary storage
#define SIZE_CLOSE_TO_PRIME_NUMBER 1021	//if 300 light defitions are to be stored with four 
										//possible rotations, then we need 1200 bytes of storage

#define CONTROLLER_WATCHDOG_TIME (SHORT) 4000
#endif // !defined(_CONTROLLERADDRESSES_HPP__INCLUDED_)


