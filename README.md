# ESP32 AC Voltage Measurement Using ADC

## 1. Project Overview

This project measures an AC voltage using the internal ADC of an ESP32.

The input signal is obtained from a transformer and then conditioned before it is connected to the ESP32 ADC.

The main signal-conditioning stages are:


AC Source
   │
   ▼
Transformer
   │
   ▼
Voltage Divider
   │
   ▼
AC Coupling / Capacitor
   │
   ▼
1.65 V DC Bias / Level Shift
   │
   ▼
ESP32 ADC Input
   │
   ▼
ADC Samples
   │
   ▼
Software Processing
   │
   ├── Maximum
   ├── Minimum
   ├── Mean / DC Offset
   ├── AC Peak-to-Peak
   └── RMS


---

## 2. Why Signal Conditioning Is Required

The ESP32 ADC cannot directly measure a normal bipolar AC waveform.

An AC waveform can contain both positive and negative voltage:


       +V
        │       /\
        │      /  \
        │     /    \
────────┼────/──────\────── 0 V
        │  /          \
        │ /            \
        │/              \
       -V


The ESP32 ADC input must remain within the safe ADC input range. Therefore, the AC signal must be:

1. Reduced to an appropriate voltage using a resistor divider.
2. Shifted upward so that it does not go negative.
3. Connected to the ADC only after confirming the conditioned waveform is within the permitted input range.

---

# 3. Transformer

The transformer provides an isolated, lower-voltage AC signal suitable for measurement.

For example, the measured transformer secondary may be approximately:


6 V RMS


The actual voltage must always be measured with a suitable multimeter/oscilloscope before connecting the signal-conditioning circuit.

### Important

The transformer output is still AC and therefore alternates around 0 V.

For a sine wave:


Vpeak = Vrms × √2


For example, if the secondary is 6 V RMS:


Vpeak ≈ 6 × 1.414
      ≈ 8.49 V


Therefore, the transformer output should **not** be connected directly to an ESP32 ADC pin.

---

# 4. Resistor Divider

A resistor divider is used to reduce the transformer voltage.

Basic circuit:


Transformer AC
      │
      R1
      │
      ├────────── ADC signal-conditioning node
      │
      R2
      │
     GND

The divider equation is:


Vout = Vin × R2 / (R1 + R2)


Where:

- `Vin` = transformer input voltage
- `R1` = upper resistor
- `R2` = lower resistor
- `Vout` = reduced voltage

### Resistor values used during experimentation

The resistor color codes discussed during the project were:

- Blue-Green-Orange-Gold = **65 kΩ ±5%**
- Green-Blue-Red-Gold = **5.6 kΩ ±5%**
- Green-Red-Red-Gold = **5.2 kΩ ±5%**

Use the actual measured resistor values in the final hardware documentation.

### Important

The resistor divider must be designed using the **peak voltage**, not only the RMS voltage.

The maximum possible instantaneous voltage must remain within the safe range of the following signal-conditioning stage.

---

# 5. Why a Capacitor Is Used

The capacitor is used for AC coupling / DC isolation between stages.

A capacitor can block the DC component while allowing the changing AC component to pass.

In the experimental circuit, two **2.2 µF, 6.3 V capacitors** were connected in series.

When capacitors are connected in series:


Ctotal = (C1 × C2) / (C1 + C2)


For two equal 2.2 µF capacitors:


Ctotal = (2.2 × 2.2) / (2.2 + 2.2)
       = 1.1 µF


Therefore, two 2.2 µF capacitors in series are approximately equivalent to:


1.1 µF


The voltage rating of series capacitors requires appropriate consideration. Do not assume that the voltage rating simply doubles unless the voltage sharing is properly controlled.

---

# 6. 1.65 V DC Bias / Level Shifting

This is one of the most important parts of the circuit.

The AC waveform naturally goes above and below 0 V:


       +AC
        /\
       /  \
──────/────\────── 0 V
     /      \
    /        \
       -AC


The ESP32 ADC cannot be allowed to see the negative portion.

Therefore, a DC bias of approximately **1.65 V** is added.

The resulting waveform becomes:


             AC waveform after bias

3.0 V           /\
               /  \
1.65 V ───────/────\──────  DC bias
             /      \
0.3 V       /        \


Mathematically:


VADC(t) = VBIAS + VAC(t)


where:


VBIAS ≈ 1.65 V


For example, if the AC component is ±0.4 V:


Maximum = 1.65 + 0.4 = 2.05 V
Minimum = 1.65 - 0.4 = 1.25 V


The entire waveform is now positive.

---

# 7. Important Distinction: RMS vs Instantaneous ADC Samples

The ADC does **not** directly measure RMS voltage.

The ADC measures instantaneous voltage samples:


V[0]
V[1]
V[2]
V[3]
...


From these samples, software can calculate:

- Maximum
- Minimum
- Mean
- Peak-to-peak voltage
- AC RMS voltage

The waveform itself is reconstructed by plotting the samples against time.

---

# 8. ESP32 ADC Configuration

The project uses the ESP-IDF ADC OneShot driver.

Example configuration:

adc_oneshot_unit_init_cfg_t init_config =
{
    .unit_id = ADC_UNIT_1,
};


ADC channel:


ADC_CHANNEL_0


Attenuation:


ADC_ATTEN_DB_11


ADC bit width:


ADC_BITWIDTH_DEFAULT


The ADC is configured and then read repeatedly using:


adc_oneshot_read()


---

# 9. ADC Resolution

The ADC data used in the project is treated as a **12-bit ADC result**.

A 12-bit ADC has:


2^12 = 4096


possible digital codes.

Therefore the raw ADC range is:


0 to 4095


The basic conversion used during testing was:


voltage = ((float)raw * 3.3f) / 4095.0f;


or:


V = ADC_RAW × VREF / 4095


### Important

`3.3 V` in this simple formula is an assumed/reference value for the calculation. For accurate voltage measurement, ADC calibration and the actual ESP32 ADC characteristics should be considered.

---

# 10. Sampling

The experimental code uses:


vTaskDelay(pdMS_TO_TICKS(1));


This requests approximately a 1 ms delay between loop iterations.

Therefore, the intended sampling interval is approximately:


Ts = 1 ms


and the intended sampling frequency is approximately:


Fs = 1 / Ts
Fs = 1 / 0.001
Fs = 1000 samples/second


So the target sampling rate is approximately:


1000 Hz


### Important practical note

`vTaskDelay(1 ms)` does **not guarantee an exact 1000 Hz ADC sampling rate**. RTOS scheduling, ADC conversion time, logging time and other processing affect the actual interval.

For accurate fixed-rate sampling, a hardware timer, `esp_timer`, or another deterministic sampling mechanism should be considered.

---

# 11. Number of Samples Per AC Cycle

The number of samples per cycle is:


Samples per cycle = Sampling frequency / Signal frequency


For a 50 Hz AC signal:

Samples per cycle = 1000 / 50
                  = 20 samples


Therefore:


50 Hz → approximately 20 samples/cycle


For a 60 Hz signal:


1000 / 60 ≈ 16.67 samples/cycle


This explains why only about 20 samples are obtained for one 50 Hz cycle at a nominal 1 kHz sampling rate.

---

# 12. Collecting Samples

For testing, the project collected blocks of samples and printed them in this form:


0,1.294
1,1.289
2,1.285
3,1.289
...


The first value is the sample index.

The second value is the measured ADC voltage.

For example:


Sample 0 → 1.294 V
Sample 1 → 1.289 V
Sample 2 → 1.285 V


These values can be copied into Excel and plotted.

---

# 13. Plotting the Waveform

In Excel:

1. Put sample number in one column.
2. Put voltage in another column.
3. Select both columns.
4. Insert an **XY Scatter** chart.
5. Select the option that connects the points with lines.

Example:


X-axis → Sample Number
Y-axis → ADC Voltage


For a 50 Hz signal sampled at approximately 1 kHz:


0–19       → approximately one cycle
20–39      → approximately second cycle
40–59      → approximately third cycle


However, use the actual measured waveform rather than assuming exactly 20 samples per cycle.

---

# 14. Mean / DC Offset

Because the waveform is shifted upward, the ADC samples have a DC offset.

The mean is:


Mean = (V0 + V1 + V2 + ... + Vn-1) / n


In Excel:


=AVERAGE(B2:B401)


If the biased waveform is centered around approximately 1.65 V, the calculated mean should be near that value, subject to circuit loading, resistor tolerance, ADC error and signal characteristics.

---

# 15. Recovering the AC Component

After calculating the mean:


VAC = VADC - Mean


Example:


ADC sample = 1.90 V
Mean       = 1.65 V

AC sample = 1.90 - 1.65
          = +0.25 V


For another sample:


ADC sample = 1.40 V

AC sample = 1.40 - 1.65
          = -0.25 V


This recreates the bipolar AC waveform in software:


 +0.25 V       /\
              /  \
  0 V ───────/────\──────
            /      \
 -0.25 V   /        \


---

# 16. Peak-to-Peak Voltage

First find:


Vmax = maximum ADC sample
Vmin = minimum ADC sample


Then:


Vpp = Vmax - Vmin


For a biased waveform, the DC offset does not change the peak-to-peak value.

Example:


Vmax = 2.05 V
Vmin = 1.25 V

Vpp = 2.05 - 1.25
    = 0.80 V


---

# 17. AC Peak Voltage

For a reasonably sinusoidal and centered waveform:


Vpeak = Vpp / 2


Example:


Vpp = 0.80 V

Vpeak = 0.80 / 2
      = 0.40 V


This is the peak of the AC component at the ADC input.

---

# 18. RMS Calculation

For a sinusoidal AC waveform:


Vrms = Vpeak / √2


Example:


Vpeak = 0.40 V

Vrms = 0.40 / 1.414
     ≈ 0.283 V RMS


For actual sampled data, a better method is to first remove the DC offset and then calculate RMS from the samples:


VAC[i] = VADC[i] - Mean


Then:

 
Vrms = sqrt(Σ(VAC[i]^2) / N)
  

In Excel, if the original ADC values are in `B2:B401` and the mean is in `D2`, a helper column can be used:

  excel
=B2-$D$2
  

Then square the AC samples and calculate their average, followed by square root.

---

# 19. Converting ADC Voltage Back to Transformer Voltage

The voltage measured by the ESP32 is the **conditioned voltage**, not the original transformer voltage.

If the resistor divider ratio is:

 
K = R2 / (R1 + R2)
  

then approximately:

 
Vtransformer = Vdivider / K
  

If additional attenuation, gain, or other conditioning stages are present, all of those factors must be included in the complete calibration equation.

Do not use the simple 3.3/4095 equation alone to claim the original transformer voltage.

---

# 20. Example Signal Flow

For the experimental setup:

 
Transformer
   │
   │ AC voltage
   ▼
Resistor Divider
   │
   │ Reduced AC
   ▼
Series Capacitor(s)
   │
   │ AC component
   ▼
1.65 V Bias Network
   │
   │ Positive-shifted AC
   ▼
ESP32 ADC
   │
   │ Raw ADC samples
   ▼
ADC → Voltage
   │
   ▼
Mean removal
   │
   ▼
AC waveform
   │
   ├── Vmax
   ├── Vmin
   ├── Vpp
   └── Vrms
  

---

# 21. Observed Sample Data

During testing, samples similar to the following were observed:

 
0,1.294
1,1.289
2,1.285
3,1.289
...
100,1.638
...
140,1.716
...
200,1.614
...
250,1.414
...
300,1.237
...
399,1.405
  

The samples show the voltage moving upward and downward around the biased signal level.

A complete cycle should be identified from the waveform itself rather than from only the first 20 samples.

---

# 22. Why the Waveform May Not Look Perfect

Possible reasons include:

- Transformer waveform distortion
- Resistor tolerance
- Capacitor tolerance
- Bias/reference variation
- ADC non-linearity
- ADC noise
- ADC attenuation characteristics
- ESP32 supply/reference variation
- Loading of the signal-conditioning circuit
- RTOS timing variation
- `ESP_LOGI()` output taking significant time
- Insufficient sampling rate
- Incorrect RC time constant

Printing every ADC sample using `ESP_LOGI()` is particularly important to remember: the serial logging operation can take much longer than the ADC conversion itself. Therefore, the actual sample interval is not necessarily exactly 1 ms when every sample is printed.

---

# 23. Recommended Sampling Architecture

For final implementation, separate **sampling** from **printing**.

Recommended concept:

 
Timer
  │
  ├── ADC sample
  │
  └── Store sample in buffer

After N samples:
  │
  ├── Calculate mean
  ├── Calculate min/max
  ├── Calculate RMS
  └── Send/process data
  

Do not depend on UART logging to determine the sampling frequency.

For example:

  c
#define SAMPLE_RATE_HZ      1000
#define SAMPLE_COUNT        400
  

Collect 400 samples first, then print/process them.

At approximately 1000 samples/s:

 
400 samples ≈ 400 ms
  

For a 50 Hz signal:

 
400 / 20 ≈ 20 cycles
  

This gives a much better view of the waveform than printing only 20 samples.

---

# 24. Safety Notes

This circuit is intended for a **low-voltage transformer secondary**.

Do not connect the ESP32 ADC directly to mains voltage.

Always ensure:

- Transformer isolation is appropriate.
- ADC input never goes below ground.
- ADC input never exceeds the permitted voltage.
- Resistor power ratings are adequate.
- Capacitor voltage ratings are adequate.
- The 1.65 V bias source is stable.
- Ground/reference connections are correct.
- The conditioned signal is verified with a multimeter/oscilloscope before connecting to the ESP32.

---

# 25. Quick Reference

| Parameter | Value / Concept |
|---|---|
| ADC | ESP32 internal ADC |
| ADC driver | ESP-IDF ADC OneShot |
| ADC unit | ADC_UNIT_1 |
| ADC channel used in test | ADC_CHANNEL_0 |
| Attenuation | ADC_ATTEN_DB_11 |
| ADC resolution | 12-bit result |
| Raw range | 0–4095 |
| Voltage conversion used for testing | `raw × 3.3 / 4095` |
| Intended sample interval | ~1 ms |
| Intended sample rate | ~1000 samples/s |
| Approx. samples/cycle at 50 Hz | 20 |
| DC bias target | ~1.65 V |
| Experimental series capacitors | 2 × 2.2 µF |
| Main processing | Mean, Min, Max, Vpp, RMS |

---

# 26. Key Formulas

### ADC voltage

 
VADC = RAW × VREF / 4095
  

### Voltage divider

 
VOUT = VIN × R2 / (R1 + R2)
  

### Sampling frequency

 
Fs = 1 / Ts
  

### Samples per cycle

 
N = Fs / Fsignal
  

### Peak-to-peak

 
Vpp = Vmax - Vmin
  

### Sinusoidal peak

 
Vpeak = Vpp / 2
  

### Sinusoidal RMS

 
Vrms = Vpeak / √2
  

### Sampled-data RMS after DC removal

 
Vrms = sqrt(Σ(VADC[i] - Mean)^2 / N)
  

### AC component

 
VAC[i] = VADC[i] - Mean
  

---

# 27. Final Concept to Remember

The complete measurement chain is:

 
AC voltage
    ↓
Transformer
    ↓
Voltage reduction
    ↓
AC coupling
    ↓
1.65 V DC level shift
    ↓
ESP32 ADC
    ↓
0–4095 ADC samples
    ↓
Convert to voltage
    ↓
Calculate mean/DC offset
    ↓
Remove DC offset
    ↓
Recover AC waveform
    ↓
Calculate Vpp / Peak / RMS
    ↓
Apply divider/calibration factor
    ↓
Obtain estimated original AC voltage
  

This README is intended as a project reference so that the circuit principle, ADC configuration, sampling concept, waveform reconstruction and RMS calculation can be understood again without relying on the original development discussion.
