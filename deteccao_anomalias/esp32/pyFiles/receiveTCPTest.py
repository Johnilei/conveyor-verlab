import socket
import numpy as np
import matplotlib.pyplot as plt

# Configurações do socket TCP
TCP_IP = '0.0.0.0'  # Recebe de qualquer IP
TCP_PORT = 12345    # Substitua pela porta que você está usando no ESP32
BUFFER_SIZE = 996  # Ajuste o tamanho do buffer conforme necessário

# Função para processar os dados recebidos
def process_data(data, x_data, y_data, z_data, data_buffer):
    data_buffer += data
    if len(data_buffer) % 6 == 0:  # 6 bytes para 3 valores int16
        accelerometer_data = np.frombuffer(data_buffer, dtype=np.int16).reshape(-1, 3)
        x_data.extend(accelerometer_data[:, 0])
        y_data.extend(accelerometer_data[:, 1])
        z_data.extend(accelerometer_data[:, 2])
        data_buffer = b''  # Limpa o buffer após o processamento
    return data_buffer

# Loop principal para aceitar várias conexões
while True:
    # Configuração do servidor TCP
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((TCP_IP, TCP_PORT))
    server_socket.listen(1)
    
    print("Aguardando conexão...")
    conn, addr = server_socket.accept()
    print(f"Conectado a: {addr[0]}:{addr[1]}")

    conn.settimeout(3.0)

    x_data, y_data, z_data = [], [], []  # Arrays para armazenar os dados de cada eixo
    data_buffer = b''  # Buffer para acumular dados recebidos

    try:
        while True:
            try:
                data = conn.recv(BUFFER_SIZE)
                if not data:
                    print(f"Conexão com {addr[0]}:{addr[1]} foi encerrada pelo cliente.")
                    break
                data_buffer = process_data(data, x_data, y_data, z_data, data_buffer)
            except socket.timeout:
                print("Timeout: 3 segundos sem dados recebidos.")
                break
    except Exception as e:
        print(f"Erro: {e}")
    finally:
        conn.close()
        server_socket.close()
        print("Conexão fechada. Aguardando nova conexão...")

    # Plot dos dados recebidos após o encerramento da conexão
    plt.figure(figsize=(10, 6))

    plt.subplot(3, 1, 1)
    plt.plot(x_data, label='X-axis')
    plt.legend()

    plt.subplot(3, 1, 2)
    plt.plot(y_data, label='Y-axis')
    plt.legend()

    plt.subplot(3, 1, 3)
    plt.plot(z_data, label='Z-axis')
    plt.legend()

    plt.tight_layout()
    plt.show()
