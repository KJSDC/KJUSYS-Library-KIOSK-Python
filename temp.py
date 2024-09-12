from flask import Flask, request, jsonify, render_template
from flask_socketio import SocketIO, emit
import serial.tools.list_ports
from flask_cors import CORS
import threading
import serial
import time

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")
CORS(app)

# Global variables for serial ports and threads
ser = None
id_thread = None
book_thread = None
stop_threads = False

# Global variables to store the latest data from each Arduino
id_data = ""
book_data = ""

# Commands
id_read_command = "I"
book_read_command = "B"
write_command = ";"
stop_write_command = ":"

# Function to send commands to serial
def Serial_write(command):
    if ser:
        ser.write(command.encode())
        print(f"Command sent to serial: {command}")

# Making it so that instead of endpoint calls every 0.5 seconds, it updates id data dynamically using sockets
def notify_id_change(new_id):
    global id_data
    id_data = new_id
    socketio.emit('id_changed', {'new_id': new_id})

# Making it so that instead of endpoint calls every 0.5 seconds, it updates book data dynamically using sockets
def notify_book_change(new_book):
    global book_data
    book_data = new_book
    socketio.emit('book_changed', {'new_book': new_book})

# Function to send command to serial to start the id read for 3 seconds
# once the read starts, read the next line data, ie, the id data and update it on the web
def get_id_read_data():
    global stop_threads, id_data
    while not stop_threads:
        if ser.in_waiting > 0:
            id_data = ser.readline().strip().decode('utf-8')
            try:
                if id_data == "PCD_Authenticate() failed: Error in communication." or id_data == "PCD_Authenticate() failed: Timeout in communication." or id_data == "MIFARE_Read() failed: The CRC_A does not match." :
                    print(f"Encountered error: {id_data}")

                elif id_data == "No Response..." or id_data == "Unknown command.":
                    print(f"System output found: {book_data}, skipping updation...")
                
                else:
                    notify_id_change(id_data)
                    print(f"ID data updated, ID: {id_data}")
            
            except serial.SerialException as e:
                print(f"Error reading the data from serial: {e}")
                break

# Function to send command to serial to start the id read for 3 seconds
# once the read starts, read the next line data, ie, the id data and update it on the web
def get_book_read_data():
    global stop_threads, book_data
    while not stop_threads:
        if ser.in_waiting > 0:
            book_data = ser.readline().strip().decode('utf-8')
            try:

                if book_data == "PCD_Authenticate() failed: Error in communication." or id_data == "PCD_Authenticate() failed: Timeout in communication." or id_data == "MIFARE_Read() failed: The CRC_A does not match." :
                    print(f"Encountered error: {book_data}")

                elif book_data == "No Response..." or book_data == "Unknown command.":
                    print(f"System output found: {book_data}, skipping updation...")
                
                else:
                    notify_book_change(book_data)
                    print(f"ID data updated, ID: {book_data}")
            
            except serial.SerialException as e:
                print(f"Error reading the data from serial: {e}")
                break

# Endpoint definitions

# Main page
@app.route('/')
def index():
    return render_template('library.html')

# List ports
@app.route('/ports', methods=['GET'])
def get_ports():
    ports = [port.device for port in serial.tools.list_ports.comports()]
    return jsonify({'ports': ports})

# Connect to the selected port
@app.route('/libraryPortConnect', methods=['POST'])
def connect_ports():
    global ser, id_thread, book_thread, stop_threads
    data = request.json

    port = data.get('selectedPort')

    try:
        # Establish connections to both serial ports
        ser = serial.Serial(port, baudrate=9600, timeout=1)
        # stop_threads = False

        return jsonify({'success': True})
    except serial.SerialException as e:
        return jsonify({'success': False, 'error': str(e)})
    
@app.route('/IDreadingStatus/<int:state>', methods=['POST'])
def id_status(state):
    global stop_threads, id_thread

    if state == 1:
        stop_threads = False
        id_thread = threading.Thread(target=get_id_read_data)
        id_thread.start()
        print("ID reader thread started...")

        while not stop_threads:
            Serial_write(id_read_command)
            time.sleep(3)
        return {"status": "ID reading stopped"}, 200
    
    elif state == 0:
        stop_threads = True
        if id_thread is not None:
            id_thread.join()
            print("ID reader thread stopped...")
        return jsonify({'status': 'ID read thread terminated'}), 200
    
    else:
        return jsonify({'status': "invalid state"}), 400
    
@app.route('/BookreadingStatus/<int:state>', methods=['POST'])
def book_status(state):
    global stop_threads, book_thread

    if state == 1:
        stop_threads = False
        book_thread = threading.Thread(target=get_book_read_data)
        book_thread.start()
        print("Book reader thread started...")

        while not stop_threads:
            Serial_write(book_read_command)
            time.sleep(3)
        return {"status": "Book reading stopped"}, 200
    
    elif state == 0:
        stop_threads = True
        if book_thread is not None:
            book_thread.join()
            print("ID reader thread stopped...")
        return jsonify({'status': 'Book read thread terminated'}), 200
    
    else:
        return jsonify({'status': "invalid state"}), 400

@app.route('/breakConnection', methods=['POST'])
def break_connection():
    global ser, stop_threads

    stop_threads = True
    if ser.is_open:
        ser.close()

    return jsonify({'status': 'Connections terminated'})

@app.route('/getBookData', methods=['GET'])
def get_book_data():
    # Return the latest book data from PN5180 reader
    return jsonify({
        'book_data': book_data
    })

@app.route('/getIDData', methods=['GET'])
def  get_id_data():
    # Return the latest id data from mfrc reader
    return jsonify({
        'id_data': id_data
    })

@app.route('/postData', methods=['POST'])
def postData():
    response = request.get_json()
    if 'id' in response:
        print(f"Received data: {response['id']}")
        return jsonify({'received data': response['id']})
    else:
        print("post failed")
        return jsonify({'status': "failed"})

if __name__ == '__main__':
    socketio.run(app)