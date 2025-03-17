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
uint32_t frecuencia;
int NotaSonando=0;
uint32_t duracion;
uint32_t TiempoDeSilencio=350000; //350 ms
int NumeroNota=0;
int CantidadNotasMelodia =0;
// Matriz de duraciones
uint32_t Duraciones[] = 
//	  .125 s	  	.25 s		  .5 s		   1 s
	{125000, 250000, 500000, 1000000};
// CORCHEA   NEGRA    BLANCA  REDONDA
// N
// Matriz de segmentos para los dígitos 0-9
uint32_t NotasyOctavas[][8] = 
{
    	// 1°oct,  2°oct,   3°oct,     4°oct,    5°oct,    6°oct,    7°oct,      8°oct
    {32, 64, 128, 256, 512, 1024, 2048, 4096}, // Do
    {34, 68, 136, 272, 544, 1088, 2176, 4352}, // Do#
    {36, 72, 144, 288, 576, 1152, 2304, 4608}, // Re
    {37, 74, 148, 296, 592, 1184, 2368, 4736}, // Re#
    {38, 76, 152, 304, 608, 1216, 2432, 4864}, // Mi
    {39, 78, 156, 312, 624, 1248, 2496, 4992}, // Fa
    {41, 82, 164, 328, 656, 1312, 2624, 5248}, // Fa#
    {43, 86, 172, 344, 688, 1376, 2752, 5504}, // Sol
    {44, 88, 176, 352, 704, 1408, 2816, 5632}, // Sol#
    {46, 92, 184, 368, 736, 1472, 2944, 5888}, // La
   {47, 94, 188, 376, 752, 1504, 3008, 6016}, // La#
   {49, 98, 196, 392, 784, 1568, 3136, 6272}, // Si 
};
const char* NombresNotas[] = 
{
    "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si"
};
// Representación más legible de las notas y octavas

typedef enum {
    DO = 0, DO_S, RE, RE_S, MI, FA, FA_S, SOL, SOL_S, LA, LA_S, SI
} Nota;

uint32_t Melodia[][3] = 
{
    {DO,   4, 1},
    {RE,   4, 1},
    {MI,  4, 1},
    {DO,   4, 1},
    
    {DO,   4, 1},
    {RE,   4, 1},    
    {MI,   4, 1},  
    {DO,   4, 1},
    
    {MI,   4, 1},  
    {FA,   4, 1},
    {SOL,   4, 2},
    
     {MI,   4, 1},  
    {FA,   4, 1},
    {SOL,   4, 2},
    
    {SOL,   4, 0},
    {LA,   4, 0},    
    {SOL,   4, 0},  
    			{FA,   		4, 		0},
    {MI,   4, 1},
    {DO,   4, 1},
};



// Nombres de las notas para imprimir

//================================================== Funciones  =====================================================
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
		frecuencia = NotasyOctavas[nota][octava];
		ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, frecuencia);
		ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
		ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
			if (NumeroNota==CantidadNotasMelodia)
			{
				NumeroNota =0;
				TiempoDeSilencio=2000000;
			}
			else{
				TiempoDeSilencio=350000;
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
         ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 32); // 50% de ciclo de trabajo
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
	        .duty_resolution = LEDC_TIMER_6_BIT, 		// resolution of PWM duty (1024)
	        .freq_hz = 32,                      		// frequency of PWM signal
	        .speed_mode = LEDC_HIGH_SPEED_MODE,      	// timer mode
	        .timer_num = LEDC_TIMER_0,            		// timer index
	        .clk_cfg = LEDC_AUTO_CLK,             		// Auto select the source clock
	    };
	    // Configuracion del canal del timer (cada ledc timer tiene dos canales)
	    ledc_channel_config_t PWM_channel = 
	    {
	                .channel    = LEDC_CHANNEL_0,
	                .duty       = 32,
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
					printf("Nota: %s, Octava: %d, Frecuencia: %lu Hz\n", NombresNotas[nota], octava +1 , (unsigned long)frecuencia);
					NotaActual=nota;
				}
			}
			
	    }
	}
