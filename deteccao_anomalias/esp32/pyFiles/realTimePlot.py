# import serial
# import matplotlib.pyplot as plt
# import struct

# # Inicializa a conexão serial
# ser = serial.Serial('COM4', 115200)  # Substitua 'COM4' pela sua porta correta

# # Configurações para a plotagem
# plt.ion()
# fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))
# fig.suptitle('Sinais de Aceleração - Eixos X, Y, Z')

# # Dados para plotar
# x_data, y_data, z_data = [], [], []

# # Parâmetros de conversão
# range_g = 16  # RANGE em G
# ratio = ((2 * range_g) / (2**15)) * 9.81  # Fator de conversão
# max_points = 1000  # Número máximo de pontos no gráfico

# def read_sensor_block():
#     # Tamanho do bloco de dados: 600 pontos (200 pontos por eixo intercalados) * 2 bytes por ponto = 1200 bytes
#     data = ser.read(1200)
#     if len(data) == 1200:
#         # Converte e separa os dados em listas para cada eixo
#         values = [struct.unpack('>h', data[i:i+2])[0] * ratio for i in range(0, 1200, 2)]
#         x_block = values[0::3]  # Pega cada terceiro valor começando do índice 0
#         y_block = values[1::3]  # Pega cada terceiro valor começando do índice 1
#         z_block = values[2::3]  # Pega cada terceiro valor começando do índice 2
#         return x_block, y_block, z_block
#     return None, None, None

# def update_plot():
#     # Limpa os gráficos antes de plotar
#     ax1.cla()
#     ax2.cla()
#     ax3.cla()

#     # Plota os dados de cada eixo em subplots diferentes
#     ax1.plot(x_data, color='r')
#     ax1.set_title('Eixo X')
#     ax1.grid(True)

#     ax2.plot(y_data, color='g')
#     ax2.set_title('Eixo Y')
#     ax2.grid(True)

#     ax3.plot(z_data, color='b')
#     ax3.set_title('Eixo Z')
#     ax3.grid(True)

#     # Atualiza os gráficos
#     plt.draw()
#     plt.pause(0.01)

# # Loop para leitura e plotagem com deslocamento
# try:
#     while True:
#         x_block, y_block, z_block = read_sensor_block()
        
#         if x_block is not None and y_block is not None and z_block is not None:
#             # Adiciona os novos 200 pontos a cada lista
#             x_data.extend(x_block)
#             y_data.extend(y_block)
#             z_data.extend(z_block)

#             # Mantém o gráfico com no máximo 1000 pontos, removendo os mais antigos se necessário
#             if len(x_data) > max_points:
#                 x_data = x_data[-max_points:]
#                 y_data = y_data[-max_points:]
#                 z_data = z_data[-max_points:]

#             # Atualiza o gráfico
#             update_plot()

# except KeyboardInterrupt:
#     print("Interrompido pelo usuário.")
# finally:
#     ser.close()


import serial
import matplotlib.pyplot as plt
import struct
from scipy import integrate
from scipy.signal import detrend

# Inicializa a conexão serial
ser = serial.Serial('COM4', 921600)  # Substitua 'COM4' pela sua porta correta

# Configurações para a plotagem
plt.ion()
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))
fig.suptitle('Deslocamento - Eixos X, Y, Z')

# Dados para plotar
x_data, y_data, z_data = [], [], []
x_velocity, y_velocity, z_velocity = 0, 0, 0  # Velocidades iniciais acumuladas
x_displacement_accum, y_displacement_accum, z_displacement_accum = 0, 0, 0  # Deslocamento inicial para continuidade

# Parâmetros de conversão
range_g = 16  # RANGE em G
ratio = ((2 * range_g) / (2**15)) * 9.81  # Fator de conversão
max_points = 1000  # Número máximo de pontos no gráfico
dt = 0.01  # Intervalo de tempo entre amostras, ajuste conforme necessário

def read_sensor_block():
    # Tamanho do bloco de dados: 600 pontos (200 pontos por eixo intercalados) * 2 bytes por ponto = 1200 bytes
    data = ser.read(1200)
    if len(data) == 1200:
        # Converte e separa os dados em listas para cada eixo
        values = [struct.unpack('>h', data[i:i+2])[0] * ratio for i in range(0, 1200, 2)]
        x_block = values[0::3]  # Pega cada terceiro valor começando do índice 0
        y_block = values[1::3]  # Pega cada terceiro valor começando do índice 1
        z_block = values[2::3]  # Pega cada terceiro valor começando do índice 2
        return x_block, y_block, z_block
    return None, None, None

def compute_displacement(acceleration_data, initial_velocity, initial_displacement):
    # Remove a componente DC (filtro passa-alta simples)
    filtered_data = detrend(acceleration_data, type='constant')
    
    # Integração para obter velocidade
    velocities = integrate.cumtrapz(filtered_data, dx=dt, initial=initial_velocity)
    
    # Integração para obter deslocamento com o último valor acumulado como início
    displacements = integrate.cumtrapz(velocities, dx=dt, initial=initial_displacement)
    
    # Retorna o vetor de deslocamentos e a última velocidade e deslocamento para continuidade
    return displacements, velocities[-1], displacements[-1]

def update_plot():
    # Limpa os gráficos antes de plotar
    ax1.cla()
    ax2.cla()
    ax3.cla()

    # Plota os dados de deslocamento de cada eixo
    ax1.plot(x_data, color='r')
    ax1.set_ylim(-0.30, 0.30)
    ax1.set_title('Deslocamento - Eixo X')
    ax1.grid(True)

    ax2.plot(y_data, color='g')
    ax2.set_ylim(-0.30, 0.30)
    ax2.set_title('Deslocamento - Eixo Y')
    ax2.grid(True)

    ax3.plot(z_data, color='b')
    ax3.set_ylim(-0.30, 0.30)
    ax3.set_title('Deslocamento - Eixo Z')
    ax3.grid(True)

    # Atualiza os gráficos
    plt.draw()
    plt.pause(0.01)
x_displacement_last = 0
y_displacement_last = 0
z_displacement_last = 0
# Loop para leitura e cálculo do deslocamento com atualização a cada 200 pontos
try:
    while True:
        x_block, y_block, z_block = read_sensor_block()
        
        if x_block is not None and y_block is not None and z_block is not None:
            # Calcula o deslocamento e atualiza a velocidade para cada eixo
            x_displacement, x_velocity, x_displacement_accum = compute_displacement(x_block, x_velocity, x_displacement_accum)
            y_displacement, y_velocity, y_displacement_accum = compute_displacement(y_block, y_velocity, y_displacement_accum)
            z_displacement, z_velocity, z_displacement_accum = compute_displacement(z_block, z_velocity, z_displacement_accum)
            x_displacement = x_displacement+x_displacement_last
            y_displacement = y_displacement+y_displacement_last
            z_displacement = z_displacement+z_displacement_last
            x_displacement_last = x_displacement[-1]
            y_displacement_last = y_displacement[-1]
            z_displacement_last = z_displacement[-1]

            # Adiciona os novos dados de deslocamento, mantendo o limite de 1000 pontos
            x_data.extend(x_displacement)
            y_data.extend(y_displacement)
            z_data.extend(z_displacement)

            if len(x_data) > max_points:
                x_data = x_data[-max_points:]
                y_data = y_data[-max_points:]
                z_data = z_data[-max_points:]

            # Atualiza o gráfico
            update_plot()

except KeyboardInterrupt:
    print("Interrompido pelo usuário.")
finally:
    ser.close()
