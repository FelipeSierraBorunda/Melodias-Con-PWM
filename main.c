#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_event.h"
#include "soc/ledc_reg.h"
#include "stdio.h"
// Varaibles globales 
int nota=0;
int CantidadDeNotas=11;
int octava=0;
int YaCambioNota=0;
int frecuencia;
int NotaSonando=0;
uint32_t duracion;
uint32_t TiempoDeSilencio=350000; //350 ms
int NumeroNota=0;
int CantidadNotasMelodia =0;
int silencio;

uint32_t BitMask, ValorDesplazadoDivisor;
// Matriz de duraciones
uint32_t Duraciones[] = 
//	  .125 s	  	.25 s		  .5 s		   1 s
	{125000, 250000, 500000, 1000000};
// CORCHEA   NEGRA    BLANCA  REDONDA

// Matriz de segmentos para los dígitos 0-9     
uint32_t NotasyOctavas[][8] = 
{
    // 1°oct    2°oct    3°oct    4°oct    5°oct    6°oct    7°oct    8°oct
    {3270, 6541, 13081, 26163, 52325, 104650, 209300, 418601}, // Do
    {3465, 6930, 13859, 27718, 55437, 110873, 221746, 443492}, // Do#
    {3671, 7342, 14683, 29366, 58733, 117466, 234932, 469864}, // Re
    {3889, 7778, 15556, 31113, 62225, 124451, 248902, 497803}, // Re#
    {4120, 8241, 16481, 32963, 65925, 131851, 263702, 527404}, // Mi
    {4365, 8731, 17461, 34923, 69846, 139691, 279383, 558765}, // Fa
    {4625, 9250, 18500, 36999, 73999, 147998, 295996, 591991}, // Fa#
    {4900, 9800, 19600, 39200, 78399, 156798, 313596, 627193}, // Sol
    {5191, 10383, 20765, 41530, 83061, 166122, 332244, 664488}, // Sol#
    {5500, 11000, 22000, 44000, 88000, 176000, 352000, 704000}, // La (Referencia 440Hz)
    {5827, 11654, 23308, 46616, 93233, 186466, 372931, 745862}, // La#
    {6174, 12347, 24694, 49388, 98777, 197553, 395107, 790213}, // Si 
};
const char* NombresNotas[] = 
{
    "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si"
};
// Representación más legible de las notas y octavas

typedef enum {
    DO = 0, DO_S, RE, RE_S, MI, FA, FA_S, SOL, SOL_S, LA, LA_S, SI
} Nota;

uint32_t Melodia[][4] = 
{
 {DO, 3, 1, 0}, {DO, 3, 1, 0}, {SOL, 3, 1, 0}, {SOL, 3, 1, 0}, 
{LA, 3, 1, 0}, {LA, 3, 1, 0}, {SOL, 3, 2, 1}, // Octava 3

{FA, 3, 1, 0}, {FA, 3, 1, 0}, {MI, 3, 1, 0}, {MI, 3, 1, 0}, 
{RE, 3, 1, 0}, {RE, 3, 1, 0}, {DO, 3, 2, 0}, // Octava 3

{DO, 4, 1, 0}, {DO, 4, 1, 0}, {SOL, 4, 1, 0}, {SOL, 4, 1, 0}, 
{LA, 4, 1, 0}, {LA, 4, 1, 0}, {SOL, 4, 2, 1}, // Octava 4

{FA, 4, 1, 0}, {FA, 4, 1, 0}, {MI, 4, 1, 0}, {MI, 4, 1, 0}, 
{RE, 4, 1, 0}, {RE, 4, 1, 0}, {DO, 4, 2, 0}, // Octava 4

{DO, 5, 1, 0}, {DO, 5, 1, 0}, {SOL, 5, 1, 0}, {SOL, 5, 1, 0}, 
{LA, 5, 1, 0}, {LA, 5, 1, 0}, {SOL, 5, 2, 1}, // Octava 5

{FA, 5, 1, 0}, {FA, 5, 1, 0}, {MI, 5, 1, 0}, {MI, 5, 1, 0}, 
{RE, 5, 1, 0}, {RE, 5, 1, 0}, {DO, 5, 2, 0}, // Octava 5

{DO, 6, 1, 0}, {DO, 6, 1, 0}, {SOL, 6, 1, 0}, {SOL, 6, 1, 0}, 
{LA, 6, 1, 0}, {LA, 6, 1, 0}, {SOL, 6, 2, 1}, // Octava 6

{FA, 6, 1, 0}, {FA, 6, 1, 0}, {MI, 6, 1, 0}, {MI, 6, 1, 0}, 
{RE, 6, 1, 0}, {RE, 6, 1, 0}, {DO, 6, 2, 0}, // Octava 6

}; //[Nota,octava,DuracionNota,DuracionSilencio]


//================================================== Funciones  =====================================================
void AplicacionFrecuenciaDecimales(int frecuenciasolicitada)
{
	int divisor;
	int fclk = 80000000;
	int Res_Contador=12;
    int denominador;
	
	denominador=(frecuenciasolicitada/100 *(1 << Res_Contador));
	divisor = (fclk / denominador); 
 	
 	//int divisorentero = divisor/100;  // Parte entera
    //int divisordecimal = divisor % 100; 
	
	//Asignamos el registro 
	ValorDesplazadoDivisor = divisor << 13;
    BitMask = REG_READ(LEDC_HSTIMER0_CONF_REG); //Configuramos el registro del timer 0 del LEd (pag 408)
 	BitMask &= (~0x007FE000);  // 0x00000FC0 es la máscara que limpia los bits de 5 a 12  0x00001FE0
    BitMask |= ValorDesplazadoDivisor;
    REG_WRITE(LEDC_HSTIMER0_CONF_REG, BitMask);

}
void TodosLosArmonicos()
{
	if (nota<=CantidadDeNotas)
	{
		frecuencia = NotasyOctavas[nota][octava];
		ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, frecuencia);
		ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
		ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
		if (nota==CantidadDeNotas)
		{
			nota =0;
			if(octava<=6){octava=octava+1;}
			else{octava=0;}
		}
		else{
			nota++;
		}
	}  	
}
void CambiaLaNota()
{		
		CantidadNotasMelodia=(sizeof(Melodia) / sizeof(Melodia[0]))-1;
		if (NumeroNota<=CantidadNotasMelodia)
		{
		nota = Melodia[NumeroNota][0];
        octava = Melodia[NumeroNota][1];
        duracion = Duraciones[Melodia[NumeroNota][2]];
        silencio = Duraciones[Melodia[NumeroNota][3]];
		frecuencia = (NotasyOctavas[nota][octava]);
		//Frecuencia decimal
		AplicacionFrecuenciaDecimales(frecuencia);
		//Frecuecnia set
		//int frecuenciaentera = frecuencia / 100;  // Parte entera
		//ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, frecuenciaentera);
		//
		ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
		ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
			if (NumeroNota==CantidadNotasMelodia)
			{
				NumeroNota =0;
				TiempoDeSilencio=2000000;
			}
			else{
				TiempoDeSilencio=silencio;
				NumeroNota++;
			}
		}  

}
//================================================== Interrupciones =================================================
// ================================================= Cambio de nota
// ISR para detectar dirección de giro
static bool IRAM_ATTR CambioNota(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
	BaseType_t high_task_awoken = pdFALSE;
	//Reiniciamos el timer
	gptimer_set_raw_count(timer, 0);
	gptimer_stop(timer);
	//Condiciones
	if(NotaSonando==0)
	{
		CambiaLaNota();
		// Configuramos el ciclo de trabajo a un valor adecuado para la frecuencia deseada
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 2046); // 50% de ciclo de trabajo
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);  // Actualizamos el ciclo de trabajo
		// Indicamos que esta sonando
		NotaSonando=1;
		//Duracion de sonido
		gptimer_alarm_config_t Sonando = 
		{
        .alarm_count = duracion,	//Duracion de las notas
        .reload_count = 0,
        .flags.auto_reload_on_alarm = false
    	};
    	// Aplicar la nueva configuración de la alarma
   		gptimer_set_alarm_action(timer, &Sonando);
   		// Reiniciar el temporizador
   		gptimer_start(timer);

	}
	else
	{
		// Detener el canal de PWM completamente
   		 ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0); // Esto detiene la señal PWM y la configura en 0

    	//Paramos el pwm
		ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
		//Indicamos que laa nota no esta sonando
		NotaSonando=0;
		//configuramos el silencio
		gptimer_alarm_config_t silencio = 
		{
        .alarm_count = TiempoDeSilencio,	//Aqui se establece cuanto tiempo estara el silencio entre notas
        .reload_count = 0,
        .flags.auto_reload_on_alarm = false
    	};
    	// Aplicar la nueva configuración de la alarma
   		gptimer_set_alarm_action(timer, &silencio);
   		// Reiniciar el temporizador
   		gptimer_start(timer);
	}
	return (high_task_awoken == pdTRUE); 		
}
//==================================================== Main =======================================================
void app_main(void)
{
		// ====================================== Timers de proposito general ====================================
		// ====================================== Timers de activacion de cambio de nota
		// Configuracion general del timer
		gptimer_config_t TimerConfiguracionGeneral = 
		{
			.clk_src = GPTIMER_CLK_SRC_APB,			 
			.direction = GPTIMER_COUNT_UP, 			
			.resolution_hz = 1000000,				 
		};
		// Declaramos el manejador del gptimer 
		gptimer_handle_t TimerNota = NULL;		  	
		//Aplicamos Configuracion General
		gptimer_new_timer(&TimerConfiguracionGeneral, &TimerNota);  
		// Configuracion de la alarma
		gptimer_alarm_config_t AlarmaNota = 
		{
			.alarm_count = 3000000,
			.reload_count = 0,
			.flags.auto_reload_on_alarm = false
		};
		// Aplicamos la configuracion de la alarma para el manejador
		gptimer_set_alarm_action(TimerNota, &AlarmaNota);
		// Establecemos que la alarma del manejador, active la siguiente interrupcion
		gptimer_event_callbacks_t CambioDeNota = 
		{
			.on_alarm = CambioNota,
		};
		// Aplicamos el evento al manejador
		gptimer_register_event_callbacks(TimerNota, &CambioDeNota, NULL);
		// Habilitaos y empezamos el timer del display
		gptimer_enable(TimerNota);
		gptimer_start(TimerNota);
						

		// ====================================== Timers para PWM (configutacion) ============================
		// Configuracion del ledc timer
	    ledc_timer_config_t PWM_timer = 
	    {
	        .duty_resolution = LEDC_TIMER_12_BIT, 		// resolution of PWM duty (1024)
	        .freq_hz = 100,                      		// frequency of PWM signal
	        .speed_mode = LEDC_HIGH_SPEED_MODE,      	// timer mode
	        .timer_num = LEDC_TIMER_0,            		// timer index
	        .clk_cfg = LEDC_APB_CLK,             		// Auto select the source clock
	    };
	    // Configuracion del canal del timer (cada ledc timer tiene dos canales)
	    ledc_channel_config_t PWM_channel = 
	    {
	                .channel    = LEDC_CHANNEL_0,
	                .duty       = 2046,
	                .gpio_num   = 25,
	                .speed_mode = LEDC_HIGH_SPEED_MODE,
	                .hpoint     = 0,
	                .timer_sel  = LEDC_TIMER_0
	      };
	    // ======================================== Configuraciones de inicio ==================================
	    // Se aplican las configuraciones
	    ledc_timer_config(&PWM_timer);
	    ledc_channel_config(&PWM_channel);
	    // Al iniciarse, se pausa el timer
	    /// Detener el canal de PWM completamente
   		ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0); // Esto detiene la señal PWM y la configura en 0
	    ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
    	vTaskDelay(2750/ portTICK_PERIOD_MS);
		
	   int NotaActual=10; //numero aleaorio pero que no este enetre 0-6 (numero de notas)
	    while (true) 
	    {
			// Hacer algo
    		vTaskDelay(10 / portTICK_PERIOD_MS); 
    		if(NotaSonando==0){
				if(NotaActual!=nota)
				{	
					
					frecuencia = NotasyOctavas[nota][octava];			
					printf("Nota: %s, Octava: %d, Frecuencia: %lu Hz\n", NombresNotas[nota], octava +1 , (unsigned long)frecuencia/100);
					NotaActual=nota;
				}
			}
			
	    }
	}
