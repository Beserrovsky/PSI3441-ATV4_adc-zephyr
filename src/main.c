/*
 * Atividade 4 — ADC via Registradores
 * FRDM-KL25Z: leitura analógica em PTB0 (ADC0_SE8) usando acesso direto
 * aos registradores do ADC0 (SIM_SCGC6, ADC0_CFG1, ADC0_SC1A, ADC0_RA).
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* SIM — habilitação de clock */
#define SIM_SCGC5           (*((volatile uint32_t *)0x40048038U))
#define SIM_SCGC6           (*((volatile uint32_t *)0x4004803CU))
#define SIM_SCGC5_PORTB     (1U << 10)
#define SIM_SCGC6_ADC0      (1U << 27)

/* PORTB_PCR0: MUX=000 configura pino como analógico */
#define PORTB_PCR0          (*((volatile uint32_t *)0x4004A000U))

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
    printk("=== ADC via Registradores — FRDM-KL25Z ===\n");
    printk("Canal: ADC0_SE8 (PTB0), 12 bits, Vref = %u mV\n", VREF_MV);

    while (1) {
        uint16_t raw = adc_read_se8();
        uint32_t mv  = (uint32_t)raw * VREF_MV / 4095U;
        printk("ADC: %4u (0x%03X) -> %4u mV\n", raw, raw, mv);
        k_msleep(500);
    }

    return 0;
}
