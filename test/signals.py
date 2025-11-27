import numpy as np
import soundfile as sf
import os

SR = 44100
DURATION = 5.0


def save_wav(name, data):
    print(f"Generazione {name}...")
    sf.write(name, data, SR)


# ###########
# #         #
# #  DIRAC  #
# #         #
# ###########

impulse = np.zeros(SR * 2)  # 2 secondi di silenzio
impulse[SR // 2] = 1.0  # Impulso al centro esatto
save_wav("./testSignals/test_impulse.wav", impulse)

# #################
# #               #
# #  WHITE NOISE  #
# #               #
# #################

noise = np.random.uniform(-1, 1, int(SR * DURATION))
save_wav("./testSignals/test_white_noise.wav", noise)

# 3. SINE SWEEP (Chirp Logaritmico)
# Da 20Hz a 20kHz
# f0 = 20
# f1 = 20000
# k = (f1 / f0) ** (1 / DURATION)
# phi = 2 * np.pi * f0 * ((k**t - 1) / np.log(k))
# sweep = np.sin(phi) * 0.5  # Gain 0.5 per sicurezza
# save_wav("./testSignals/test_sine_sweep.wav", sweep)

# ######################
# #                    #
# #  PURE SINE 1000Hz  #
# #                    #
# ######################

t = np.linspace(0, DURATION, int(SR * DURATION))
freq = 1000
sine_1k = np.sin(2 * np.pi * freq * t) * 0.8
save_wav("./testSignals/test_sine_1k.wav", sine_1k)

# ####################
# #                  #
# #  SAWTOOTH 100Hz  #
# #                  #
# ####################

f0 = 100
nyquist = 22000
harmonics = int(nyquist // f0)

saw_100 = np.zeros_like(t)

for k in range(1, harmonics + 1):
    saw_100 -= np.sin(2 * np.pi * f0 * k * t) / k

saw_100 *= (2 / np.pi) * 0.8
save_wav("./testSignals/test_saw_100.wav", saw_100)

print("\n--- Generazione Completata ---\n")
