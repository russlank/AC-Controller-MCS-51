#include <reg51.h>
// #include <intrins.h>

// #define _DEBUG_

#define CLOCKSPEED      8
// 8 MHz.
#define TIMER0          Timer0() interrupt 1 using 1
#define TIMER1          Timer1() interrupt 3 using 2
#define BOOL            bit
#define BYTE            unsigned char
#define WORD            unsigned int
#define DWORD           unsigned long int
#define SINT            char
#define INT             int
#define LINT            long int
#define TRUE            1
#define FALSE           0
#define FLOAT           float
#define VOID            void

#define modCOOL     0
#define modDRY      1
#define modFAN      2
#define modHEAT     3
#define modNONE     4

#define fanmAUTO    0
#define fanmLOW     1
#define fanmMID     2
#define fanmHIG     3

#define swmNONE     0
#define swmSLEEP    1
#define swmTIMER    2

#define CLOCKTIMERPERIOD    ( -250)
#define TIMESTEP            5
#define SECUND( X)          (( X) / TIMESTEP)
#define TIMETHREASHOULD     3
// ( 100 * CLOCKSPEED)
#define Beep()              BeepCounter = 255

code TEMP_TABLE[14] = { 9, 10, 10, 11, 11, 12, 13, 13, 14, 14, 15, 16, 16, 17 };

sbit OPAMP_OUTPUT = P3 ^ 6;
sbit INFRAINPUT = P3 ^ 2;
/*
BOOL PUMPER;
BOOL W4;
BOOL FANLOW;
BOOL FANMID;
BOOL FANHIG;
BOOL SWING;
*/
sbit FANLOW = P1 ^ 2;
sbit FANMID = P1 ^ 3;
sbit FANHIG = P1 ^ 4;
sbit PUMPER = P1 ^ 5;
sbit W4 = P1 ^ 6;
sbit SWING = P1 ^ 7;

sbit N_INPUT = P1 ^ 1;
sbit P_INPUT = P1 ^ 0;
sbit CAP = P1 ^ 0;
sbit SPKR = P3 ^ 3;

sbit ONOFF_INDUCATOR = P3 ^ 7;
sbit HEAT_INDUCATOR = P3 ^ 0;
sbit COOL_INDUCATOR = P3 ^ 1;
sbit TIMER_INDUCATOR = P3 ^ 5;

WORD TimeCounter = 0;
BYTE SecsCounter = 0;
WORD Timer = SECUND(0);
BYTE PumperDelay = SECUND(600);
BYTE PumperOffDelay = SECUND(0);
BYTE FanDelay = SECUND(60);
BYTE FanOffDelay = SECUND(0);
BYTE FanChangeDelay = SECUND(0);
BYTE W4Delay = SECUND(120);
BYTE W4OffDelay = SECUND(0);
BOOL SwingOn = FALSE;
BOOL OnOff = FALSE;
BOOL PumperOn = FALSE;
BOOL W4On = FALSE;
BOOL Continuous = FALSE;
BYTE Mode = modCOOL;
BYTE FanMode = fanmAUTO;
BYTE FanSpeed = 0;
SINT ReqTemp = 25;
SINT CurrentTemp = 20;
SINT TempDelta = -1;
BOOL NewDataRescived = FALSE;
BOOL TimeAdvanceReq = FALSE;
BYTE SwitchMode = swmNONE;
BYTE RemuteData[6];
BYTE BeepCounter = 0;

TIMER0
{
	TimeCounter += 12;
#ifdef _DEBUG_
	if (TimeCounter >= 4000){
		TimeCounter -= 4000;
#else
	if (TimeCounter >= (4000 * CLOCKSPEED)){
		TimeCounter -= (4000 * CLOCKSPEED);
#endif
		SecsCounter++;
		if (SecsCounter >= TIMESTEP){
			SecsCounter = 0;
			TimeAdvanceReq = TRUE;
		}
	}

	if (BeepCounter > 0) {
		SPKR = ~SPKR;
		BeepCounter--;
	}
	else SPKR = 0;
	}

		BYTE MeasurePW(BOOL Value)
	{
		TR1 = 0; // Stop timer if it is running
		TF1 = 0; // Clear timer overflow flag
		TH1 = 0x00; // Load timer high order value
		TL1 = 0x00; // Load timer low order value
		TR1 = 1; // Start timer
		if (Value == 1)
			while ((INFRAINPUT == 1) && (TF1 == 0)); // Wait until pulse input becames 0, or timer overflows
		else
			while ((INFRAINPUT == 0) && (TF1 == 0)); // Wait until pulse input becames 1, or timer overflows
		TR1 = 0; // Stop timer to read its value
		if (TF1 == 0) // If timer hasn`t overflowed
			return TH1; // Return the high order timer value
		else
			return (0xff); // Return (0xff) indicating that overflow has ocured
	}

	InfraRead0() interrupt 0 using 3
	{
		register BYTE PulseWidth;
		register BYTE I, J, Extra;

		NewDataRescived = FALSE;

		PulseWidth = MeasurePW(0);
		if (PulseWidth == 0xff) goto Finish;

		PulseWidth = MeasurePW(1);
		if (PulseWidth == 0xff) goto Finish;

		for (I = 0; I < 6; I++){ // Loop for 6 bytes of data
			RemuteData[I] = 0x00; // Reset the data byte
			for (J = 0; J < 8; J++) { // Loop for 8 bits for each byte of data
				PulseWidth = MeasurePW(0); // Wait until 0 finished
				if (PulseWidth == 0xff) goto Finish; // If 0 has taken long time skip infra data input
				PulseWidth = MeasurePW(1); // Measure 1 pulse time
				if (PulseWidth == 0xff) goto Finish; // If 1 has taken long time skip infra data input
				RemuteData[I] = RemuteData[I] >> 1; // Get room for new bit of data
				if (PulseWidth > TIMETHREASHOULD) RemuteData[I] = RemuteData[I] | 0x80; // Set bit value according to 1 pulse width
			}
		}

		Extra = 0x00; // Reset the verify data byte

		for (J = 0; J < 4; J++) { // Loop for 4 bits of verifying data
			PulseWidth = MeasurePW(0); // Wait until 0 finished
			if (PulseWidth == 0xff) goto Finish; // If 0 has taken long time skip infra data input
			PulseWidth = MeasurePW(1); // Measure 1 pulse time
			if (PulseWidth == 0xff) goto Finish; // If 1 has taken long time skip infra data input
			Extra = Extra >> 1; // Get room for new bit of data
			if (PulseWidth > TIMETHREASHOULD) Extra = Extra | 0x80; // Set bit value according to 1 pulse width
		}
		MeasurePW(0);  // Wait until 0 finished

		if ((Extra == 0x20) && ((RemuteData[5] & 0xf0) == 0x20)){ // Verify the data integrity
			NewDataRescived = TRUE; // Set ~NewDataRescived~ flag to true indicating to new infra data recived
		};
	Finish:
		TF1 = 1; // Set timer overflow flag on to protect another time measurments actions from blocking
		IE0 = 0; // Clear another interrupt requests that occured by series of pulses
	}

#ifdef _DEBUG_
	/*
	void Delay( INT ATime)
	{
	TF1 = 0;
	TH1 = *(BYTE *)(((BYTE *)(&ATime)) + 0);
	TL1 = *(BYTE *)(((BYTE *)(&ATime)) + 1);
	TR1 = 1;
	while (!TF1);
	}
	*/
#endif


	SINT ReadTemperature_(VOID)
	{
		WORD Time;
		CAP = 0; // Start capacitor discharging
		TF1 = 0; // Clear timer overflow flag
		TH1 = 0x00; // Load timer high order value
		TL1 = 0x00; // Load timer low order value
		TR1 = 1; // Start timer
		while (TF1 == 0); // Wait until overfloaw
		TR1 = 0; // Stop timer ???
		TF1 = 0; // Clear timer overflow flag
		TH1 = 0x00; // Load timer high order value
		TL1 = 0x00; // Load timer low order value
		/*
		#ifdef _DEBUG_
		INDUCATOR1 = 0; // Switch on temperature measurment inducator
		#endif
		*/
		CAP = 1; // Start capacitor charging
		TR1 = 1; // Start timer to measure charging time
		while ((OPAMP_OUTPUT == 0) && (TF1 == 0)); // Wait until capacitor
		// voltage reachs mesured voltage or timer overflows
		TR1 = 0; // Stop timer to read its value
		CAP = 0; // Start capacitor discharging
		/*
		#ifdef _DEBUG_
		INDUCATOR1 = 1; // Switch off temperature measurment inducator
		#endif
		*/
		if (TF1 == 0) { // If timer overflow hasn`t ocured
			*(BYTE *)(((BYTE *)(&Time)) + 0) = TH1; // Get timer high order value
			*(BYTE *)(((BYTE *)(&Time)) + 1) = TL1; // Get timer low order value
			Time = ((Time >> 7) & 0x00ff); // Correct the 16 bit value to retrive 8 bit value only
			return (*(BYTE *)(((BYTE *)(&Time)) + 1)); // Return 8 bit voltage value
		}
		else return 0x00; // If timer overflow has ocured
	}

	SINT ReadTemperature(VOID)
	{
		// Read temperature three times and take the larger value
		BYTE T1 = ReadTemperature_();
		BYTE T2 = ReadTemperature_();
		BYTE T3 = ReadTemperature_();
		if (T1 > T2) if (T1 > T3) return T1;
		else return T3;
		else if (T2 > T3) return T2;
		else return T3;
	}

	/*
	VOID DisplayData( VOID)
	{
	BYTE I, J;
	BYTE TempData;
	for ( I = 0; I < 6; I++){
	TempData = RemuteData[I];
	for ( J = 0; J < 8; J ++){
	if (TempData & 0x01) INDUCATOR3 = FALSE;
	else INDUCATOR3 = TRUE;
	INDUCATOR2 = FALSE;
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	INDUCATOR2 = TRUE;
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	Delay( -30000);
	TempData = TempData >> 1;
	}
	}
	}
	*/

	/*
	VOID SwitchToHot( VOID)
	{
	if (Mode != modHEAT){
	Mode = modHEAT;
	PumperOffDelay = SECUND( 10);
	W4OffDelay     = SECUND( 20);
	FanOffDelay    = SECUND( 30);
	}
	}
	*/

	/*
	VOID SwitchToCold( VOID)
	{
	if (Mode != modCOOL){
	Mode = modCOOL;
	PumperOffDelay = SECUND( 10);
	W4OffDelay     = SECUND( 20);
	FanOffDelay    = SECUND( 30);
	}
	}
	*/

	/*
	VOID SwitchToFan( VOID)
	{
	if (Mode != modFAN){
	Mode = modFAN;
	PumperOffDelay = SECUND( 10);
	W4OffDelay     = SECUND( 20);
	FanOffDelay    = SECUND( 30);
	}
	}
	*/

	VOID _SetOffDelays(VOID)
	{
		PumperOffDelay = SECUND(TIMESTEP) * 1;
		W4OffDelay = SECUND(TIMESTEP) * 2;
		FanOffDelay = SECUND(TIMESTEP) * 3;
	}

	VOID SwitchToOff(VOID)
	{
		if (OnOff == TRUE){
			OnOff = FALSE;
			_SetOffDelays();
		}
	}

	VOID SwitchToOn(VOID)
	{
		if (OnOff == FALSE){
			OnOff = TRUE;
			_SetOffDelays();
		}
	}

	VOID SwitchPumperOn(VOID)
	{
		if (PumperOn == FALSE){
			PumperOn = TRUE;
			PumperOffDelay = SECUND(600);
			PUMPER = TRUE;
		}
	}

	VOID SwitchPumperOff(VOID)
	{
		if (PumperOn == TRUE){
			if (W4OffDelay < SECUND(120)) W4OffDelay = SECUND(120);
			if (FanOffDelay < SECUND(300)) FanOffDelay = SECUND(300);
			if (PumperDelay < SECUND(600)) PumperDelay = SECUND(600);
			PumperOn = FALSE;
			PUMPER = FALSE;
		}
	}

	VOID SwitchFanSpeed(BYTE ASpeed)
	{
		if (FanSpeed != ASpeed){
			if (ASpeed == 0) {
				if (FanDelay < SECUND(30)) FanDelay = SECUND(30);
				SWING = FALSE;
			}
			else {
				FanChangeDelay = SECUND(60);
				if (FanSpeed == 0){
					if (FanOffDelay < SECUND(60)) FanOffDelay = SECUND(60);
					if (W4Delay < SECUND(120)) W4Delay = SECUND(120);
					if (PumperDelay < SECUND(600)) PumperDelay = SECUND(600);
				}
			}
			FanSpeed = ASpeed;
			FANMID = FALSE;
			FANHIG = FALSE;
			FANLOW = FALSE;
			switch (FanSpeed) {
			case fanmLOW:
				FANLOW = TRUE;
				break;
			case fanmMID:
				FANMID = TRUE;
				break;
			case fanmHIG:
				FANHIG = TRUE;
				break;
			}
		}
	}

	VOID SwitchW4On(VOID)
	{
		if (W4On == FALSE){
			if (PumperDelay < SECUND(60)) PumperDelay = SECUND(60);
			W4OffDelay = SECUND(60);
			W4On = TRUE;
			W4 = TRUE;
		}
	}

	VOID SwitchW4Off(VOID)
	{
		if (W4On == TRUE){
			if (FanOffDelay < SECUND(300)) FanOffDelay = SECUND(300);
			if (W4Delay < SECUND(30)) W4Delay = SECUND(30);
			W4On = FALSE;
			W4 = FALSE;
		}
	}

	VOID TimeAdvance(VOID)
	{
		PumperDelay = 0;
		PumperOffDelay = 0;
		FanDelay = 0;
		FanOffDelay = 0;
		FanChangeDelay = 0;
		W4Delay = 0;
		W4OffDelay = 0;
		/*
		if ( PumperDelay > SECUND( 0)) PumperDelay -= SECUND( TIMESTEP);
		if ( PumperOffDelay > SECUND( 0)) PumperOffDelay -= SECUND( TIMESTEP);
		if ( FanDelay > SECUND( 0)) FanDelay -= SECUND( TIMESTEP);
		if ( FanOffDelay > SECUND( 0)) FanOffDelay -= SECUND( TIMESTEP);
		if ( FanChangeDelay > SECUND( 0)) FanChangeDelay -= SECUND( TIMESTEP);
		if ( W4Delay > SECUND( 0)) W4Delay -= SECUND( TIMESTEP);
		if ( W4OffDelay > SECUND( 0)) W4OffDelay -= SECUND( TIMESTEP);
		if ( Timer > SECUND( 0)) Timer -= SECUND( TIMESTEP);
		*/
	}

	VOID FanProcess(BYTE ATempDelta, BOOL AContinuous)
	{
		if (FanDelay == SECUND(0)) {
			if (FanChangeDelay == SECUND(0)) {
				if (FanMode == fanmAUTO) {
					if (AContinuous == TRUE) SwitchFanSpeed(fanmMID);
					else if (ATempDelta > 6) SwitchFanSpeed(fanmHIG);
					else if (ATempDelta > 3) SwitchFanSpeed(fanmMID);
					else SwitchFanSpeed(fanmLOW);
				}
				else SwitchFanSpeed(FanMode);
			}
			SWING = SwingOn;
			/*
			switch ( FanMode) {
			case fanmLOW:
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 1);
			break;
			case fanmMID:
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 2);
			break;
			case fanmHIG:
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 3);
			break;
			default:
			if (AContinuous == TRUE) {
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 2);
			}
			else if ( ATempDelta > 6) {
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 3);
			}
			else if ( ATempDelta > 3) {
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 2);
			}
			else {
			if ( FanChangeDelay == SECUND( 0)) SwitchFanSpeed( 1);
			}
			}
			*/
		}
		else SwitchFanSpeed(0);
	}

	VOID Process(VOID)
	{
		BOOL OnOffState;

		TIMER_INDUCATOR = TRUE;
		if (OnOff) {
			ONOFF_INDUCATOR = FALSE;
			OnOffState = TRUE;
			switch (SwitchMode){
			case swmSLEEP:
				if (Timer == 0) OnOffState = FALSE;
				break;
			case swmTIMER:
				if (Timer != 0) {
					OnOffState = FALSE;
					TIMER_INDUCATOR = FALSE;
				}
				break;
			}
		}
		else {
			ONOFF_INDUCATOR = TRUE;
			HEAT_INDUCATOR = TRUE;
			COOL_INDUCATOR = TRUE;
			OnOffState = FALSE;
		}

		if (OnOffState) {
			switch (Mode){
			case modHEAT:
				HEAT_INDUCATOR = FALSE;
				COOL_INDUCATOR = TRUE;
				if ((CurrentTemp < (ReqTemp + TempDelta)) || (Continuous == TRUE)){
					TempDelta = 1;
					if ((PumperDelay == SECUND(0)) && (FanSpeed != 0) && (W4On == TRUE)) SwitchPumperOn();
					else SwitchPumperOff();
					FanProcess(ReqTemp - CurrentTemp, Continuous);
					if ((W4Delay == SECUND(0)) && (FanSpeed != 0)) SwitchW4On();
					else SwitchW4Off();
				}
				else {
					TempDelta = -1;
					if (PumperOffDelay == SECUND(0)) {
						SwitchPumperOff();
						if (W4OffDelay == SECUND(0)){
							SwitchW4Off();
							if (FanOffDelay == SECUND(0)) SwitchFanSpeed(0);
						}
					}
				}
				break;
			case modCOOL:
				HEAT_INDUCATOR = TRUE;
				COOL_INDUCATOR = FALSE;
				if (W4On == TRUE) SwitchW4Off();
				if ((CurrentTemp > (ReqTemp - TempDelta)) || (Continuous == TRUE)){
					TempDelta = 1;
					if ((PumperDelay == SECUND(0)) && (FanSpeed != 0)) SwitchPumperOn();
					else SwitchPumperOff();
					FanProcess(CurrentTemp - ReqTemp, Continuous);
				}
				else {
					TempDelta = -1;
					if (PumperOffDelay == SECUND(0)) {
						SwitchPumperOff();
						if (FanOffDelay == SECUND(0)) SwitchFanSpeed(0);
					};
				}
				break;
			case modDRY:
			case modFAN:
				HEAT_INDUCATOR = TRUE;
				COOL_INDUCATOR = TRUE;
				if (W4On == TRUE) SwitchW4Off();
				if (PumperOn == TRUE) SwitchPumperOff();
				if ((CurrentTemp > (ReqTemp - TempDelta)) || (Continuous == TRUE)){
					TempDelta = 1;
					FanProcess(CurrentTemp - ReqTemp, Continuous);
				}
				else {
					TempDelta = -1;
					if (FanOffDelay == SECUND(0)) SwitchFanSpeed(0);
				}
				break;
			}
		}
		else {
			if (PumperOn){
				SwitchPumperOff();
				FanOffDelay = SECUND(TIMESTEP) * 2;
				W4OffDelay = SECUND(TIMESTEP);
			}

			if ((W4On) && (W4OffDelay == SECUND(0))){
				SwitchW4Off();
				FanOffDelay = SECUND(TIMESTEP);
			}

			if ((FanSpeed != 0) && (FanOffDelay == SECUND(0))) SwitchFanSpeed(0);
		}
	}

	WORD _RetriveTime(BYTE *ATime)
	{
		{
			register BYTE Hours = *(ATime + 1);
			register BYTE BTime = (Hours & 0x0f);
			if ((Hours & 0x10) != 0x00) BTime += 10;
			if ((Hours & 0x80) != 0x00) BTime += 12;
			{
				register BYTE Minutes = *ATime;
				Minutes = (Minutes & 0x0f) + ((Minutes >> 4) * 10);
				return (((WORD)BTime * 60) + Minutes);
			}
		}
	}

	VOID ProcessRecivedData()
	{
		do {
			NewDataRescived = FALSE; // Clear the flag that new recived data not processed

			if ((RemuteData[0] & 0x08) != 0x00) SwitchToOn();
			else SwitchToOff();

			{
				register BYTE NewMode = RemuteData[0] & 0x03;
				/*
				switch (RemuteData[0] & 0x03){
				case 0x00:
				NewMode = modCOOL;
				break;
				case 0x01:
				Mode = modDRY;
				break;
				case 0x02:
				NewMode = modFAN;
				break;
				default:
				NewMode = modHEAT;
				}
				*/
				if (NewMode != Mode){
					Mode = NewMode;
					TempDelta = -1;
					_SetOffDelays();
				}
			}
			SwingOn = TRUE;
			if ((RemuteData[0] & 0xC0) == 0x00) SwingOn = FALSE;
			{
				register BYTE Temp = RemuteData[5] & 0x0f;
				switch (Temp){
				case 0x00:
				case 0x0E:
				case 0x0F:
					Continuous = TRUE;
					// ReqTemp = 0;
					break;
				default:
					Continuous = FALSE;
					ReqTemp = TEMP_TABLE[Temp];
				}
			}
			switch (RemuteData[0] & 0x30){
			case 0x10:
				FanMode = fanmHIG;
				break;
			case 0x20:
				FanMode = fanmMID;
				break;
			case 0x30:
				FanMode = fanmLOW;
				break;
			default:
				FanMode = fanmAUTO;
			}

			SwitchMode = (RemuteData[4] & 0x60) >> 5;

			if (SwitchMode == swmSLEEP) Timer = SECUND(60) * 30;
			else {
				Timer = 24 * 60 + _RetriveTime(&RemuteData[3]) - _RetriveTime(&RemuteData[1]);
				if (Timer > (24 * 60)) Timer -= (24 * 60);
				Timer *= SECUND(60);
			};

		} while (NewDataRescived == TRUE); // Repeat process new data if there is new data has been recived while processing

		Beep();
	}

	main()
	{
		PUMPER = FALSE;
		W4 = FALSE;
		FANLOW = FALSE;
		FANMID = FALSE;
		FANHIG = FALSE;
		SWING = FALSE;
		P_INPUT = 1;
		N_INPUT = 1;
		ONOFF_INDUCATOR = TRUE;
		HEAT_INDUCATOR = TRUE;
		COOL_INDUCATOR = TRUE;
		TIMER_INDUCATOR = TRUE;

		IE0 = 0; // clear External 0 interrupt request
		IP = 0x03; // set high intrrupt priorery to timer0 and external interrupt0
		IT0 = 1; // Interrupt 0 by edge
		TMOD = 0x12; // mode 2 for timer 0 (reload 8 Bit) , made 1 for timer 1 (16 Bit counter)
		TL0 = CLOCKTIMERPERIOD; // set timer0 period
		TH0 = CLOCKTIMERPERIOD; // set timer0 reload period
		TR0 = 1; // start timer0
		ET0 = 1; // enable timer 0 interrupt
		EX0 = 1; // enable External 0 interrupt
		EA = 1; // global interrupt enable

		Beep();

		while (TRUE) {
			if (NewDataRescived) {
				ProcessRecivedData();
				Process();
			}
			if (TimeAdvanceReq) {
				TimeAdvanceReq = FALSE;
				CurrentTemp = ReadTemperature();
				Process();
				TimeAdvance();
				// P1 = (((~( CurrentTemp)) << 2) | 0x03);
			};
		}
	}