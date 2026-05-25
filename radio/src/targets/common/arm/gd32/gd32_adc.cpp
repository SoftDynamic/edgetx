/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "gd32_adc.h"
#include "memory_sections.h"
#include "gd32_dma.h"
#include "gd32_gpio_driver.h"
#include "gd32_gpio.h"

#include "gd32_stdlib.h"

#include "hal.h"
#include "timers_driver.h"
#include "delays_driver.h"
#include "debug.h"

#include <string.h>
#include "FreeRTOSConfig.h"


#define OVERSAMPLING 4

#define SAMPLING_TIMEOUT_US 200

// Same prio for DMA TC and ADC IRQs to avoid preemption issues
#define ADC_IRQ_PRIO   configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

// GD32 ADC register bit helpers
#define _ENABLE_EOCIE(ADCx) SET_BIT(ADCx->CTL0, ADC_CTL0_EOCIE)
#define _DISABLE_EOCIE(ADCx) CLEAR_BIT(ADCx->CTL0, ADC_CTL0_EOCIE)
#define _START_ADC_SINGLE(ADCx) SET_BIT(ADCx->CTL1, ADC_CTL1_SWRCST)
#define _START_ADC_DMA(ADCx) SET_BIT(ADCx->CTL1, ADC_CTL1_SWRCST | ADC_CTL1_DMA)
#define _CLEAR_ADC_STATUS(ADCx) CLEAR_BIT(ADCx->STAT, ADC_STAT_STRC | ADC_STAT_EOC | ADC_STAT_WDE)

// GD32F3x0 has one ADC
#define ADC0 ((ADC_TypeDef *)ADC_BASE)

// GD32F3x0 has one ADC with up to 16 external channels + 1 internal VREF
// Ch: 0-7 on PA0-PA7, 8-9 on PB0-PB1, 10-15 on PC0-PC5
// Internal: CH16=VREF, CH17=VBAT, CH18=TEMP
#define GD32_ADC_CH_VREF_INT  16
#define GD32_ADC_CH_VBAT      17

// Max 32 inputs supported
static uint32_t _adc_input_mask;
static uint32_t _adc_input_inhibt_mask = 0;
static volatile uint32_t _adc_inhibit_mask;

// DMA buffers
static uint16_t _adc_dma_buffer[MAX_ADC_INPUTS] __DMA_NO_CACHE;

// ADCs started
static volatile uint8_t _adc_started_mask;
static volatile uint8_t _adc_completed;

static const gd32_adc_t* _adc_ADCs;
static uint8_t _adc_n_ADC;
static const gd32_adc_input_t* _adc_inputs;
static uint8_t _adc_n_inputs;

// Need for oversampling and decimation
static uint8_t _adc_run;
static uint8_t _adc_oversampling_disabled;
static uint16_t _adc_oversampling[MAX_ADC_INPUTS];

// Indicates ADC timeout has occured
static volatile bool _adc_timeout_error;

void gd32_hal_set_inputs_mask(uint32_t inputs)
{
  _adc_input_inhibt_mask |= inputs;
}

uint32_t gd32_hal_get_inputs_mask()
{
  return _adc_input_inhibt_mask;
}

void enableVBatBridge()
{
  if (adcGetMaxInputs(ADC_INPUT_RTC_BAT) < 1) return;

  adc_vbat_enable();

  auto channel = adcGetInputOffset(ADC_INPUT_RTC_BAT);
  _adc_inhibit_mask &= ~(1 << channel);
}

void disableVBatBridge()
{
  if (adcGetMaxInputs(ADC_INPUT_RTC_BAT) < 1) return;

  auto channel = adcGetInputOffset(ADC_INPUT_RTC_BAT);
  _adc_inhibit_mask |= (1 << channel);

  adc_vbat_disable();
}

bool isVBatBridgeEnabled()
{
  return !!READ_BIT(ADC_CTL1, ADC_CTL1_VBETEN);
}

static void adc_enable_clock()
{
  rcu_periph_clock_enable(RCU_ADC);
  rcu_adc_clock_config(RCU_ADCCK_APB2_DIV6);
}

static void adc_disable_dma();
static void adc_dma_clear_flags();

static void adc_init_pins(const gd32_adc_gpio_t* GPIOs, uint8_t n_GPIO)
{
  while (n_GPIO > 0) {
    for (uint8_t pin_idx = 0; pin_idx < GPIOs->n_pins; pin_idx++) {
      uint32_t pin = GPIOs->pins[pin_idx];
      // Configure as analog (input float, no pull)
      gpio_init(GPIO_PIN(GPIOs->GPIOx, pin), GPIO_IN, GPIO_PIN_SPEED_LOW);
      gpio_set(GPIO_PIN(GPIOs->GPIOx, pin));
    }
    GPIOs++; n_GPIO--;
  }
}

static uint32_t _rank_lookup[] = {
  0, 1, 2, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15
};

static void adc_init_channels(const uint8_t* chan, uint8_t nconv)
{
  if (!chan || !nconv) return;

  // GD32F3x0: RSQ0 holds sequence length, RSQ1/2/3 hold ranks
  // We use ADC_RSQx registers to set the sequence
  // Each channel requires: channel number + sample time

  uint8_t rank = 0;
  while (nconv > 0) {
    uint8_t ch = *chan;

    if (_adc_input_inhibt_mask & (1 << ch)) {
      nconv--; chan++;
      continue;
    }

    // Configure regular channel at this rank
    adc_regular_channel_config(rank, ch, ADC_SAMPTIME);

    _adc_input_mask |= (1 << ch);
    nconv--; rank++; chan++;
  }
}

static bool adc_init_dma_channel(uint16_t* dest, uint8_t nconv)
{
  adc_dma_clear_flags();

  dma_parameter_struct dmaInit;
  dma_struct_para_init(&dmaInit);

  dmaInit.periph_addr  = (uint32_t)&ADC_RDATA;
  dmaInit.memory_addr  = (uint32_t)dest;
  dmaInit.number       = nconv;
  dmaInit.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
  dmaInit.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
  dmaInit.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
  dmaInit.memory_width = DMA_PERIPHERAL_WIDTH_16BIT;
  dmaInit.direction    = DMA_PERIPHERAL_TO_MEMORY;
  dmaInit.priority     = DMA_PRIORITY_ULTRA_HIGH;
  dma_init(DMA_CH0, &dmaInit);

  dma_channel_enable(DMA_CH0);

  return true;
}

bool gd32_hal_adc_init(const gd32_adc_t* ADCs, uint8_t n_ADC,
                        const gd32_adc_input_t* inputs,
                        const gd32_adc_gpio_t* ADC_GPIOs, uint8_t n_GPIO)
{
  (void)inputs;
  _adc_input_mask = 0;
  _adc_inhibit_mask = 0;
  _adc_oversampling_disabled = 0;

  adc_init_pins(ADC_GPIOs, n_GPIO);
  memset(_adc_dma_buffer, 0, sizeof(_adc_dma_buffer));

  _adc_input_mask = 0;
  const gd32_adc_t* adc = ADCs;

  while (n_ADC > 0) {
    uint8_t nconv = adc->n_channels;
    if (nconv > 0) {
      adc_enable_clock();

      // Deinit and configure
      adc_deinit();

      // Configure resolution (12-bit)
      adc_resolution_config(ADC_RESOLUTION_12B);

      // Data alignment
      adc_data_alignment_config(ADC_DATAALIGN_RIGHT);

      // Disable continuous mode (single conversion)
      adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);

      // Configure scan mode for multiple channels
      if (nconv > 1) {
        adc_special_function_config(ADC_SCAN_MODE, ENABLE);
      }

      // Configure each channel
      const uint8_t* chan = adc->channels;
      adc_init_channels(chan, nconv);

      // Set sequence length
      adc_channel_length_config(ADC_REGULAR_CHANNEL, nconv);

      // Enable ADC
      adc_enable();

      // Calibrate
      delay_us(10);
      adc_calibration_enable();

      if (nconv > 1) {
        // Multiple channels - use DMA
        uint16_t* dma_buffer = _adc_dma_buffer + adc->offset;
        if (!adc_init_dma_channel(dma_buffer, nconv))
          return false;

        adc_dma_mode_enable();

        // Enable DMA channel IRQ
        dma_interrupt_enable(DMA_CH0, DMA_INT_FTF);
        NVIC_SetPriority(DMA_Channel0_IRQn, ADC_IRQ_PRIO);
        NVIC_EnableIRQ(DMA_Channel0_IRQn);
      } else {
        // Single channel - use EOC IRQ
        NVIC_SetPriority(ADC_CMP_IRQn, ADC_IRQ_PRIO);
        NVIC_EnableIRQ(ADC_CMP_IRQn);
      }
    }

    adc++; n_ADC--;
  }

  return true;
}

// GD32 DMA channel offset computation for flag checking
static inline dma_channel_enum _dma_get_channel(uint32_t ch)
{
  return (dma_channel_enum)ch;
}

static void adc_dma_clear_flags()
{
  dma_flag_clear(DMA_CH0, DMA_FLAG_FTF);
  dma_flag_clear(DMA_CH0, DMA_FLAG_HTF);
  dma_flag_clear(DMA_CH0, DMA_FLAG_ERR);
}

static void adc_start_dma_conversion()
{
  adc_dma_clear_flags();
  _CLEAR_ADC_STATUS(ADC0);
  _DISABLE_EOCIE(ADC0);

  // Re-enable DMA channel
  dma_channel_disable(DMA_CH0);
  dma_channel_enable(DMA_CH0);

  // Start ADC
  _START_ADC_DMA(ADC0);
}

static void adc_disable_dma()
{
  dma_channel_disable(DMA_CH0);
  dma_interrupt_disable(DMA_CH0, DMA_INT_FTF);
}

static void adc_start_normal_conversion()
{
  _CLEAR_ADC_STATUS(ADC0);
  _ENABLE_EOCIE(ADC0);
  _START_ADC_SINGLE(ADC0);
}

static void adc_start_read(const gd32_adc_t* ADCs, uint8_t n_ADC)
{
  uint8_t adc_mask = 1;
  _adc_started_mask = 0;

  const gd32_adc_t* adc = ADCs;
  while (n_ADC > 0) {
    if (adc->n_channels == 0) {
      adc++; n_ADC--; adc_mask <<= 1;
      continue;
    }

    bool seq_mode = (adc->n_channels > 1);
    if (seq_mode) {
      adc_disable_dma();
      _adc_started_mask |= adc_mask;
      adc_start_dma_conversion();
    } else {
      _adc_started_mask |= adc_mask;
      adc_start_normal_conversion();
    }

    adc++; n_ADC--; adc_mask <<= 1;
  }
}

bool gd32_hal_adc_start_read(const gd32_adc_t* ADCs, uint8_t n_ADC,
                              const gd32_adc_input_t* inputs, uint8_t n_inputs)
{
  _adc_timeout_error = false;
  _adc_completed = 0;
  _adc_run = 0;

  _adc_ADCs = ADCs;
  _adc_n_ADC = n_ADC;
  _adc_inputs = inputs;
  _adc_n_inputs = n_inputs;

  memset(_adc_oversampling, 0, sizeof(_adc_oversampling));
  adc_start_read(_adc_ADCs, _adc_n_ADC);

  return true;
}

static void copy_adc_values(uint16_t* src, const gd32_adc_t* adc)
{
  for (uint8_t i = 0; i < adc->n_channels; i++) {
    uint8_t channel = adc->channels[i];

    if (~_adc_input_mask & (1 << channel))
      continue;

    if (_adc_inhibit_mask & (1 << channel)) {
      src++;
      continue;
    }

    if (_adc_inputs[channel].inverted)
      _adc_oversampling[channel] += 0xFFF - *src;
    else
      _adc_oversampling[channel] += *src;

    src++;
  }
}

void gd32_hal_adc_wait_completion(const gd32_adc_t* ADCs, uint8_t n_ADC,
                                   const gd32_adc_input_t* inputs, uint8_t n_inputs)
{
  (void)ADCs;
  (void)n_ADC;
  (void)inputs;
  (void)n_inputs;

  auto timeout = timersGetUsTick();
  while (!_adc_completed) {
    if ((uint32_t)(timersGetUsTick() - timeout) >= SAMPLING_TIMEOUT_US) {
      _adc_timeout_error = true;
      return;
    }
  }
}

void gd32_hal_adc_disable_oversampling()
{
  _adc_oversampling_disabled = 1;
}

static void _adc_mark_completed(const gd32_adc_t* adc)
{
  uint8_t adc_idx = (adc - _adc_ADCs);
  _adc_started_mask &= ~(1 << adc_idx);
}

static void _adc_chain_conversions(const gd32_adc_t* adc)
{
  _adc_mark_completed(adc);

  if (_adc_started_mask != 0) return;
  if (_adc_timeout_error) return;

  if (!_adc_oversampling_disabled && (++_adc_run < OVERSAMPLING)) {
    adc_start_read(_adc_ADCs, _adc_n_ADC);
    return;
  }

  auto adcValues = getAnalogValues();
  for (uint8_t i = 0; i < _adc_n_inputs; i++) {
    if (~_adc_input_mask & (1 << i)) continue;
    if (_adc_inhibit_mask & (1 << i)) continue;
    adcValues[i] = _adc_oversampling[i] / OVERSAMPLING;
  }

  _adc_completed = 1;
}

// DMA channel 0 transfer complete ISR
extern "C" void DMA_Channel0_IRQHandler(void)
{
  if (dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_FTF) != RESET) {
    dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_FTF);

    dma_channel_disable(DMA_CH0);

    // Clear ADC DMA mode
    CLEAR_BIT(ADC0->CTL1, ADC_CTL1_DMA);

    uint16_t* dma_buffer = _adc_dma_buffer + _adc_ADCs->offset;
    copy_adc_values(dma_buffer, _adc_ADCs);

    _adc_chain_conversions(_adc_ADCs);
  }
}

// ADC EOC ISR (single channel mode)
extern "C" void ADC_CMP_IRQHandler(void)
{
  if (adc_interrupt_flag_get(ADC_INT_FLAG_EOC) != RESET) {
    adc_interrupt_flag_clear(ADC_INT_FLAG_EOC);
    _DISABLE_EOCIE(ADC0);

    uint16_t* dma_buffer = _adc_dma_buffer + _adc_ADCs->offset;
    *dma_buffer = ADC_RDATA;
    copy_adc_values(dma_buffer, _adc_ADCs);

    _adc_chain_conversions(_adc_ADCs);
  }
}
