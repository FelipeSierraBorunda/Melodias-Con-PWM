#include "driver/ledc.h"
#include "esp_event.h"
#include "soc/ledc_reg.h"

// Matriz de segmentos para los dígitos 0-9
uint32_t NotasyOctavas[][8] = 
{
    	// 1°oct,  2°oct,   3°oct,     4°oct,    5°oct,    6°oct,    7°oct,      8°oct
    {32, 64, 128, 256, 512, 1024, 2048, 4096}, // Do
    {34, 68, 136, 272, 544, 1088, 2176, 4352}, // Re
    {36, 72, 144, 288, 576, 1152, 2304, 4608}, // Mi
    {38, 76, 152, 304, 608, 1216, 2432, 4864}, // Fa
    {41, 82, 164, 328, 656, 1312, 2624, 5248}, // Sol
    {43, 86, 172, 344, 688, 1376, 2752, 5504}, // La
    {46, 92, 184, 368, 736, 1472, 2944, 5888}, // Si
};
// Nombres de las notas para imprimir
const char* NombresNotas[] = {"Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};
void app_main(void){
	    ledc_timer_config_t PWM_timer = {
	        .duty_resolution = LEDC_TIMER_6_BIT, 		// resolution of PWM duty (1024)
	        .freq_hz = 32,                      		// frequency of PWM signal
	        .speed_mode = LEDC_HIGH_SPEED_MODE,      	// timer mode
	        .timer_num = LEDC_TIMER_0,            		// timer index
	        .clk_cfg = LEDC_AUTO_CLK,             		// Auto select the source clock
	    };

	    ledc_channel_config_t PWM_channel = {
	                .channel    = LEDC_CHANNEL_0,
	                .duty       = 32,
	                .gpio_num   = 25,
	                .speed_mode = LEDC_HIGH_SPEED_MODE,
	                .hpoint     = 0,
	                .timer_sel  = LEDC_TIMER_0
	            };
	    ledc_timer_config(&PWM_timer);
	    ledc_channel_config(&PWM_channel);
	    ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
    	vTaskDelay(500 / portTICK_PERIOD_MS);

	    int nota;
	    int octava=0;
	    while (true) 
	    {
	    	for ( nota = 0; nota < 7; nota++) 
	    	{ 
				uint32_t frecuencia = NotasyOctavas[nota][octava];
				// Imprimir en consola la nota, octava y frecuencia
           		 printf("Nota: %s, Octava: %d, Frecuencia: %lu Hz\n", NombresNotas[nota], octava + 1, (unsigned long)frecuencia);
	             ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, frecuencia);
	             ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
	             ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
	
	             vTaskDelay(500 / portTICK_PERIOD_MS);
				if(nota==6)
				{
					printf(" ==== Empieza la Octava: %d =====\n", octava + 2);
					if(octava<8){octava=octava+1;}
					if(octava==8){octava=0;}
				}
				 ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
	    		vTaskDelay(100 / portTICK_PERIOD_MS);
        	}
        	
		    ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
	    	vTaskDelay(1000 / portTICK_PERIOD_MS);
	    }
	}
