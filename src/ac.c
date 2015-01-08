#include <reg51.h>
// #include <intrins.h>

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

#define CLOCKTIMERPERIOD    ( -250)
#define TIMETHREASHOULD 	( 100 * CLOCKSPEED)
#define TIMESTEP            5
#define SECUND( X)          (( X) / TIMESTEP)

sbit CAP            = P1^0;
sbit OPAMP          = P3^6;
sbit PINPUT         = P1^0;
sbit NINPUT         = P1^1;
sbit INFRAINPUT	    = P3^2;
sbit PUMPER         = P1^7;
sbit W4             = P1^6;
sbit FANLOW         = P1^5;
sbit FANMID         = P1^4;
sbit FANHIG         = P1^3;

sbit INDUCATOR1     = P3^0;
sbit INDUCATOR2     = P3^4;
sbit INDUCATOR3     = P3^5;

#define modNONE     0
#define modCOOL     1
#define modHEAT     2
#define modDRY      3
#define modFAN      4

#define fanmAUTO    0
#define fanmLOW     1
#define fanmMID     2
#define fanmHIG     3

WORD TimeCounter        = 0;
BYTE SecsCounter        = 0;
BYTE RemuteData[6];
WORD Timer              = SECUND( 0);
BYTE PumperDelay        = SECUND( 600);
BYTE PumperOffDelay     = SECUND( 0);
BYTE FanDelay           = SECUND( 60);
BYTE FanOffDelay        = SECUND( 0);
BYTE FanChangeDelay     = SECUND( 0);
BYTE W4Delay            = SECUND( 120);
BYTE W4OffDelay         = SECUND( 0);
BYTE Mode               = modCOOL;
BYTE FanMode            = fanmAUTO;
BOOL OnOff              = FALSE;
BOOL PumperOn           = FALSE;
BOOL W4On               = FALSE;
BOOL Continuous         = FALSE;
BYTE FanSpeed           = 0;

BYTE ReqTemp            = 20;
BYTE CurrentTemp        = 20;


BOOL ValidData = FALSE;
BOOL NewDataRescived = FALSE;
BOOL TimeAdvanceReq = FALSE;

TIMER0
{
    TimeCounter += 12;
    if ( TimeCounter >= 8000/* (4000 * CLOCKSPEED) */){
        TimeCounter -= 8000/* (4000 * CLOCKSPEED) */;
        SecsCounter ++;
        if ( SecsCounter >= TIMESTEP){
            SecsCounter = 0;
            TimeAdvanceReq = TRUE;
            }
        }
}

WORD MeasurePW( BOOL Value)
{
    TR1 = 0;
    TF1 = 0;
	TH1 = 0x00;
    TL1 = 0x00;
    TR1 = 1;
    if (Value == 1) while ((INFRAINPUT == 1) && (TF1 == 0));
    else while ((INFRAINPUT == 0) && (TF1 == 0));
    TR1 = 0;
    if (TF1 == 0){
        register WORD Width;
	    *( BYTE *)((( BYTE *)(&Width)) + 0) = TH1;
        *( BYTE *)((( BYTE *)(&Width)) + 1) = TL1;
        return Width;
        }
    else return (0xffff);
}

InfraRead0() interrupt 0 using 3
{
	register WORD PulseWidth;
	register BYTE I, J, Extra;

    INDUCATOR3 = 0;
    ValidData = FALSE;

    PulseWidth = MeasurePW( 0);
    if (PulseWidth == 0xffff) goto Finish;

    PulseWidth = MeasurePW( 1);
    if (PulseWidth == 0xffff) goto Finish;

	for ( I = 0; I < 6; I++){
        RemuteData[I] = 0x00;
        for ( J = 0; J < 8; J++) {
            PulseWidth = MeasurePW( 0);
            if (PulseWidth == 0xffff) goto Finish;
            PulseWidth = MeasurePW( 1);
            if (PulseWidth == 0xffff) goto Finish;
		    RemuteData[I] = RemuteData[I] >> 1;
		    if (PulseWidth > TIMETHREASHOULD) RemuteData[I] = RemuteData[I] | 0x80;
            }
		}

    Extra = 0x00;

    for ( J = 0; J < 4; J++) {
        PulseWidth = MeasurePW( 0);
        if (PulseWidth == 0xffff) goto Finish;
        PulseWidth = MeasurePW( 1);
        if (PulseWidth == 0xffff) goto Finish;
		Extra = Extra >> 1;
		if (PulseWidth > TIMETHREASHOULD) Extra = Extra | 0x80;
        }
    MeasurePW( 0);
    if (( Extra == 0x20) && (( RemuteData[5] & 0xf0) == 0x20) ){
        ValidData = TRUE;
        NewDataRescived = TRUE;
        };
Finish:
    INDUCATOR3 = 1;
    TF1 = 1;
	IE0 = 0;
}


void Delay( INT ATime)
{
	TF1 = 0;
	TH1 = *(BYTE *)(((BYTE *)(&ATime)) + 0);
    TL1 = *(BYTE *)(((BYTE *)(&ATime)) + 1);
    TR1 = 1;
	while (!TF1);
}

BYTE ReadTemperature_( VOID)
{
    WORD Time;
    CAP = 0;
    Delay( - 30000);
    Delay( - 30000);
    Delay( - 30000);
    Delay( - 30000);
    TF1 = 0;
	TH1 = 0x00;
    TL1 = 0x00;
    INDUCATOR1 = 0;
    CAP = 1;
    TR1 = 1;
    while ((OPAMP == 0) && (TF1 == 0));
    TR1 = 0;
    CAP = 0;
    INDUCATOR1 = 1;
    if ( TF1 == 0) {
	    *( BYTE *)((( BYTE *)(&Time)) + 0) = TH1;
        *( BYTE *)((( BYTE *)(&Time)) + 1) = TL1;
        Time = (( Time >> 7) & 0x00ff);
        return (*( BYTE *)((( BYTE *)(&Time)) + 1));
        }
    else return 0x00;
}

BYTE ReadTemperature( VOID)
{
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

VOID SwitchToOff( VOID)
{
    if (OnOff == TRUE){
        OnOff = FALSE;
        PumperOffDelay = SECUND( 10);
        W4OffDelay     = SECUND( 20);
        FanOffDelay    = SECUND( 30);
        }
}

VOID SwitchToOn( VOID)
{
    if (OnOff == FALSE){
        OnOff = TRUE;
        PumperOffDelay = SECUND( 10);
        W4OffDelay     = SECUND( 20);
        FanOffDelay    = SECUND( 30);
        }
}

VOID SwitchPumperOn( VOID)
{
    if ( PumperOn == FALSE){
        PumperOn = TRUE;
        PumperOffDelay = SECUND( 600);
        PUMPER = TRUE;
        }
}

VOID SwitchPumperOff( VOID)
{
    if ( PumperOn == TRUE){
        if ( W4OffDelay < SECUND( 120)) W4OffDelay = SECUND( 120);
        if ( FanOffDelay < SECUND( 300)) FanOffDelay = SECUND( 300);
        if ( PumperDelay < SECUND( 600)) PumperDelay = SECUND( 600);
        PumperOn = FALSE;
        PUMPER = FALSE;
        }
}

VOID SwitchFanSpeed( BYTE ASpeed)
{
    if ( FanSpeed != ASpeed){
        if ( ASpeed == 0) {
            if ( FanDelay < SECUND( 30)) FanDelay = SECUND( 30);
            }
        else {
            FanChangeDelay = SECUND( 60);
            if ( FanSpeed == 0){
                if ( FanOffDelay < SECUND( 60)) FanOffDelay = SECUND( 60);
                if ( W4Delay < SECUND( 120)) W4Delay = SECUND( 120);
                if ( PumperDelay < SECUND( 600)) PumperDelay = SECUND( 600);
                }
            }
        FanSpeed = ASpeed;
        FANMID = FALSE;
        FANHIG = FALSE;
        FANLOW = FALSE;
        switch ( FanSpeed) {
            case 1:
                FANLOW = TRUE;
                break;
            case 2:
                FANMID = TRUE;
                break;
            case 3:
                FANHIG = TRUE;
            }

        }
}

VOID SwitchW4On( VOID)
{
    if ( W4On == FALSE){
        if (PumperDelay < SECUND( 60)) PumperDelay = SECUND( 60);
        W4OffDelay = SECUND( 60);
        W4On = TRUE;
        W4 = TRUE;
        }
}

VOID SwitchW4Off( VOID)
{
    if ( W4On == TRUE){
        if ( FanOffDelay < SECUND( 300)) FanOffDelay = SECUND( 300);
        if ( W4Delay < SECUND( 30)) W4Delay = SECUND( 30);
        W4On = FALSE;
        W4 = FALSE;
        }
}

VOID TimeAdvance( VOID)
{
    if ( PumperDelay > SECUND( 0)) PumperDelay -= SECUND( TIMESTEP);
    if ( PumperOffDelay > SECUND( 0)) PumperOffDelay -= SECUND( TIMESTEP);
    if ( FanDelay > SECUND( 0)) FanDelay -= SECUND( TIMESTEP);
    if ( FanOffDelay > SECUND( 0)) FanOffDelay -= SECUND( TIMESTEP);
    if ( FanChangeDelay > SECUND( 0)) FanChangeDelay -= SECUND( TIMESTEP);
    if ( W4Delay > SECUND( 0)) W4Delay -= SECUND( TIMESTEP);
    if ( W4OffDelay > SECUND( 0)) W4OffDelay -= SECUND( TIMESTEP);
    if ( Timer > SECUND( 0)) Timer -= SECUND( TIMESTEP);
}

VOID FanProcess( BYTE ATempDelta, BOOL AContinuous)
{
   if ( FanDelay == SECUND( 0)) {
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
       }
   else SwitchFanSpeed( 0);
}

VOID Process( VOID)
{
    if ( OnOff) {
        switch ( Mode){
            case modHEAT:
                if (( CurrentTemp < ReqTemp) || (Continuous == TRUE)){
                    if (( PumperDelay == SECUND( 0)) && ( FanSpeed != 0) && ( W4On == TRUE)) SwitchPumperOn();
                    else SwitchPumperOff();

                    FanProcess( ReqTemp - CurrentTemp, Continuous);

                    if (( W4Delay == SECUND( 0)) && ( FanSpeed != 0)) SwitchW4On();
                    else SwitchW4Off();
                    }
                else {
                    if ( PumperOffDelay == SECUND( 0)) {
                        SwitchPumperOff();
                        if ( W4OffDelay == SECUND( 0)){
                            SwitchW4Off();
                            if ( FanOffDelay == SECUND( 0)) SwitchFanSpeed( 0);
                            }
                        }
                    }
                break;
            case modCOOL:
                if ( W4On == TRUE) SwitchW4Off();

                if (( CurrentTemp > ReqTemp) || (Continuous == TRUE)){
                    if (( PumperDelay == SECUND( 0)) && ( FanSpeed != 0)) SwitchPumperOn();
                    else SwitchPumperOff();

                    FanProcess( CurrentTemp - ReqTemp, Continuous);

                    }
                else {
                    if ( PumperOffDelay == SECUND( 0)) {
                        SwitchPumperOff();
                        if ( FanOffDelay == SECUND( 0)) SwitchFanSpeed( 0);
                        };
                    }
                break;
            case modFAN:
                if ( W4On == TRUE) SwitchW4Off();
                if ( PumperOn == TRUE) SwitchPumperOff();

                if (( CurrentTemp > ReqTemp) || (Continuous == TRUE)){
                    FanProcess( CurrentTemp - ReqTemp, Continuous);
                    }
                else {
                    if ( FanOffDelay == SECUND( 0)) SwitchFanSpeed( 0);
                    }
                break;
            case modDRY:
                break;
            }
        }
    else {
        if ( PumperOn){
            SwitchPumperOff();
            FanOffDelay = SECUND( 20);
            W4OffDelay = SECUND( 10);
            }

        if (( W4On) && (W4OffDelay == SECUND( 0))){
            SwitchW4Off();
            FanOffDelay = SECUND( 10);
            }

        if (( FanSpeed != 0) && ( FanOffDelay == SECUND( 0))) SwitchFanSpeed( 0);
        }
}

WORD _RetriveTime( BYTE *ATime)
{
    {
        register BYTE Hours = *(ATime + 1);
        register BYTE BTime = (Hours & 0x0f);
        if (( Hours & 0x10) != 0x00) BTime += 10;
        if (( Hours & 0x80) != 0x00) BTime += 12;
        {
            register BYTE Minutes = *ATime;
            Minutes += ( Minutes & 0x0f) + ((Minutes >> 4) * 10);
            return ( ((WORD)BTime * 60) + Minutes);
            }
        }
}

VOID ProcessRecivedData()
{
    if ((RemuteData[0] & 0x08) != 0x00) SwitchToOn();
    else SwitchToOff();

    {
        register BYTE NewMode;
        switch (RemuteData[0] & 0x03){
            case 0x00:
                NewMode = modCOOL;
                break;
            case 0x01:
                // Mode = modDRY;
                break;
            case 0x02:
                NewMode = modFAN;
                break;
            default:
                NewMode = modHEAT;
            }

        if ( NewMode != Mode){
            Mode = NewMode;
            PumperOffDelay = SECUND( 10);
            W4OffDelay     = SECUND( 20);
            FanOffDelay    = SECUND( 30);
            }
        }

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
               ReqTemp = Temp + 17;
           }
        }

    switch ( RemuteData[0] & 0x30){
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
    Timer = 24 * 60 + _RetriveTime( &RemuteData[3]) - _RetriveTime( &RemuteData[1]);
    if ( Timer > (24 * 60)) Timer -= (24 * 60);
    Timer *= SECUND( 60);

}

main()
{
    BYTE I;

    PUMPER = FALSE;
    W4 = FALSE;
    FANLOW = FALSE;
    FANMID = FALSE;
    FANHIG = FALSE;
    PINPUT = 1;
    NINPUT = 1;

    IE0 = 0;                /* clear External 0 interrupt request */
	IP = 0x03;              /* set high intrrupt priorery to timer0 and external interrupt0 */
	IT0 = 1;                /* Interrupt 0 by edge */
	TMOD = 0x12;            /* mode 2 for timer 0 (reload 8 Bit) , made 1 for timer 1 (16 Bit counter)*/
	TL0 = CLOCKTIMERPERIOD; /* set timer0 period */
	TH0 = CLOCKTIMERPERIOD; /* set timer0 reload period */
	TR0 = 1;                /* start timer0 */
	ET0 = 1;                /* enable timer 0 interrupt */
	EX0 = 1;                /* enable External 0 interrupt */
    EA = 1;                 /* global interrupt enable */

    /*
    for (I=0;I<5;I++){
        INDUCATOR1 = 0;
        Delay(- 30000);
        INDUCATOR1 = 1;
        Delay(- 30000);
        }
    Delay(- 30000);
    */

    while (TRUE) {
       if (( NewDataRescived) && ( ValidData)) {
           NewDataRescived = FALSE;
           ProcessRecivedData();
           Process();
           }
       INDUCATOR2 = ~ValidData;
       if ( TimeAdvanceReq) {
           TimeAdvanceReq = FALSE;
           CurrentTemp = ReadTemperature();
           CurrentTemp = 23;
           Process();
           TimeAdvance();
           // P1 = (((~( CurrentTemp)) << 2) | 0x03);
           };
       }
}