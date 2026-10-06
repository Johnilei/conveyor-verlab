import socket
import struct
import os
import datetime
import csv

ESP32_IP = "192.168.4.1"
ESP32_PORT = 8888
PC_LISTEN_PORT = 8888
LOG_DIRECTORY = "logs" # Pasta onde arquivos CSV serão salvos
MIN_PACKETS_FOR_NEW_FILE = 5 # limite mínimo de pacotes para criar um novo arquivo

packet_buffer = {}
last_valid_csv_filepath = None

def save_as_new_csv(data_points, trans_id):
    # Cria um novo arquivo CSV e escreve o cabeçalho e os dados
    global last_valid_csv_filepath
    
    # gera o nome do arquivo
    now = datetime.datetime.now()
    filename = f"log_esteira_{now.strftime('%Y-%m-%d_%H-%M-%S')}_{trans_id}.csv"
    filepath = os.path.join(LOG_DIRECTORY, filename)

    try:
        with open(filepath, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            # escreve cabeçalho
            writer.writerow(['timestamp_us', 'rotX', 'rotY', 'rotZ', 'gForceX', 'gForceY', 'gForceZ'])
            # escreve todas as outras linhas (os dados)
            writer.writerows(data_points)
        print(f"Dados salvos com sucesso em NOVO arquivo: {filepath}")
        last_valid_csv_filepath = filepath
    except IOError as e:
        print(f"[!] Erro ao escrever o novo arquivo CSV: {e}")
        return None
    return filepath

def append_to_existing_csv(data_points, filepath):
    # Anexa dados a um arquivo CSV existente, sem escrever o cabeçalho
    # usada se a coleta for pequena demais, ou seja, se houver algum erro no sensor magnético
    try:
        with open(filepath, 'a', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerows(data_points)
        print(f"Dados anexados com sucesso ao arquivo: {filepath}")
    except IOError as e:
        print(f"[!] Erro ao anexar dados ao arquivo CSV: {e}")
    except FileNotFoundError:
        print(f"[!] Erro: O arquivo anterior {filepath} não foi encontrado para anexar os dados.")

def handle_completed_transmission(trans_id, received_data):
    # Decide se cria um novo arquivo ou anexa a um existente
    # Chamada quando todos os pacotes de uma coleta chegam
    total_packets = received_data['total_packets']
    print(f"\nTransmissão completa com {total_packets} pacotes recebida (ID: {trans_id}).")

    os.makedirs(LOG_DIRECTORY, exist_ok=True)
    sorted_packets = sorted(received_data['packets'].items()) # organiza os pacotes por número
    all_data_points = []
    
    # percorre os pacotes recebidos e extrai os dados
    for packet_num, packet_data in sorted_packets:
        try:
            _id, _p_num, _total_p, num_samples_in_packet = struct.unpack_from('>LHHH', packet_data)
        except struct.error as e:
            print(f"[!] Erro ao ler cabeçalho do pacote {packet_num}: {e}. Pulando pacote.")
            continue

        pointer = 10 # pula os primeiros 10 bytes (do cabeçalho)
        for _ in range(num_samples_in_packet):
            if len(packet_data) < pointer + 28:
                print(f"\n[!] Aviso: Fim inesperado do pacote {packet_num}.")
                break
            try:
                # Tenta desempacotar os dados do pacote
                data_point = struct.unpack_from('>Lffffff', packet_data, pointer)
                all_data_points.append(data_point)
                pointer += 28
            except struct.error as e:
                print(f"[!] Erro ao desempacotar dados no pacote {packet_num}: {e}")
                break

    if not all_data_points:
        print("[!] Nenhum dado válido foi decodificado.")
        return

    # Se for uma coleta pequena E já existe um arquivo anterior, anexa dados
    if total_packets < MIN_PACKETS_FOR_NEW_FILE and last_valid_csv_filepath:
        print(f"Coleta pequena detectada. Anexando ao arquivo anterior...")
        append_to_existing_csv(all_data_points, last_valid_csv_filepath)
    else:
        # Se for uma coleta grande (ou a primeira coleta), cria novo arquivo
        if total_packets < MIN_PACKETS_FOR_NEW_FILE:
            print(f"Coleta pequena detectada, mas nenhum arquivo anterior existe. Criando novo arquivo...")
        save_as_new_csv(all_data_points, trans_id)


def main():
    # Prepara a rede, envia o sinal inicial e ouve os dados
    
    # Cria o socket UDP
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind(('0.0.0.0', PC_LISTEN_PORT))
        print(f"Escutando na porta {PC_LISTEN_PORT}...")
    except OSError as e:
        print(f"[!] Erro ao vincular o socket à porta {PC_LISTEN_PORT}: {e}")
        return

    # Envia pacote inicial (sinal de conexão) para o ESP
    print(f"Enviando sinal de conexão para o ESP32 em {ESP32_IP}:{ESP32_PORT}")
    sock.sendto(b'hello_esp32', (ESP32_IP, ESP32_PORT))

    print("Aguardando o início da coleta de dados pelo ESP32...")
    print("Aproxime o ímã do sensor para iniciar o envio dos dados coletados")

    # Loop para receber pacotes
    try:
        while True:
            data, addr = sock.recvfrom(1500)
            
            try:
                header = struct.unpack_from('>LHHH', data) # Lê o cabeçalho do pacote
            except struct.error:
                print("\n[!] Pacote recebido com tamanho inválido.")
                continue

            trans_id, packet_num, total_packets, samples_in_packet = header
            
            # mostra o progresso do recebimento da coleta
            print(f"\r[*] Recebido pacote {packet_num}/{total_packets} da transmissão ID {trans_id}", end="")

            if trans_id not in packet_buffer: # Se for uma nova transmissão, inicializa o buffer
                packet_buffer[trans_id] = {
                    "total_packets": total_packets,
                    "packets": {}
                }
            
            packet_buffer[trans_id]['packets'][packet_num] = data
            
            # Verifica se todos os pacotes foram recebidos
            current_total_packets = packet_buffer[trans_id]['total_packets']
            if len(packet_buffer[trans_id]['packets']) == current_total_packets:
                handle_completed_transmission(trans_id, packet_buffer[trans_id]) # Processa e salva os dados
                del packet_buffer[trans_id]

    except KeyboardInterrupt:
        print("\n[!!!] Script encerrado pelo usuário.")
    finally:
        sock.close()
        print("Socket fechado.")


if __name__ == '__main__':
    main()