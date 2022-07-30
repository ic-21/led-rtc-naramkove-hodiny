/************************************************************************************/
/*                                                                                  */
/*  Projekt do predmetu IMP															*/
/*                                                                                  */
/*  ARM-KL05: LED náramkové hodinky na bázi RTC modulu                              */
/*                                                                                  */
/*  Autor: Ivana Colníková, xcolni00									            */
/*                                                                                  */
/************************************************************************************/

#include "MKL05Z4.h"

#define SELECT_INPUT_PIN(x)		(((1) << (x & 0x1Fu)))

//inicalizácia mikrokontroléru, zakladne nastavenie hodin a vypnutie watchdogu
void InitMCU()
{
	//nastavenie max. frekvencie a rozsahu casovaca
	MCG->C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );

	//zapnutie casovaca mcu
	SIM->CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);

	//vypnutie watchdogu
	SIM->COPC = SIM_COPC_COPT(0x00);
}

//inicializacia pinov, nastavenie vystupu a mux modu
void InitPIN()
{
	//zapnut casovac pre PORTA, PORTB
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

	//nastavenie MUX bitov na GPIO mód
	PORTA->PCR[8] =  PORT_PCR_MUX(1); 	//DS4 K
	PORTA->PCR[9] =  PORT_PCR_MUX(1); 	//DS3 K
	PORTA->PCR[10] = PORT_PCR_MUX(1); 	//DS2 K
	PORTA->PCR[11] = PORT_PCR_MUX(1);	//DS1 K

	PORTB->PCR[0] =  PORT_PCR_MUX(1);	//seg a
	PORTB->PCR[1] =  PORT_PCR_MUX(1); 	//seg b
	PORTB->PCR[2] =  PORT_PCR_MUX(1); 	//seg c
	PORTB->PCR[3] =  PORT_PCR_MUX(1); 	//seg DP

	PORTB->PCR[8] =  PORT_PCR_MUX(1); 	//seg d
	PORTB->PCR[9] =  PORT_PCR_MUX(1); 	//seg e
	PORTB->PCR[10] = PORT_PCR_MUX(1); 	//seg f
	PORTB->PCR[11] = PORT_PCR_MUX(1); 	//seg g

	//nastav GPIO piny ako vystupné
	GPIOA_PDDR |= GPIO_PDDR_PDD( 1 << 8 );
	GPIOA_PDDR |= GPIO_PDDR_PDD( 1 << 9 );
	GPIOA_PDDR |= GPIO_PDDR_PDD( 1 << 10 );
	GPIOA_PDDR |= GPIO_PDDR_PDD( 1 << 11 );

	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 0 );
	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 1 );
	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 2 );
	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 3 );

	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 8 );
	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 9 );
	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 10 );
	GPIOB_PDDR |= GPIO_PDDR_PDD( 1 << 11 );

	//implicitne ako vstup
	//SW1                  GPIO mod              prerusenie      zapnutie pullup    vyber pullup
	PORTB->PCR[4] = ( 0 | PORT_PCR_MUX(1) | PORT_PCR_IRQC(0xA) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK );
}

//inicializacia RTC periferie
void InitRTC()
{
	//zapnutie RTC casovaca
	SIM->SCGC6 |= SIM_SCGC6_RTC_MASK;

	//resetovanie registrov
	RTC->CR  = RTC_CR_SWR_MASK;
	RTC->CR  &= ~RTC_CR_SWR_MASK;

	//nastavenie preruseni
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_EnableIRQ(RTC_IRQn);

	//zapnutie oscilatora
	RTC->CR |= RTC_CR_OSCE_MASK;

	//cakanie na stabilizaciu krystalu...
	for(unsigned int i = 0; i < 0x600000; i++ ) { }

	//nastavenie pripadnej kompenzacie
	RTC->TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);

	//nastavenie hodnoty registra pocitadla sekund a alarmu
	RTC->TSR = 0;
	RTC->TAR = 0;

	//nastavenie vyvolania prerusenia
	RTC->IER |= RTC_IER_TAIE_MASK;

	//zapnutie RTC casovaca
	RTC->SR |= RTC_SR_TCE_MASK;
}

//funkcia na vytvorenie omeskania
void delay(int bound)
{
	//prazdny cyklus
	for(int i=0; i<bound; i++);
}

/* ******* DEFINICIE ******** */
#define POS4 0x00000E00	// DSK 4
#define POS3 0x00000D00	// DSK 3
#define POS2 0x00000B00	// DSK 2
#define POS1 0x00000700	// DSK 1
#define POSX 0x00000F00 // OFF

#define DOT   0x000000008   // .
#define NUM_0 0x000000707	// 0
#define NUM_1 0x000000006	// 1
#define NUM_2 0x000000B03	// 2
#define NUM_3 0x000000907	// 3
#define NUM_4 0x000000C06	// 4
#define NUM_5 0x000000D05	// 5
#define NUM_6 0x000000F05	// 6
#define NUM_7 0x000000007	// 7
#define NUM_8 0x000000F07	// 8
#define NUM_9 0x000000D07	// 9

//enum pozicii
__uint32_t positions[4] = {
		POS1, POS2, POS3, POS4
};

//enum cisiel na displeji
__uint32_t numbers[10] = {
    NUM_0, NUM_1, NUM_2, NUM_3, NUM_4,
	NUM_5, NUM_6, NUM_7, NUM_8, NUM_9
};

//stav zariadenia
__uint8_t STATE = 0;

//ako dlho bude cas zobrazeny na hodinkach
__uint8_t TIMEOUT = 15;

//cakanie a povolenie na stlacenie
__uint8_t CAN_PRESS = 1;
__uint8_t ALLOW_PRESS = 0;

//reprezentacia hodiniek ( H h : M m )
short unsigned int H = 0, h = 0, M = 0, m = 0;

//zvys hodnoty hodin
void get_time()
{
	//inkrementuj minuty a sleduj ci presli cez 9
	m = m + 1;
	if ( m == 10 ) {
		//ak ano tak nastav na nulu
		m = 0;
	}

	//ak sme presli cez 9 -> 10
	if ( m == 0 ) {
		//inkrementuj hodnotu MINUT
		M = M + 1;
	}

	//ak sme presli cez 59 -> 60
	if ( M == 6 ) {
		//nastav MINUTY na 0
		M = 0;

		//inkremetuj hodiny
		h = h + 1;

		//ak sme presli z 9 -> 10
		if ( h == 10 ) {
			//nastav hodiny na 0
			h = 0;
		}

		//presla hodina 9 -> 10, 19 -> 20
		if ( ( H == 0 || H == 1 ) && h == 0 ) {
			//inkrementuj HODINY
			H = H + 1;
		}

		//presiel jeden cely den 23 -> 00
		if ( h == 4 && H == 2) {
			//nastav cas na 00:00
			H = 0;
			h = 0;
		}
	}
}

//zvys cas hodin, pri nastavovani hodin (STATE = 2)
void increment_hours()
{
	//inkrementuj
	h = h + 1;
	//ak sme presli 9 -> 10
	if ( h == 10 ) {
		//nastav hodiny na 0
		h = 0;
	}

	//ak sme presli 9 -> 10, 19 -> 20
	if (h == 0 && (H == 0 || H == 1)) {
		//inkrementuj HODINY
		H = H + 1;
	}

	// ak sme presli 23 -> 00
	if (h == 4 && H == 2) {
		//nastav cas na 00
		H = 0;
		h = 0;
	}
}

// zvys cas minut, pri nastavovani minut (STATE = 3)
void increment_minutes()
{
	//inkrementuj minuty
	m = m + 1;

	//ak sme presli cez 9 -> 10
	if ( m == 10 ) {
		//nastav minuty na 0
		m = 0;
	}

	// ak sme presli 9 -> 10
	if ( m == 0 ) {
		//inkrementuj MINUTY
		M = M + 1;
	}

	// ak sme presli 59 -> 00
	if ( M == 6 ) {
		//nastav cas na 00
		M = 0;
		m = 0;
	}
}

//pocitadlo uplynutych sekund
//kontrola ci uplynula minuta
__uint8_t SECONDS_PASSED = 0;

//obsluha prerusenia RTC
void RTC_IRQHandler()
{
	//prerusenie vyvolane overflow-om alebo neplatnou hodnotou
	if( ((RTC->SR & RTC_SR_TIF_MASK)== 0x01) || ((RTC->SR & RTC_SR_TOF_MASK) == 0x02))
	{
	   RTC->SR &= 0x07;  //vymaz TCE, aby sa dal nastavit TSR
	   RTC->TSR = 0x00000000; //vynuluj
	}
	//prerusenie vyvolane alarmom
	else if((RTC->SR & RTC_SR_TAF_MASK) == 0x04)
	{
	   //inkrementuj hodnotu alarmu
   	   RTC->TAR = RTC->TAR + 1;

   	   //ak bolo zatlacene tlacitko
   	   if ( CAN_PRESS == 0 ) {
   		   //inkrementuj pocitadlo pre timeout na tlacitku
   		   ALLOW_PRESS = ALLOW_PRESS + 1;

   		   //ak presli (cca) 2 sekundy
   		   if ( ALLOW_PRESS == 2 ) {
   			   //znova povol stlacenie
   			   CAN_PRESS = 1;
   			   ALLOW_PRESS = 0;
   		   }
   	   }

   	   //sleep mode
   	   if ( STATE == 0 )
   	   {
   		   //inkrementuj pocitadlo sekund
   		   SECONDS_PASSED = SECONDS_PASSED + 1;

   		   //ak preslo 60 sekund
   		   if ( SECONDS_PASSED == 60 )
   		   {
   			   //zvys cas
   			   get_time();
   			   //vynuluj pocitadlo sekund
   			   SECONDS_PASSED = 0;
   		   }
   	   }

   	   //display
   	   if ( STATE == 1 )
   	   {
   		   //inkrementuj pocitadlo sekund
		   SECONDS_PASSED = SECONDS_PASSED + 1;

		   //ak preslo 60 sekund
		   if ( SECONDS_PASSED == 60 )
		   {
			   //zvys cas
			   get_time();
			   //vynuluj pocitadlo sekund
			   SECONDS_PASSED = 0;
		   }

		   //dekrementuj timeout
		   TIMEOUT = TIMEOUT - 1;
   	   }

   	   //nastavujem hodiny
   	   if ( STATE == 2 )
   	   {
   		   //inkrementuj hodiny
   		   increment_hours();
   	   }

   	   //nastavujem minuty
   	   if ( STATE == 3 )
   	   {
   		   //inkrementuj minuty
   		   increment_minutes();
   	   }
   }
}

//uved mikrokontroler do rezimu spanku (LLS)
void LLS_SleepMode()
{
	//vypnutie clock monitoru pre spravny prechod do LLS
	MCG_C6 &= ~MCG_C6_CME0_MASK;

	//povol PTB4 ako zdroj pre LLWU prerusenie
	LLWU->PE2 = LLWU_PE2_WUPE6(2);

	//povol prerusenie od RTC Alarmu
	LLWU->ME = 0x20;
	//NVIC_EnableIRQ(LLW_IRQn);

   volatile unsigned int r;

   //povolenie presunu do LLS
   SMC_PMPROT = SMC_PMPROT_ALLS_MASK;

   //nastavenie aby sa preslo do STOP modu
   SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
   SMC_PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;
   SMC_PMCTRL |= SMC_PMCTRL_STOPM(0x3);

   //pockat na zapis
   r = SMC_PMCTRL;
   r++;

   //cakaj na prerusenie skr LLWU
   __WFI();
}

//omeskanie
#define delay_time 15000

int main(void)
{
	//zakladna inicializacia MCU - hodiny, watchdog
	InitMCU();

	//inicializacia potrebnych I/O pinov
	InitPIN();

	//inicializacia RTC modulu
	InitRTC();

	//nekonecny cyklus
	for(;;) {

		//rozhodni o stave
		switch(STATE) {

			/* SLEEP */
			case 0: {

				//rezim spanku
				LLS_SleepMode();

				//ak sa stlacilo tlacitko
				if ( LLWU->F1 & 0x40 ) {

					//vymaz flagy
					LLWU->F1 |= 0xFF;

					//nastavenie preruseni
					NVIC_ClearPendingIRQ(RTC_IRQn);
					NVIC_EnableIRQ(RTC_IRQn);

					STATE = 1; //zmen stav na DISPLAY
					TIMEOUT = 10; //nastav timeout
					CAN_PRESS = 0;
				}

				break;
			}

			/* DISPLAY */
			case 1: {

				//zobraz cislo na pozicii 1 minuty
				GPIOA_PDOR = positions[3]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[m]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time);					//pockam

				//zobraz cislo na pozicii 2 MINUTY
				GPIOA_PDOR = positions[2]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[M]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				//zobraz cislo na pozicii 3 hodiny
				GPIOA_PDOR = positions[1]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = ( numbers[h] | DOT ); 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				//zobraz cislo na pozicii 4 HODINY
				GPIOA_PDOR = positions[0]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[H]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				//ak bolo stlacene tlacitko
				if ( ( (GPIOB_PDIR & GPIO_PDIR_PDI(SELECT_INPUT_PIN(4))) == 0 ) && ( CAN_PRESS == 1 ) )
				{
					STATE = 2; //nastavuj hodiny
					CAN_PRESS = 0; //zakaz stlacenie
				}

				/* --------- SLEEP MODE ---------*/
				//ak vyprsal timeout
				if ( TIMEOUT == 0 )
				{
					//vrat sa do stavu 0
					STATE = 0;
					//vypni display
					GPIOA_PDOR = POSX;
				}

				break;
			}

			/* HODINY */
			case 2: {

				//zobraz cislo na pozicii 3
				GPIOA_PDOR = positions[1]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[h]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				//zobraz cislo na pozicii 4
				GPIOA_PDOR = positions[0]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[H]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				// ak bolo stlacene tlacitko
				if ( ( (GPIOB_PDIR & GPIO_PDIR_PDI(SELECT_INPUT_PIN(4))) == 0 ) && ( CAN_PRESS == 1 ) )
				{
					STATE = 3; //nastavuj minuty
					CAN_PRESS = 0; //zakam stlacenie
				}

				break;
			}

			/* MINUTY */
			case 3: {

				//zobraz cislo na pozicii 1
				GPIOA_PDOR = positions[3]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[m]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				//zobraz cislo na pozicii 2
				GPIOA_PDOR = positions[2]; 	//nastavim si, ktoru poziciu chcem nastavovat
				GPIOB_PDOR = numbers[M]; 	//nastavim si, ktore cislo chcem zobrazit
				delay(delay_time); //pockam

				//ak bolo stlacene tlacitko
				if ( ( (GPIOB_PDIR & GPIO_PDIR_PDI(SELECT_INPUT_PIN(4))) == 0 ) && ( CAN_PRESS == 1 ) )
				{
					STATE = 1; //vrat sa do stavu DISPLAY
					TIMEOUT = 15; //nastav timeout
					CAN_PRESS = 0; //zakaz stlacenie
					SECONDS_PASSED = 0;
				}

				break;
			}

			// ------
			default:
				break;
		}
	}
}
