# Relatório — Semáforo com RGB LED no FRDM-KL25Z

## Nome
Felipe Beserra de Oliveira

---

## Número USP
Número USP Aqui

---

## Respostas, comentários e análises

O experimento consistiu na utilização da placa FRDM-KL25Z com o sistema operacional embarcado Zephyr RTOS para controlar o LED RGB integrado da placa.

Os pinos utilizados foram:

| Cor | Pino |
|---|---|
| Vermelho | PTB18 |
| Verde | PTB19 |
| Azul | PTD1 |

O LED RGB da FRDM-KL25Z utiliza lógica ativa em nível baixo (*active low*), ou seja:

- `0` → LED ligado
- `1` → LED desligado

O programa implementa o comportamento de um semáforo simples:

1. Vermelho ligado por 5 segundos
2. Verde ligado por 5 segundos
3. Amarelo (vermelho + verde) por 2 segundos
4. Repetição contínua do ciclo

Durante o desenvolvimento foi necessário adaptar os includes do Zephyr para versões mais recentes do framework, substituindo:

```c
#include <zephyr.h>
```

por:

```c
#include <zephyr/kernel.h>
```

Inicialmente, o tutorial recomendava simplificar o código evitando o uso de *Device Tree (DT)*. Entretanto, não foi possível adaptar corretamente o código sem DT para a versão mais recente do framework utilizada no ambiente PlatformIO/Zephyr. Dessa forma, optou-se por investir em uma implementação compatível com o modelo atual do Zephyr, utilizando Device Tree e as APIs modernas de GPIO.

As informações referentes aos LEDs RGB da placa foram obtidas na documentação oficial da plataforma:

> *FRDM-KL25Z User's Manual*, Rev. 2.0, 2013-10-24  
> Capítulo 5 — *FRDM-KL25Z Hardware Description*  
> Seção 5.6 — *RGB LED*

Também foram utilizadas as APIs de GPIO do Zephyr para configuração e controle dos pinos digitais.

---

## Código (main.c)

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* RGB LED pins */
#define RED_PIN     18
#define GREEN_PIN   19
#define BLUE_PIN    1

/* GPIO controllers */
#define RED_GPIO    DT_NODELABEL(gpiob)
#define GREEN_GPIO  DT_NODELABEL(gpiob)
#define BLUE_GPIO   DT_NODELABEL(gpiod)

static const struct device *gpiob = DEVICE_DT_GET(RED_GPIO);
static const struct device *gpiod = DEVICE_DT_GET(BLUE_GPIO);

void leds_off(void)
{
    gpio_pin_set(gpiob, RED_PIN, 1);
    gpio_pin_set(gpiob, GREEN_PIN, 1);
    gpio_pin_set(gpiod, BLUE_PIN, 1);
}

void red_on(void)
{
    leds_off();
    gpio_pin_set(gpiob, RED_PIN, 0);
}

void yellow_on(void)
{
    leds_off();

    /* Red + Green = Yellow */
    gpio_pin_set(gpiob, RED_PIN, 0);
    gpio_pin_set(gpiob, GREEN_PIN, 0);
}

void green_on(void)
{
    leds_off();
    gpio_pin_set(gpiob, GREEN_PIN, 0);
}

int main(void)
{
    if (!device_is_ready(gpiob) || !device_is_ready(gpiod)) {
        return -1;
    }

    /* Configure pins as outputs */
    gpio_pin_configure(gpiob, RED_PIN, GPIO_OUTPUT_HIGH);
    gpio_pin_configure(gpiob, GREEN_PIN, GPIO_OUTPUT_HIGH);
    gpio_pin_configure(gpiod, BLUE_PIN, GPIO_OUTPUT_HIGH);

    while (1) {

        /* RED */
        red_on();
        k_msleep(5000);

        /* GREEN */
        green_on();
        k_msleep(5000);

		/* YELLOW */
        yellow_on();
        k_msleep(2000);
    }
	
    return 0;
}
```

---

## Repositório

Link do repositório GitHub:

```text
https://github.com/Beserrovsky/piscaLED-PSI3441-zephyr-blink.git
```