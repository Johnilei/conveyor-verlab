# import serial
# import numpy as np
# import matplotlib.pyplot as plt
# from scipy.fft import fft
# import time

# def plot_fft_serial(port, baudrate, num_points, sampling_rate):
#     # Configurar a porta serial
#     ser = serial.Serial(port, baudrate)

#     # Calcular o período de amostragem
#     T = 1.0 / sampling_rate

#     # Configurar o gráfico
#     plt.ion()  # Ativar o modo interativo

#     while True:
#         # Ler os dados da porta serial
#         data = []
#         for _ in range(num_points):
#             line = ser.readline()
#             data.append(float(line.strip()))

#         # Subtrair a média dos dados
#         data = data - np.mean(data)

#         # Aplicar a FFT
#         yf = fft(data)
#         xf = np.linspace(0.0, 1.0/(2.0*T), num_points//2)

#         # Plotar a FFT
#         plt.plot(xf, 2.0/num_points * np.abs(yf[0:num_points//2]))
#         plt.grid()
#         plt.draw()  # Desenhar o gráfico

#         # Pausar por 2 segundos e atualizar o gráfico
#         plt.pause(0.1)

#         # Limpar o gráfico para o próximo plot
#         plt.clf()

# # Chamar a função
# plot_fft_serial('COM5', 115200, 16000, 32000)



import serial
import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import fft
import serial
import struct
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal
from scipy import integrate
from scipy.ndimage import uniform_filter1d

# Inicializa a conexão serial
ser = serial.Serial('COM4', 115200)  # Substitua 'COM_PORT' pela porta correta

def read_sensor_data():
    buffer = b''
    startMarker = b'/R'
    endMarker = b'/E'

    while True:
        while ser.inWaiting() > 0:
            buffer += ser.read(ser.inWaiting())

            # Verifica se o buffer contém o identificador de início
            if startMarker in buffer:
                startIndex = buffer.find(startMarker) + len(startMarker)
                buffer = buffer[startIndex:]  # Remove tudo antes do início

            # Verifica se o buffer contém o identificador de fim
            if endMarker in buffer:
                endIndex = buffer.find(endMarker)
                data = buffer[:endIndex]  # Extrai os dados
                buffer = buffer[endIndex + len(endMarker):]  # Remove os dados processados

                # Converte os dados em uma lista de valores inteiros
                values = [struct.unpack('>h', data[i:i+2])[0] for i in range(0, len(data), 2)]

                # Agrupa os dados em eixos X, Y, Z
                x_data = values[0::3]  # Dados do eixo X
                y_data = values[1::3]  # Dados do eixo Y
                z_data = values[2::3]  # Dados do eixo Z

                return x_data, y_data, z_data

def plot_signal_serial():

    plt.ion()
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))
    fig.suptitle('Sinais de Aceleração - Eixos X, Y, Z')

    while True:
        range = 16  # RANGE em G
        ratio = ((2*range)/(2**15))*9.81

        # Lê os dados dos eixos X, Y e Z
        x_data, y_data, z_data = read_sensor_data()

        # Converte os dados para unidades de aceleração
        x_resultados = [(valor * ratio) for valor in x_data]
        y_resultados = [(valor * ratio) for valor in y_data]
        z_resultados = [(valor * ratio) for valor in z_data]

        # Limpa os gráficos antes de plotar
        ax1.cla()
        ax2.cla()
        ax3.cla()

        # Plota os dados de cada eixo em subplots diferentes
        ax1.plot(x_resultados, color='r')
        ax1.set_title('Eixo X')
        ax1.grid(True)

        ax2.plot(y_resultados, color='g')
        ax2.set_title('Eixo Y')
        ax2.grid(True)

        ax3.plot(z_resultados, color='b')
        ax3.set_title('Eixo Z')
        ax3.grid(True)

        # Atualiza os gráficos
        plt.draw()
        plt.pause(0.1)

def plot_fft_serial(sampling_rate):

    plt.ion()
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))
    fig.suptitle('FFT dos Sinais de Aceleração - Eixos X, Y, Z')

    T = 1.0 / sampling_rate

    while True:
        # Lê os dados dos eixos X, Y e Z
        x_data, y_data, z_data = read_sensor_data()

        # Remove o valor médio (DC component) para centragem no zero
        x_data = x_data - np.mean(x_data)
        y_data = y_data - np.mean(y_data)
        z_data = z_data - np.mean(z_data)

        # FFT dos dados dos três eixos
        x_fft = fft(x_data)
        y_fft = fft(y_data)
        z_fft = fft(z_data)

        # Frequências correspondentes
        N = len(x_data)
        xf = np.linspace(0.0, 1.0 / (2.0 * T), N // 2)

        # Limpa os gráficos anteriores
        ax1.cla()
        ax2.cla()
        ax3.cla()

        # Plota a magnitude da FFT dos dados do eixo X
        ax1.plot(xf, 2.0 / N * np.abs(x_fft[:N // 2]), color='r')
        ax1.set_ylim(-0.30, 0.30)
        ax1.set_title('FFT - Eixo X')
        ax1.grid(True)

        # Plota a magnitude da FFT dos dados do eixo Y
        ax2.plot(xf, 2.0 / N * np.abs(y_fft[:N // 2]), color='g')
        ax2.set_ylim(-0.30, 0.30)
        ax2.set_title('FFT - Eixo Y')
        ax2.grid(True)

        # Plota a magnitude da FFT dos dados do eixo Z
        ax3.plot(xf, 2.0 / N * np.abs(z_fft[:N // 2]), color='b')
        ax3.set_ylim(-0.30, 0.30)
        ax3.set_title('FFT - Eixo Z')
        ax3.grid(True)

        # Atualiza os gráficos
        plt.draw()
        plt.pause(0.1)

# Executa a função de plotagem
plot_signal_serial()
#plot_fft_serial(2000)


ordem = 2
lowpass_cutoff = 0.1
highpass_cutoff = 0.1
fs = 1000
T = 1.0 / fs
range = 16 #   RANGE em G
ratio = ((2*range)/(2**15))*9.81
#Exponetion Average filter - EMA

            
def plot_signal_complete_serial():
    x_data, y_data, z_data = read_sensor_data()
    data = x_data - np.mean(x_data)

    
    sos_lowpass = signal.butter(2, 0.1, btype='lowpass', fs=fs, output='sos')
    data_array = signal.sosfilt(sos_lowpass, data)
    #data_array = data - data_array
    #sos_lowpass = signal.butter(ordem, 3, btype='lowpass', fs=fs, output='sos')
    #data_array = signal.sosfilt(sos_lowpass, data_array)
    
 
    resultados_multiplicados = [(valor * ratio) for valor in data]

    #resultados_multiplicados = np.where(np.abs(resultados_multiplicados) < 0.07, 0, resultados_multiplicados)

    # velocidade - m/s
    velocity_x = integrate.cumulative_trapezoid(resultados_multiplicados, dx=T, initial=0)
    
    #velocity_x = velocity_x - np.mean(velocity_x)


    # deslocamento - m
    displacement_x = integrate.cumulative_trapezoid(velocity_x, dx=T, initial=0)
    #displacement_x = displacement_x - np.mean(displacement_x)
    #displacement_x = displacement_x - uniform_filter1d(displacement_x, size=2000)

    # updown_step_sensor_data.txt - sem remover offset no deslocamento
    # up_step_sensor_data.txt - sem remover offset na velocidade 
    
    '''
    plt.figure(figsize=(13, 6))
    plt.plot(resultados_multiplicados)
    plt.plot(velocity_x)
    plt.plot(displacement_x)
    #plt.plot(displacement_x - average_data)
    #plt.plot(average_data)
    #plt.ylim([-0.5, 0.5])
    plt.grid()
    plt.show()
    '''

    plt.figure(figsize=(13, 6))
    
    plt.subplot(3, 1, 1)
    plt.plot(resultados_multiplicados, label='Aceleração (m/s²)')
    plt.grid()
    plt.legend()

    plt.subplot(3, 1, 2)
    plt.plot(velocity_x, label='Velocidade (m/s)')
    plt.grid()
    plt.legend()

    plt.subplot(3, 1, 3)
    plt.plot(displacement_x, label='Deslocamento (m)')
    plt.grid()
    plt.legend()

    plt.tight_layout()
    plt.show()

# plot_signal_complete_serial()



