#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "bh1750/bh1750.h"

// Headers de FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
//#include "semphr.h"


#define SDA_PIN 8
#define SCL_PIN 9
#define I2C_FREQ 100000

#define LED_PIN 25
#define LED_ON 	1
#define LED_OFF	0

#define PIN_GEN_SEÑAL 19      //GPIO 19 COMO SALIDA DE PWM: PIN 25
#define FREC_PWM      1200     //Frecuencia para el PWM



void setup_pwm(uint32_t pin, float frec, float dt ) {
   
    gpio_set_function(pin, GPIO_FUNC_PWM);          //Pongo el pin como pwm
    uint slice_num = pwm_gpio_to_slice_num(pin);    //Tomo el slice fisico del pin
    
    float clock_freq = 125000000.f;                 //Frec de clock de la pico 2
    float divider = clock_freq / (frec * 8192);     //Calculo con precision de 16 bits de la frec que se quiere--65536
    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, 8191);                  //16 bit de resolucion para el conteo maximo del ciclo de PWM--65535
    
    pwm_set_gpio_level(pin,dt * 8192);              //Calcula el duty cycle--65536
    pwm_set_enabled(slice_num,true);                // Habilitab el PWM en el pin
    printf("Estoy aca\n");    
}


int main()
{
    stdio_init_all();

    //setup_pwm(PIN_GEN_SEÑAL, 1000.f, 0.8f);
    //sleep_ms(1000);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);
    gpio_put(LED_PIN, LED_OFF);

    // I2C INIT
    // I2C0 (DEFAULT) a 100khz
    i2c_init(i2c0, I2C_FREQ);
    // I2C0_SDA en GPIO4
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    // I2C0_SCL en GPIO5
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    // Pongo pullups a 3V3
    //gpio_pull_up(SDA_PIN);
    //gpio_pull_up(SCL_PIN);

    uint16_t medicion;
    bool stat;
    stat = bh1750_init();
    if(stat == false){
        while(1){
            printf("Init error.../n");
            sleep_ms(500);
        }
    } 
    if(stat){
        printf("INIT OK\n");
        sleep_ms(1000);
    }

    
    float dt_variable = 0.5f;

    while (true){
        
        /*
        setup_pwm(PIN_GEN_SEÑAL, 1000.f, dt_variable);
        
        dt_variable = dt_variable + 0.1;
        sleep_ms(100);
        
        if( dt_variable == 0.9f)
            dt_variable = 0.0f;
        */
        setup_pwm(PIN_GEN_SEÑAL, 5000.f, 0.0f);
        sleep_ms(100);
        setup_pwm(PIN_GEN_SEÑAL, 5000.f, 0.0f);
        sleep_ms(100);

        medicion = bh1750_read();
        printf("Lux: %d\n", medicion);
        gpio_put(LED_PIN, LED_ON);
        sleep_ms(500);
        gpio_put(LED_PIN, LED_OFF);
        sleep_ms(500);
    }   
}


