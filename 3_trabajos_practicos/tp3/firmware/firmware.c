#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "helper.h"   //Para generar el PWM

TaskHandle_t handle_Task_FREC = NULL;

SemaphoreHandle_t semphrCounting;   // Semaforo para CONTAR LOS FLANCOS

#define PIN_ENT_SEÑAL 18     //GPIO 24 COMO ENTRADA
#define PIN_GEN_SEÑAL 19     //GPIO 25 COMO SALIDA PWM
#define FREC_PWM 9500        //Frecuencia para el PWM

//TAREA FRECUENCIMETRO, DETECTOR DE FLANCOS ASCENDENTES
void task_frec(void *params) {
    
    bool estado_anterior = false;
    bool estado_actual = false;
    uint16_t flancos_ascendentes = 0;
    TickType_t tiempo_ms = xTaskGetTickCount();
    int contador = 0;
    
    while(1) {
        
        estado_anterior = estado_actual;
        estado_actual = gpio_get(PIN_ENT_SEÑAL);

        if(estado_actual && !estado_anterior){
            xSemaphoreGive(semphrCounting);  //flancos_ascendentes ++;
        }

        if(xTaskGetTickCount() - tiempo_ms > 1000){
            xSemaphoreTake( semphrCounting, 0 );
            contador = uxSemaphoreGetCount(semphrCounting);
            printf("Frecuencia: %i Hz\n", contador);
            printf("Error de conteo: %d Hz\n", FREC_PWM-contador);
            tiempo_ms = xTaskGetTickCount();
            xQueueReset(semphrCounting);    //flancos_ascendentes = 0;

        }

    }
}

/**
 * @brief Programa principal
 */
int main(void) {
    stdio_init_all();

    // Inicializacion de GPIO ENTRADA SEÑAL
    gpio_init(PIN_ENT_SEÑAL);
    gpio_set_dir(PIN_ENT_SEÑAL, false); //true para salida, false para entrada
    gpio_pull_down(PIN_ENT_SEÑAL);

    // Generador de PWM para comprobar contador de flancos
    pwm_user_init(PIN_GEN_SEÑAL, FREC_PWM);     //Esto es lo que hace el helper.
    
    //Creacion de Semaforo Counting
    semphrCounting = xSemaphoreCreateCounting( 10000, 0 );  
    
    // Creacion de tareas
    xTaskCreate(task_frec, "Task_Frecuencimetro", 4*configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    // Arranca el scheduler
    vTaskStartScheduler();
    while(1);
}