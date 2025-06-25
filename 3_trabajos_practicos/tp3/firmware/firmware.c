#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "helper.h" //Biblioteca para generar pulsos y testear.

#include "lcd.h"

#define I2C_PORT i2c0
#define I2C_SDA  8   //PIN 11-GPIO8
#define I2C_SCL  9   //PIN 12-GPIO9
#define ADDR     0x27    // Direccion de 7 bits del adaptador del LCD

TaskHandle_t handle_Task_FREC = NULL;

SemaphoreHandle_t semphrCounting;   // Semaforo para CONTAR LOS FLANCOS.

#define PIN_ENT_SEÑAL 18     //GPIO 18 COMO ENTRADA DE SEÑAL: PIN 24
#define PIN_GEN_SEÑAL 19     //GPIO 19 COMO SALIDA DE PWM: PIN 25
#define FREC_PWM      9500     //Frecuencia para el Helper

void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (gpio == PIN_ENT_SEÑAL && (events & GPIO_IRQ_EDGE_RISE)) {
        xSemaphoreGiveFromISR(semphrCounting, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

//TAREA FRECUENCIMETRO, DETECTOR DE FLANCOS ASCENDENTES
void task_frec(void *params) {
    
    TickType_t tiempo_ms = xTaskGetTickCount();
    int contador = 0;
    char str1[20]="FREC:";
    char str2[20]="";
    
    //HABILITO LA INTERRUPCION DEL PIN 24 POR FLANCOS ASCENDENTES
    gpio_set_irq_enabled_with_callback(PIN_ENT_SEÑAL, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    while(1){

        if(xTaskGetTickCount() - tiempo_ms > 1000){
            xSemaphoreTake( semphrCounting, 0 );
            contador = uxSemaphoreGetCount(semphrCounting);
            lcd_clear();
            lcd_set_cursor(0,0);
            lcd_string(str1);
            lcd_set_cursor(0, 6);
            sprintf(str2, "%i Hz", contador);
             lcd_string(str2);
            //lcd_set_cursor(1,0);
            //sprintf("Error:", "%i Hz", FREC_PWM-contador);

            tiempo_ms = xTaskGetTickCount();
            xQueueReset(semphrCounting);        //flancos_ascendentes = 0;
        }
    vTaskDelay(pdMS_TO_TICKS(10));  // Pequeño delay para no saturar la CPU

    }
}


int main()
{
    stdio_init_all();

    // Inicializacion de GPIO ENTRADA SEÑAL
    gpio_init(PIN_ENT_SEÑAL);
    gpio_set_dir(PIN_ENT_SEÑAL, false);         //true para salida, false para entrada
    gpio_pull_down(PIN_ENT_SEÑAL);

    i2c_init(I2C_PORT, 400*1000);                    // Inicializo el I2C con un clock de 400 KHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Habilito la funcion de I2C en los GPIOs
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);       // Habilito pull-ups
    gpio_pull_up(I2C_SCL);
    lcd_init(I2C_PORT, ADDR);         // Inicializo LCD
    lcd_clear();                // Limpio el LCD

    // Generador de PWM para comprobar contador de flancos
    pwm_user_init(PIN_GEN_SEÑAL, FREC_PWM);     //Esto es lo que se hace con Helper

    //Creacion de Semaforo Counting
    semphrCounting = xSemaphoreCreateCounting( 10000, 0 );  //Limite de conteo: 10k. Inicio de conteo: 0.
    
    // Creacion de tareas
    xTaskCreate(task_frec, "Task_Frecuencimetro", 4*configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    // Arranca el scheduler
    vTaskStartScheduler();
    while (true);
}