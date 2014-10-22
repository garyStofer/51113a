#include "main.h" 

int __cdecl
main(  int argc, char* argv[]    )
{
	BOOL stat =0;
	TCPcio pcio;		// The harware IO class
	USHORT result;
	int i;

	stat = pcio.PCIO_Open();
	if ( !stat )
	{
		    printf("Could not connect to AIO_IO driver, exitiung\n");
			return stat;
	}

	// we can  talk to the Hardware by discrete Read and Write IO_control calls or by memory mapped direct access/
	// IOCtrl method is a bit slower than the direct memory access, but has the 
	// benefit of being checked against range and valid file-handle in the call.

	pcio.Write_IOCtrl(PCIO_STRB_CTRL,A_STRB);
	pcio.Read_IOCtrl(PCIO_INT_SEL,&result);	// exp 0xb148
	pcio.Write_IOCtrl(PCIO_STRB_CTRL,D_STRB);		
	pcio.Read_IOCtrl(PCIO_INT_SEL,&result);	// exp 0xb108

#ifdef Force_a_Timer1_interrupt	
	// The following is a test to see that the interrupt in the  driver is enabled  handled .
	// Using discrete IO-control calls timer 1 is set to expire in 0x1122 uS. 
	// Then the timer register is read back to verify that we can read it, and then the timer is started
	// This should then trigger an interrupt in the driver code which in term clears the timer, so that it can be 
	// rewritten with a new value

	// This enables the IO adapter interrupt source which must be on for the ISR to capture the timer interrupt and reset the timer
	// The driver code enables interuptss as called back from the framework after the ISR  has been 
	// registered with the kernel ( EvtInterrupDisable() , ..Enable() )

	pcio.Write_IOCtrl(PCIO_INT_SEL,ADAPTER_INT_EN);
												

	pcio.Write_IOCtrl(TMR_BASE_LSW,0x1122);
	pcio.Read_IOCtrl(TMR_BASE_LSW,&result);		// exp 0x1122
	pcio.Write_IOCtrl(TMR1_CTRL,TMR_CTRL_RUN);	// should trigger an interrupt in driver and clear the timer again 
		
	do	 // this polls until the timer has elapsed and the interrupt has reset the timer 
	{
		pcio.Read_IOCtrl(TMR_BASE_LSW,&result);	// exp 0x0000
	}while (result!= 0);

	pcio.Write_IOCtrl(TMR_BASE_LSW,0x2211);		// This write only takes effect after the timer has been reset by the ISR in the driver
	pcio.Read_IOCtrl(TMR_BASE_LSW,&result);		// exp 0x2211
	pcio.Write_IOCtrl(TMR1_CTRL,TMR_CTRL_RESET);// Reset the timer again --
#endif
	// Resetting controller -- This can be a catch22 -- if the controller is hung up and not responding on the cable we can't reset it
	pcio.Write_IOCtrl(CONTROLLER_RESET,(USHORT) 0);		// This will execute reset in the controller and takes >100ms to complete
	Sleep(CONTROLLER_BOOT_TIME);		// !!!!Controller needs more than 100ms to complete reset --  during which time no commands are taken
	pcio.Write_IOCtrl(CONTROLLER_RESET,(USHORT) 1);		// This releases reset in controller 
	Sleep(100);
	
	// Now check the controller -- This is the first read out on the cable 
	pcio.Read_IOCtrl(CONTROL_TYPE, &result);		// expect 0x140 for wc51140
	printf("Connected controller: %x\n", result);

	if (result != 0x140 )
	{
		printf("Controller not found -- exiting\n");
		return 0;
	}

	pcio.Read_IOCtrl(CONTROL_FIRM_MAJOR, &result);	// expect 2
	printf("Controller Major version: %d\n",result);

	if ( result != 2 )
	{
		printf("Controller Major version is wrong, exiting");
		return 0;
	}

	pcio.Read_IOCtrl(CONTROL_FRIM_MINOR, &result);	// expect 0
	printf("Controller Minor version: %d\n",result);
	 
	// lightshow
	pcio.Write_IOCtrl(LT_DRIVERS, 3);	// 3 is on 0 = 0ff	power on to light tower
	pcio.Write_IOCtrl(LT_FLASH_DURATION, 0x200);	

	// Blink Lights
	for (i=400 ; i>=50; i = i/2)
	{
		pcio.Write_IOCtrl(LT_FLASH_DURATION, i);
		pcio.Write_IOCtrl(LT_RED,2); 		// light on blink
		pcio.Write_IOCtrl(LT_YELLOW,2); 	
		pcio.Write_IOCtrl(LT_GREEN,2); 		
		Sleep(1000);
	}

	pcio.Write_IOCtrl(LT_YELLOW,0);  
	pcio.Write_IOCtrl(LT_RED,0); 		// lights off
	pcio.Write_IOCtrl(LT_GREEN,0);
	// Sleep(2000);
		
#ifdef LIGHT_TOWER_TEST_noInterruptsRunning
	for (i=70; i>10;i--)
	{
		pcio.Write_IOCtrl(LT_RED,0);  
		pcio.Write_IOCtrl(LT_YELLOW,3);
		Sleep(i);
		pcio.Write_IOCtrl(LT_YELLOW,0);
		pcio.Write_IOCtrl(LT_GREEN,3); 		// light on
		Sleep(i);
		pcio.Write_IOCtrl(LT_GREEN,0);
		pcio.Write_IOCtrl(LT_RED,3);  
		Sleep(i);
	}

	pcio.Write_IOCtrl(LT_BEEPER1,0); 
	pcio.Write_IOCtrl(LT_BEEPER2,0); 

	pcio.Write_IOCtrl(LT_YELLOW,0);  
	pcio.Write_IOCtrl(LT_RED,0); 		// lights off
	pcio.Write_IOCtrl(LT_GREEN,0);
	Sleep(2000);



	//pcio.Write_IOCtrl(LT_BEEPER2,3);	// BEEPER on
	Sleep(50);
	pcio.Write_IOCtrl(LT_BEEPER2,0);	// Beeper off



	pcio.PCIO_Write(LT_YELLOW,0);  
	pcio.PCIO_Write(LT_RED,0); 		// All lights off
	pcio.PCIO_Write(LT_GREEN,0);
	// end of Lightshow without interrupts running
#endif 
	// by leaving a light on we can see if the WD_timeout hits -- Lights go out when controller reboots
	pcio.PCIO_Write(LT_GREEN,3);

	printf("Starting interrupt thread\n"); 
	
	// turn some lights back on after controller reboot
	pcio.PCIO_Write(LT_DRIVERS,3);	// 3 is on 0 = 0ff	power on to light tower 
	pcio.PCIO_Write(LT_YELLOW,3);  
	
	// Start a thread to handle controller interrupts -- 
	if ( pcio.StartInterruptThread() )
	{

		// this will take a while to run  
		for (i=70; i>4;i--)
		{
			pcio.Write_IOCtrl(LT_RED,0);  // these two go via DeviceIOCtrl  -- mixing up mem-map and ioCtrl writes while interrupt thread is running
			pcio.Write_IOCtrl(LT_YELLOW,3);
			Sleep(i);
			pcio.PCIO_Write(LT_YELLOW,0);	// these via mem map 
			pcio.PCIO_Write(LT_GREEN,3); 	
			Sleep(i);
			pcio.PCIO_Write(LT_GREEN,0); // light off
			pcio.PCIO_Write(LT_RED,3);  // light on
			Sleep(i);

		}
		pcio.PauseSysWatchdog();
		printf("temporarily Pausing the Sys watchdog\n");
		
		// you can put a BP in the section of code below and the WC140 should not reboot when code execution is stopped
		for (; i<60;i++)
		{
			Sleep(i);
			pcio.Write_IOCtrl(LT_YELLOW,3);
			pcio.Write_IOCtrl(LT_RED,0);  // these two go via DeviceIOCtrl  
			
			Sleep(i);
			pcio.PCIO_Write(LT_GREEN,3); 		// light on
			pcio.PCIO_Write(LT_YELLOW,0);	// these via mem map 

			Sleep(i);
			pcio.PCIO_Write(LT_RED,3);
			pcio.PCIO_Write(LT_GREEN,0);
		}

	
		pcio.ContinueSysWatchdog();
		printf("Sys watchdog is re-enabled \n");
		// Putting a BP in the code below will timeout the WD again if the debugger stops execution of the code
		i =10;
		while (i--) // loop forever
		{
			pcio.Write_IOCtrl(LT_RED,0);  // these two go via DeviceIOCtrl  
			pcio.Write_IOCtrl(LT_YELLOW,3);
			Sleep(i);
			pcio.PCIO_Write(LT_YELLOW,0);	// these via mem map 
			pcio.PCIO_Write(LT_GREEN,3); 		// light on
			Sleep(i);
			pcio.PCIO_Write(LT_GREEN,0);
			pcio.PCIO_Write(LT_RED,3);  
			Sleep(i);
		}


		printf("Ending interrupt thread\n"); 
		pcio.KillAllThreads(0);		
		pcio.PCIO_Close();	

	}
	
	return stat;}
