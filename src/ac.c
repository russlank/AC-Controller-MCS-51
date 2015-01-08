#include <reg51.h>
// #include <intrins.h>

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

#define CLOCKTIMERPERIOD    (-250)
// #define MAXPULSETIME		0x1A * 4 * 3
// #define MAXLONGPULSETIME	10000 * 3
// #define TIMETHREASHOULD 	0x1A * 1
#define TIMETHREASHOULD 	400

sbit CAP           = P1^0;
sbit OPAMP         = P3^6;
sbit PINPUT        = P1^0;
sbit NINPUT        = P1^1;
sbit InfraInput	   = P3^2;

sbit INDUCATOR1     = P3^0;
sbit INDUCATOR2     = P3^4;
sbit INDUCATOR3     = P3^5;

#define modOFF      0
#define modCOLD     1
#define modHOT      2

WORD Time;
BYTE Data[6];

WORD PumperDelay        = 600;
WORD PumperOffDelay     = 0;
WORD FanDelay           = 60;
WORD FanOffDelay        = 0;
WORD FanChangeDelay     = 0;
WORD W4Delay            = 120;
WORD W4OffDelay         = 0;
BYTE Mode               = modOFF;

BOOL PumperOn = FALSE;
BOOL W4On = FALSE;
BYTE FanSpeed = 0;

BYTE ReqTemp = 20;
BYTE CurrentTemp = 20;

BOOL ValidData = FALSE;
BOOL NewDataRescived = FALSE;

TIMER0
{
    Time++;
    if ( PumperDelay > 0) PumperDelay--;
    if ( PumperOffDelay > 0) PumperOffDelay--;
    if ( FanDelay > 0) FanDelay--;
    if ( FanOffDelay > 0) FanOffDelay--;
    if ( FanChangeDelay > 0) FanChangeDelay--;
    if ( W4Delay > 0) W4Delay--;
    if ( W4OffDelay > 0) W4OffDelay--;
}
/*

InfraRead0() interrupt 0 using 3
{
	register WORD LastPulseWidth;
	register BYTE I, J, Extra;

    ValidData = FALSE;

	LastPulseWidth = 0;
	while ((InfraInput == 0) && (LastPulseWidth < MAXLONGPULSETIME)) LastPulseWidth++;
	if (LastPulseWidth >= MAXLONGPULSETIME) goto Finish;

	LastPulseWidth = 0;
	while ((InfraInput == 1) && (LastPulseWidth < MAXLONGPULSETIME)) LastPulseWidth++;
	if (LastPulseWidth >= MAXLONGPULSETIME) goto Finish;

	for ( I=0;I<6;I++){
        Data[I] = 0x00;
        for ( J=0;J<8;J++) {
		    LastPulseWidth = 0;
		    while ((InfraInput == 0) && (LastPulseWidth < MAXPULSETIME)) LastPulseWidth++;
		    if (LastPulseWidth >= MAXPULSETIME) goto Finish;
		    LastPulseWidth = 0;
		    while ((InfraInput == 1) && (LastPulseWidth < MAXPULSETIME)) LastPulseWidth++;
		    if (LastPulseWidth >= MAXPULSETIME) goto Finish;
		    Data[I] = Data[I] >> 1;
		    if (LastPulseWidth > TIMETHREASHOULD) Data[I] = Data[I] | 0x80;
            }
		}

    Extra = 0x00;

    for ( J=0;J<4;J++) {
	    LastPulseWidth = 0;
		while ((InfraInput == 0) && (LastPulseWidth < MAXPULSETIME)) LastPulseWidth++;
		if (LastPulseWidth >= MAXPULSETIME) goto Finish;
		LastPulseWidth = 0;
		while ((InfraInput == 1) && (LastPulseWidth < MAXPULSETIME)) LastPulseWidth++;
		if (LastPulseWidth >= MAXPULSETIME) goto Finish;
		Extra = Extra >> 1;
		if (LastPulseWidth > TIMETHREASHOULD) Extra = Extra | 0x80;
        }

	LastPulseWidth = 0;
	while ((InfraInput == 0) && (LastPulseWidth < MAXPULSETIME)) LastPulseWidth++;

    if (( Extra & 0xf0) == ( Data[5] & 0xf0)){
        ValidData = TRUE;
        NewDataRescived = TRUE;
        };
Finish:
	IE0 = 0;
}

*/


WORD MeasurePW( BOOL Value)
{
    TR1 = 0;
    TF1 = 0;
	TH1 = 0x00;
    TL1 = 0x00;

    TR1 = 1;

    if (Value == 1) while ((InfraInput == 1) && (TF1 == 0));
    else while ((InfraInput == 0) && (TF1 == 0));

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

	for ( I=0;I<6;I++){
        Data[I] = 0x00;
        for ( J=0; J<8; J++) {

            PulseWidth = MeasurePW( 0);
            if (PulseWidth == 0xffff) goto Finish;

            PulseWidth = MeasurePW( 1);
            if (PulseWidth == 0xffff) goto Finish;

		    Data[I] = Data[I] >> 1;
		    if (PulseWidth > TIMETHREASHOULD) Data[I] = Data[I] | 0x80;
            }
		}

    Extra = 0x00;

    for ( J=0;J<4;J++) {
        PulseWidth = MeasurePW( 0);
        if (PulseWidth == 0xffff) goto Finish;

        PulseWidth = MeasurePW( 1);
        if (PulseWidth == 0xffff) goto Finish;

		Extra = Extra >> 1;
		if (PulseWidth > TIMETHREASHOULD) Extra = Extra | 0x80;
        }

    MeasurePW( 0);

    if (( Extra & 0xf0) == ( Data[5] & 0xf0)){
        ValidData = TRUE;
        NewDataRescived = TRUE;
        };
    /*
    ValidData = TRUE;
    NewDataRescived = TRUE;
    */
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
        Time = (( Time >> 6) & 0x00ff);
        return (*( BYTE *)((( BYTE *)(&Time)) + 1));
        }
    else return 0xff;
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
        TempData = Data[I];
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

VOID SwitchToHot( VOID)
{
    if (Mode != modHOT){
        Mode = modHOT;
        PumperOffDelay = 10;
        W4OffDelay     = 20;
        FanOffDelay    = 30;
        }
}

VOID SwitchToCold( VOID)
{
    if (Mode != modCOLD){
        Mode = modCOLD;
        PumperOffDelay = 10;
        W4OffDelay     = 20;
        FanOffDelay    = 30;
        }
}

VOID SwitchToOff( VOID)
{
    if (Mode != modOFF){
        Mode = modOFF;
        PumperOffDelay = 10;
        W4OffDelay     = 20;
        FanOffDelay    = 30;
        }
}

VOID SwitchPumperOn( VOID)
{
    if (!PumperOn){
        PumperOn = TRUE;
        PumperOffDelay = 600;
        }
}

VOID SwitchPumperOff( VOID)
{
    if ( PumperOn){
        if ( W4OffDelay < 120) W4OffDelay = 120;
        if ( FanOffDelay < 300) FanOffDelay = 500;
        if ( PumperDelay < 600) PumperDelay = 600;
        PumperOn = FALSE;
        }
}

VOID SwitchFanOff( VOID)
{
    if ( FanSpeed != 0){
        if ( FanDelay < 30) FanDelay = 30;
        FanSpeed = 0;
        }
}

VOID SwitchFanOn( BYTE ASpeed)
{
    if ( FanSpeed != ASpeed){
        if ( ASpeed == 0) if ( FanDelay < 30) FanDelay = 30;
        else {
            FanChangeDelay = 60;
            if ( FanSpeed == 0){
                if ( FanOffDelay < 60) FanOffDelay = 60;
                if ( W4Delay < 30) W4Delay = 30;
                if ( PumperDelay < 600) PumperDelay = 600;
                }
            }
        FanSpeed = ASpeed;
        }
}

VOID SwitchW4On( VOID)
{
    if ( !W4On){
        if (PumperDelay < 60) PumperDelay = 60;
        W4OffDelay = 60;
        W4On = TRUE;
        }
}

VOID SwitchW4Off( VOID)
{
    if ( W4On){
        if ( FanOffDelay < 300) FanOffDelay = 300;
        if ( W4Delay < 30) W4Delay = 30;
        W4On = FALSE;
        }
}

VOID Process( VOID)
{
    switch ( Mode){
    case modHOT:
        if ( CurrentTemp < ReqTemp){
            if (( PumperDelay == 0) && ( FanSpeed != 0) && ( W4On)) SwitchPumperOn();
            else SwitchPumperOff();

            if ( FanDelay == 0){
                if (( ReqTemp - CurrentTemp) > 6){
                    if ( FanChangeDelay == 0) SwitchFanOn( 3);
                    }
                else if (( ReqTemp - CurrentTemp) > 3){
                    if ( FanChangeDelay == 0) SwitchFanOn( 2);
                    }
                else {
                    if ( FanChangeDelay == 0) SwitchFanOn( 1);
                    }
                }
            else SwitchFanOn( 0);

            if (( W4Delay == 0) && ( FanSpeed != 0)) SwitchW4On();
            else SwitchW4Off();
            }
        else {
            if ( PumperOffDelay == 0) SwitchPumperOff();
            if ( W4OffDelay == 0) SwitchW4Off();
            if ( FanOffDelay) SwitchFanOn( 0);
            }
        break;
    case modCOLD:
        if ( W4On) SwitchW4Off();
        if ( CurrentTemp > ReqTemp){
            if (( PumperDelay == 0) && ( FanSpeed != 0)) SwitchPumperOn();
            else SwitchPumperOff();

            if ( FanDelay == 0){
                if (( CurrentTemp - ReqTemp) > 6){
                    if ( FanChangeDelay == 0) SwitchFanOn( 3);
                    }
                else if (( CurrentTemp - ReqTemp) > 3){
                    if ( FanChangeDelay == 0) SwitchFanOn( 2);
                    }
                else {
                    if ( FanChangeDelay == 0) SwitchFanOn( 1);
                    }
                }
            else SwitchFanOn( 0);

            }
        else {
            if ( PumperOffDelay == 0) SwitchPumperOff();
            if ( FanOffDelay) SwitchFanOn( 0);
            }
        break;
    default:
        if ( PumperOn){
            SwitchPumperOff();
            FanOffDelay = 20;
            W4OffDelay = 10;
            }

        if (( W4On) && (W4OffDelay == 0)){
            SwitchW4Off();
            FanOffDelay = 10;
            }

        if (( FanSpeed != 0) && ( FanOffDelay == 0)) SwitchFanOn( 0);
    }
}

*/

main()
{
    BYTE I;

	IP = 0x03;              /* set high intrrupt priorery to timer0 and external interrupt0 */
	IT0 = 1;                /* Interrupt 0 by edge */
	TMOD = 0x12;            /* mode 2 for timer 0 (reload 8 Bit) , made 1 for timer 1 (16 Bit counter)*/
	TL0 = CLOCKTIMERPERIOD; /* set timer0 period */
	TH0 = CLOCKTIMERPERIOD; /* set timer0 reload period */
	TR0 = 0;//1                /* start timer0 */
	ET0 = 1;                /* enable timer 0 interrupt */
	EX0 = 1;//1                   /* enable External 0 interrupt */

    for (I=0;I<5;I++){
        INDUCATOR1 = 0;
        Delay(- 30000);
        INDUCATOR1 = 1;
        Delay(- 30000);
        }
    Delay(- 30000);

    IE0 = 0;
    EA = 1;                 /* global interrupt enable */

    PINPUT = 1;
    NINPUT = 1;

    while (TRUE) {
       P1 = (((~(ReadTemperature())) << 2) | 0x03);
       /*
       if ( NewDataRescived){
           DisplayData();
           NewDataRescived = FALSE;
           }
       */
       INDUCATOR2 = ~ValidData;
       // Process();
       }
}