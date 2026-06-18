"""
STM32 IoT Gateway 上位机 - TCP Server
监听端口，接收 STM32 上报的温湿度数据并显示

用法:
  python tcp_server.py [port]
  默认端口 5000
"""
import socket
import sys
import time
import threading

HOST = '0.0.0.0'
PORT = 5000


def handle_client(conn, addr):
    """处理单个客户端连接"""
    print(f'[+] 设备已连接: {addr}')

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                print(f'[-] 设备断开: {addr}')
                break

            text = data.decode('utf-8', errors='replace').strip()
            timestamp = time.strftime('%H:%M:%S')

            # 解析温湿度数据
            for line in text.split('\n'):
                line = line.strip()
                if not line:
                    continue
                if line.startswith('T:') and 'H:' in line:
                    print(f'[{timestamp}] 温湿度 -> {line}')
                else:
                    print(f'[{timestamp}] {line}')
    except (ConnectionResetError, OSError) as e:
        print(f'[-] 连接异常: {addr} ({e})')
    finally:
        conn.close()


def main():
    port = PORT
    if len(sys.argv) >= 2:
        port = int(sys.argv[1])

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, port))
    server.listen(1)

    print(f'=== STM32 IoT Gateway 上位机 ===')
    print(f'监听 {HOST}:{port} ...')
    print(f'等待设备连接...\n')

    try:
        while True:
            conn, addr = server.accept()
            t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            t.start()
    except KeyboardInterrupt:
        print('\n退出')
    finally:
        server.close()


if __name__ == '__main__':
    main()
