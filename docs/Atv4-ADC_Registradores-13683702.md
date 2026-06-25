# Relatório — ADC via Registradores no FRDM-KL25Z (RELATÓRIO PARCIAL)

## Nome

Felipe Beserra de Oliveira

---

## Número USP

13683702

---

## Status: relatório parcial

O código foi escrito a partir do enunciado e **compila sem erros**, mas ainda não foi flashado nem validado na placa física. A seção "Testes planejados" abaixo descreve o procedimento de teste previsto, não um resultado já obtido — os valores reais de tensão/LED ainda precisam ser capturados.

### TODO — pendências para finalizar este relatório

1. Ligar um fio/potenciômetro em PTB0, variando a tensão entre GND e 3.3V (atenção: nunca superar 3.3V, sob risco de danificar o pino).
2. Flashar o binário compilado na FRDM-KL25Z (`pio.exe run --target upload`).
3. Capturar a saída serial (`pio.exe device monitor`) com os valores reais de `raw` e `mv` para ao menos três pontos: próximo de 0V, faixa intermediária e próximo de 3.3V.
4. Confirmar visualmente o acionamento correto do LED azul (perto de 3.3V), do LED verde (perto de 0V) e de nenhum dos dois na faixa intermediária.
5. Substituir a seção "Testes planejados" pelos dados reais capturados (valores de `raw`/`mv` observados na serial e confirmação do comportamento dos LEDs).

---

## Respostas, comentários e análises

### Descrição da Atividade

O experimento consiste em fazer uma aquisição analógica no FRDM-KL25Z configurando diretamente os registradores do ADC0 (sem usar o driver ADC do Zephyr) e, a partir do valor lido, acender o LED azul quando a tensão estiver próxima de 3.3V e o LED verde quando estiver próxima de 0V.

O canal utilizado foi o `ADC0_SE8`, disponível no pino PTB0.

### Registradores utilizados

| Registrador    | Endereço  | Função                                                                        |
| -------------- | ---------- | ------------------------------------------------------------------------------- |
| `SIM_SCGC6`  | 0x4004803C | Habilita o clock do módulo ADC0 (bit 27)                                       |
| `SIM_SCGC5`  | 0x40048038 | Habilita o clock dos Ports B e D (bits 10 e 12)                                 |
| `PORTB_PCR0` | 0x4004A000 | Configura PTB0 como entrada analógica (MUX = 000)                              |
| `ADC0_CFG1`  | 0x4003B008 | Resolução (12 bits) e divisor de clock do ADC                                 |
| `ADC0_SC1A`  | 0x4003B000 | Seleciona o canal e inicia a conversão; bit`COCO` indica conversão completa |
| `ADC0_RA`    | 0x4003B010 | Resultado da conversão (12 bits)                                               |

A conversão é feita por polling: o canal é escrito em `ADC0_SC1A`, o que dispara a conversão, e o programa aguarda em loop até o bit `COCO` (bit 7) ser setado antes de ler `ADC0_RA`.

O valor lido (0–4095) é convertido para milivolts:

```
mv = raw * VREF_MV / 4095
```

com `VREF_MV = 3300`.

### Acionamento dos LEDs

PTD1 (azul) e PTB19 (verde) foram configurados como saída via registradores (`PORTx_PCRn`, `GPIOx_PDDR`, `GPIOx_PSOR/PCOR`), com a mesma lógica *active-low* usada nas demais atividades. A cada leitura do ADC:

- `mv >= 2800`: LED azul ligado, verde apagado (tensão próxima de 3.3V)
- `mv <= 500`: LED verde ligado, azul apagado (tensão próxima de 0V)
- valor intermediário: ambos apagados

### Testes planejados (ainda não executados)

O plano de teste é ligar a entrada PTB0 manualmente a 3.3V, a GND e a um potenciômetro (faixa intermediária), e verificar se o LED azul acende perto de 3.3V, o verde perto de 0V, e nenhum dos dois nas faixas intermediárias. Os valores lidos (`raw` e `mv`) são impressos via `printk` a cada 500ms, o que permitirá registrar os números reais quando o teste for executado. **Nenhum desses testes foi realizado até o momento** — esta seção será substituída pelos resultados reais assim que a fiação for feita e a placa flashada.

---

## Código (main.c)

```c
/*
 * Atividade 4 — ADC via Registradores
 * FRDM-KL25Z: leitura analógica em PTB0 (ADC0_SE8) usando acesso direto
 * aos registradores do ADC0 (SIM_SCGC6, ADC0_CFG1, ADC0_SC1A, ADC0_RA).
 * O valor lido aciona o LED azul (PTD1) quando próximo de 3.3V e o
 * LED verde (PTB19) quando próximo de 0V, ambos via registradores GPIO.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* SIM — habilitação de clock */
#define SIM_SCGC5           (*((volatile uint32_t *)0x40048038U))
#define SIM_SCGC6           (*((volatile uint32_t *)0x4004803CU))
#define SIM_SCGC5_PORTB     (1U << 10)
#define SIM_SCGC5_PORTD     (1U << 12)
#define SIM_SCGC6_ADC0      (1U << 27)

/* PORTB_PCR0: MUX=000 configura pino como analógico */
#define PORTB_PCR0          (*((volatile uint32_t *)0x4004A000U))

/* PORTB_PCR19 / PORTD_PCR1: MUX=001 configura pino como GPIO */
#define PORTB_PCR19         (*((volatile uint32_t *)0x4004A04CU))
#define PORTD_PCR1          (*((volatile uint32_t *)0x4004C004U))
#define PORT_PCR_MUX_GPIO   (1U << 8)

/* GPIOB / GPIOD — registradores de dados e direção */
#define GPIOB_PSOR          (*((volatile uint32_t *)0x400FF044U))
#define GPIOB_PCOR          (*((volatile uint32_t *)0x400FF048U))
#define GPIOB_PDDR          (*((volatile uint32_t *)0x400FF054U))

#define GPIOD_PSOR          (*((volatile uint32_t *)0x400FF0C4U))
#define GPIOD_PCOR          (*((volatile uint32_t *)0x400FF0C8U))
#define GPIOD_PDDR          (*((volatile uint32_t *)0x400FF0D4U))

#define LED_GREEN_BIT       (1U << 19)  /* PTB19 */
#define LED_BLUE_BIT        (1U << 1)   /* PTD1  */

/* Limiares de tensao (mV) para acionamento dos LEDs */
#define V_NEAR_VCC_MV       2800U   /* proximo de 3.3V -> LED azul */
#define V_NEAR_GND_MV        500U   /* proximo de 0V   -> LED verde */

/* ADC0 — registradores */
#define ADC0_SC1A           (*((volatile uint32_t *)0x4003B000U))
#define ADC0_CFG1           (*((volatile uint32_t *)0x4003B008U))
#define ADC0_CFG2           (*((volatile uint32_t *)0x4003B00CU))
#define ADC0_RA             (*((volatile uint32_t *)0x4003B010U))
#define ADC0_SC2            (*((volatile uint32_t *)0x4003B020U))
#define ADC0_SC3            (*((volatile uint32_t *)0x4003B024U))

/*
 * ADC0_CFG1 = 0x05:
 *   ADICLK[1:0] = 01  -> bus clock / 2 (~12 MHz, dentro do limite ADC)
 *   MODE[3:2]   = 01  -> 12 bits de resolucao
 *   ADLSMP      = 0   -> tempo de amostragem curto
 *   ADIV[6:5]   = 00  -> divisor 1
 *   ADLPC       = 0   -> modo normal (nao low-power)
 */
#define ADC_CFG1_12BIT_BUS2 0x05U

/* ADC0_SC1A bits */
#define ADC_COCO            (1U << 7)   /* Conversion Complete */
#define ADC_CH_SE8          8U          /* ADC0_SE8 = PTB0 (single-ended) */
#define ADC_CH_DISABLE      0x1FU       /* desabilita conversor */

/* Tensao de referencia (mV) */
#define VREF_MV             3300U

static void adc_init(void)
{
    /* 1. Habilita clock do ADC0 via SIM_SCGC6 (bit 27) */
    SIM_SCGC6 |= SIM_SCGC6_ADC0;

    /* 2. Habilita clock do Port B (para PTB0) via SIM_SCGC5 */
    SIM_SCGC5 |= SIM_SCGC5_PORTB;

    /* 3. PTB0 como analogico: MUX=000 (valor padrao, escrever 0) */
    PORTB_PCR0 = 0U;

    /* 4. Configura ADC0: 12 bits, bus/2, amostragem curta */
    ADC0_CFG1 = ADC_CFG1_12BIT_BUS2;
    ADC0_CFG2 = 0x00U;
    ADC0_SC2  = 0x00U;  /* trigger por software, referencia interna */
    ADC0_SC3  = 0x00U;  /* conversao simples (nao continua) */

    /* 5. Desabilita conversor ate primeira leitura */
    ADC0_SC1A = ADC_CH_DISABLE;
}

static void leds_init(void)
{
    /* Habilita clock dos ports B e D */
    SIM_SCGC5 |= SIM_SCGC5_PORTB | SIM_SCGC5_PORTD;

    /* Configura PTB19 e PTD1 como GPIO */
    PORTB_PCR19 = PORT_PCR_MUX_GPIO;
    PORTD_PCR1  = PORT_PCR_MUX_GPIO;

    /* Direção: saída */
    GPIOB_PDDR |= LED_GREEN_BIT;
    GPIOD_PDDR |= LED_BLUE_BIT;

    /* LEDs sao active-low: nivel 1 = apagado */
    GPIOB_PSOR = LED_GREEN_BIT;
    GPIOD_PSOR = LED_BLUE_BIT;
}

static void leds_update(uint32_t mv)
{
    if (mv >= V_NEAR_VCC_MV) {
        GPIOD_PCOR = LED_BLUE_BIT;   /* azul ligado */
        GPIOB_PSOR = LED_GREEN_BIT;  /* verde apagado */
    } else if (mv <= V_NEAR_GND_MV) {
        GPIOB_PCOR = LED_GREEN_BIT;  /* verde ligado */
        GPIOD_PSOR = LED_BLUE_BIT;   /* azul apagado */
    } else {
        GPIOB_PSOR = LED_GREEN_BIT;  /* ambos apagados */
        GPIOD_PSOR = LED_BLUE_BIT;
    }
}

static uint16_t adc_read_se8(void)
{
    /* Inicia conversao: escrever canal em SC1A dispara a conversao */
    ADC0_SC1A = ADC_CH_SE8;  /* DIFF=0, AIEN=0, canal 8 */

    /* Aguarda COCO (bit 7 de SC1A) — polling */
    while (!(ADC0_SC1A & ADC_COCO)) { }

    /* Le resultado (12 bits, justificado a direita) */
    return (uint16_t)(ADC0_RA & 0x0FFFU);
}

int main(void)
{
    adc_init();
    leds_init();
    printk("=== ADC via Registradores — FRDM-KL25Z ===\n");
    printk("Canal: ADC0_SE8 (PTB0), 12 bits, Vref = %u mV\n", VREF_MV);

    while (1) {
        uint16_t raw = adc_read_se8();
        uint32_t mv  = (uint32_t)raw * VREF_MV / 4095U;
        leds_update(mv);
        printk("ADC: %4u (0x%03X) -> %4u mV\n", raw, raw, mv);
        k_msleep(500);
    }

    return 0;
}
```

---

## Repositório

```text
https://github.com/Beserrovsky/PSI3441-ATV4_adc-zephyr
```
