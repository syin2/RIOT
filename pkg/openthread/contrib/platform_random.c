/*
 * Copyright (C) 2017 Fundacion Inria Chile
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Implementation of OpenThread random platform abstraction
 *
 * @author      Jose Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include <stdint.h>

#include "openthread/platform/random.h"
#include "periph/cpuid.h"
#include "random.h"

#ifdef MODULE_CC2538_RF
#define SOC_ADC_ADCCON1_RCTRL0                  0x00000004  //ADCCON1 RCTRL bit 0
#define SOC_ADC_ADCCON1_RCTRL1                  0x00000008  //ADCCON1 RCTRL bit 1

#define SYS_CTRL_RCGCRFC_RFC0                   0x00000001

#define RFCORE_XREG_FRMCTRL0_INFINITY_RX        0x00000008
#define RFCORE_SFR_RFST_INSTR_RXON              0xE3        // Instruction set RX on

#define RFCORE_XREG_RSSISTAT_RSSI_VALID         0x00000001  // RSSI value is valid.

#define RFCORE_XREG_RFRND_IRND                  0x00000001

#endif


#define ENABLE_DEBUG (1)
#include "debug.h"

/* init random */
//void ot_random_init(void)
//{
//#ifdef CPUID_LEN
//    char cpu_id[CPUID_LEN];
//    cpuid_get(cpu_id);
//    uint32_t seed = 0;
//    for (unsigned i = 0; i < CPUID_LEN; i++) {
//        seed += cpu_id[i];
//    }
//    random_init(seed);
//#else
//    #error "CPU not supported (current CPU doesn't provide CPUID, required for entropy)"
//#endif
//}
///* OpenThread will call this to get a random number */
//uint32_t otPlatRandomGet(void)
//{
//    uint32_t rand_val = random_uint32();
//
//    DEBUG("otPlatRandomGet: %i\n", (int) rand_val);
//    return rand_val;
//}

#ifdef MODULE_CC2538_RF

static void generateRandom(uint8_t *aOutput, uint16_t aOutputLength)
{
    SOC_ADC_ADCCON1 &= ~(SOC_ADC_ADCCON1_RCTRL1 | SOC_ADC_ADCCON1_RCTRL0);
    SYS_CTRL_RCGCRFC = SYS_CTRL_RCGCRFC_RFC0;

    while((SYS_CTRL_RCGCRFC) != SYS_CTRL_RCGCRFC_RFC0);

    RFCORE_XREG_FRMCTRL0 = RFCORE_XREG_FRMCTRL0_INFINITY_RX;

    //instruction set RX on
    RFCORE_SFR_RFST = ISRXON;

    while(!RFCORE_XREG_RSSISTAT & RFCORE_XREG_RSSISTAT_RSSI_VALID);

    for(uint16_t index = 0; index < aOutputLength; index++)
    {
        aOutput[index] = 0;

        for(uint8_t offset = 0; offset < 8 * sizeof(uint8_t); offset++)
        {
            aOutput[index] <<= 1;
            aOutput[index] |= (RFCORE_XREG_RFRND & RFCORE_XREG_RFRND_IRND);
        }
    }
    DEBUG("generating random number...\n");
    //RX OFF
    RFCORE_SFR_RFST = ISRFOFF;
}

void ot_random_init(void)
{
    uint16_t seed = 0;

    while (seed == 0x0000 || seed == 0x8883)
    {
        generateRandom((uint8_t *)&seed, sizeof(seed));
    }

    SOC_ADC_RNDL = (seed >> 8) & 0xff;
    SOC_ADC_RNDL = seed & 0xff;
}

/* OpenThread will call this to get a random number */
uint32_t otPlatRandomGet(void)
{
    uint32_t rand_val = 0;

    SOC_ADC_ADCCON1 |= SOC_ADC_ADCCON1_RCTRL0;
    rand_val = SOC_ADC_RNDL | ( SOC_ADC_RNDH << 8);

    SOC_ADC_ADCCON1 |= SOC_ADC_ADCCON1_RCTRL0;
    rand_val |= (SOC_ADC_RNDL | (SOC_ADC_RNDH << 8 ) << 16);

    DEBUG("otPlatRandomGet: %i\n", (int) rand_val);
    return rand_val;
}

#endif
