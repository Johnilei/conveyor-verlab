import numpy as np
from scipy.fft import fft
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter
# 1. Criando o sinal de senoide de aceleração em m/s²

# 2. Definindo constantes
num_points = 32000

sampling_rate = 32000
T = 1.0 / sampling_rate
f1 = 100
f2 = 2000
f3 = 4000
f4 = 8000
f5 = 10000
amplitude = 20

# 3. Criando o vetor de tempo
t = np.linspace(0.0, num_points*T, num_points)
f = np.linspace(0.0, 32000, num_points)

# 4. Criando o sinal de senoide
y1 = amplitude * np.sin(2 * np.pi * f1 * t)
y2 = amplitude * np.sin(2 * np.pi * f2 * t)
y3 = amplitude * np.sin(2 * np.pi * f3 * t)
y4 = amplitude * np.sin(2 * np.pi * f4 * t)
y5 = amplitude * np.sin(2 * np.pi * f5 * t)
y = y1

with open('senoide.txt', 'w') as file:
    file.write("[")
    for value in y:
        file.write(f"{value}, ")
    file.write("]")


# 5. Calculando a velocidade baseada na aceleração e conversão para mm/s
velocity = np.cumsum(y) * T * 1000

# 6. Criando filtro passa altas para eliminar a interferência DC do sinal (Filtro butterworth ordem 2 com frequencia de corte em 5 Hz)
b, a = butter(2, 5/(0.5*sampling_rate), btype='high')

# 7. Aplicando o filtro ao vetor de dados
filtered_velocity = lfilter(b, a, velocity)


fftvel = (2.0/num_points * np.abs(fft(y)))
# 8. Plotando a FFT do sinal
plt.figure(figsize=(10, 6))
plt.plot(f[0:num_points//2],fftvel[0:num_points//2])
plt.title("FFT ")
plt.xlabel("Freq")
plt.ylabel("Magnitude")
plt.grid()
plt.show()


