import sys
import socket
import struct
import time
import datetime

# Constantes
MAX_BUFFER_SIZE = 1024
DNS_SERVER_PORT = 53
DNS_QUERY_TYPES = {
    'A': 1,
    'CNAME': 5,
    'SOA': 6,
    'NS': 2,
    'MX': 15
}
CACHE_SIZE = 10

# Cache
cache = []

# Función para agregar una consulta al cache
def add_to_cache(query, response_ip):
    if len(cache) == CACHE_SIZE:
        cache.pop(0)
    cache.append((query, response_ip))

# Función para buscar una consulta en el cache
def lookup_cache(query):
    for q, response_ip in cache:
        if q == query:
            return response_ip
    return None

# Función para formatear la solicitud DNS
def format_dns_query(domain, query_type):
    query = struct.pack('!BBHHHBB', 0x12, 0x34, 1, 0, 1, 0, DNS_QUERY_TYPES[query_type])
    query += b'\x00' + domain.encode() + b'\x00'
    query += b'\x00\x00\x00\x00\x00\x00'
    return query

# Función para enviar la solicitud DNS y recibir la respuesta
def send_dns_query(server_ip, query):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.sendto(query, (server_ip, DNS_SERVER_PORT))
    response, _ = client_socket.recvfrom(MAX_BUFFER_SIZE)
    client_socket.close()
    return response

# Función para procesar la respuesta DNS
def process_dns_response(response):
    domain = b''
    data = response[13:]
    while data:
        length = data[0]
        if length == 0:
            break
        domain += data[1:length + 1] + b'.'
        data = data[length + 1:]
    return domain.decode('utf-8')[:-1], socket.inet_ntoa(data[10:14])

# Función principal
def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} </path/log.log>")
        return

    log_file = sys.argv[1]

    while True:
        user_input = input("SERVER TYPE DOMAIN: ").strip()
        if user_input.lower() == "exit":
            break

        user_input = user_input.split()
        if len(user_input) != 3:
            print("Entrada inválida. Intente de nuevo.")
            continue

        server_ip, query_type, domain = user_input

        if query_type.upper() not in DNS_QUERY_TYPES:
            print(f"Tipo de consulta '{query_type}' no válido.")
            continue

        query_type = query_type.upper()
        query_key = f"{query_type} {domain}"
        response_ip = lookup_cache(query_key)
        if response_ip:
            print(f"Respuesta desde el cache: {domain} -> {response_ip}")
            continue

        query = format_dns_query(domain, query_type)
        response = send_dns_query(server_ip, query)

        try:
            resolved_domain, response_ip = process_dns_response(response)
            print(f"Respuesta del servidor: {resolved_domain} -> {response_ip}")
            add_to_cache(query_key, response_ip)
            client_ip = socket.gethostbyname(socket.gethostname())
            log_entry = f"{datetime.datetime.now()} | {client_ip} | {query_type} {domain} | {response_ip}"
            with open(log_file, "a") as log:
                log.write(log_entry + "\n")
        except Exception as e:
            print(f"Error al procesar la respuesta: {e}")

        if user_input[0].lower() == "flush":
            cache.clear()
            print("Cache borrado.")

if __name__ == "__main__":
    main()
