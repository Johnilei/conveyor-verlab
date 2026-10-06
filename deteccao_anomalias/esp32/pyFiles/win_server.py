# ----------------------------------------------------------------------------------
# Título: Servidor de Aquisição e Armazenamento de Dados de Sensores
# Descrição: Este software implementa um servidor TCP para receber dados de sensores
#              (acelerômetro, giroscópio e temperatura), salvar cada conexão em
#              arquivos CSV separados.
# Autores: [Clayton Carvalho, Romeu Takeda]
# Empresa: [Dielétrons - Eletrônica Industrial]
# Última revisão: 27 de Março de 2025
# ----------------------------------------------------------------------------------

import socket
import numpy as np
import threading
import csv
import queue
from concurrent.futures import ThreadPoolExecutor
import datetime
import logging
import struct

SERVER_IP = '0.0.0.0' # Broadcasting
SERVER_PORT = 12345
BUFFER_SIZE = 996
SAMPLE_SIZE_BYTES = 14  # Tamanho em bytes de cada amostra recebida

running = True
data_queue = queue.Queue()

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def save_data():
    """
    Consome dados da fila, salva-os em arquivos CSV.
    """
    while True:
        dados = data_queue.get()
        if dados is None:
            data_queue.task_done()
            break
        ip, timestamp, data_list = dados
        filename = f"{ip}_{timestamp.strftime('%Y%m%d_%H%M%S')}.csv"
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as arquivo_csv:
                escritor = csv.writer(arquivo_csv)
                escritor.writerow(['x_data', 'y_data', 'z_data', 'x_gyro', 'y_gyro', 'z_gyro', 'temp'])
                escritor.writerows(data_list)
            data_queue.task_done()
        except IOError as e:
            logging.error(f"Erro de I/O ao salvar o arquivo {filename}: {e}")

def handle_client(conn, addr):
    """
    Lida com a conexão de um cliente, recebendo dados e enfileirando-os para salvamento.

    Args:
        conn (socket.socket): O objeto socket da conexão com o cliente.
        addr (tuple): O endereço do cliente (IP, porta).
    """
    logging.info(f"Conectado a: {addr[0]}:{addr[1]}")
    conn.settimeout(6.0)
    data_buffer = b''
    all_samples = []

    # Recebe o cabeçalho com o tamanho esperado
    header = conn.recv(4)
    expected_bytes = int.from_bytes(header, byteorder='little')
    print(f"Esperando {expected_bytes} bytes de dados do cliente.")

    received_bytes = 0  # Contador de bytes recebidos

    try:
        while running:
            try:
                data = conn.recv(BUFFER_SIZE)
                if not data:
                    logging.info(f"Conexão encerrada por {addr[0]}.")
                    break
                data_buffer += data
                received_bytes += len(data)

                while len(data_buffer) >= SAMPLE_SIZE_BYTES:
                    sample_bytes = data_buffer[:SAMPLE_SIZE_BYTES]
                    sample = np.frombuffer(sample_bytes, dtype=np.int16)
                    all_samples.append(sample.tolist())
                    data_buffer = data_buffer[SAMPLE_SIZE_BYTES:]

                # Envia resposta assim que atingir o esperado
                if received_bytes >= expected_bytes:
                    sleepTime = 240  # Tempo de sono
                    untriggerTime = 1  # Tempo de não trigger
                    nSamples = 5  # Numero de amostras
                    interTriggerTime = 5 # Tempo de aceitação de novo trigger
                    resposta = struct.pack('<HHHH', sleepTime, untriggerTime, nSamples, interTriggerTime)  # little-endian, 4x uint16
                    conn.sendall(resposta)
                    logging.info(f"Resposta enviada para {addr[0]}.")
                    break  # Sai do loop após enviar a resposta

            except socket.timeout:
                logging.warning(f"Timeout: Conexão encerrada por {addr[0]}.")
                break
            except socket.error as e:
                logging.error(f"Erro de socket ao comunicar com {addr[0]}: {e}")
                break
    except Exception as e:
        logging.error(f"Erro ao lidar com o cliente {addr[0]}: {e}")
    finally:
        conn.close()
        if all_samples:
            data_queue.put((addr[0], datetime.datetime.now(), all_samples))
        logging.info(f"Thread do cliente {addr[0]} finalizada.")

def server_main():
    """
    Função principal do servidor, aceita conexões de clientes e as delega para tratamento.
    """
    global running
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(5)
    server_socket.settimeout(1.0)
    logging.info("Servidor iniciado. Aguardando conexões...")

    with ThreadPoolExecutor(max_workers=10) as executor:
        while running:
            try:
                conn, addr = server_socket.accept()
                executor.submit(handle_client, conn, addr)
            except socket.timeout:
                continue
            except Exception as e:
                logging.error(f"Erro no servidor: {e}")

    server_socket.close()
    logging.info("Servidor encerrado.")
    data_queue.put(None)

def exit_monitor():
    """
    Monitora a entrada do usuário para encerrar o servidor.
    """
    global running
    print("Digite 'q' para encerrar o servidor.")
    while True:
        command = input().strip().lower()
        if command == "q":
            running = False
            print("Encerrando o servidor...")
            break

if __name__ == "__main__":
    server_thread = threading.Thread(target=server_main)
    exit_thread = threading.Thread(target=exit_monitor)
    consumer_thread = threading.Thread(target=save_data)

    server_thread.start()
    exit_thread.start()
    consumer_thread.start()

    server_thread.join()
    exit_thread.join()
    data_queue.join()
    consumer_thread.join()

    running = False
    print("Servidor completamente encerrado.")