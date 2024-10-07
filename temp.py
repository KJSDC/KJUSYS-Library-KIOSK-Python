from flask import Flask, request, jsonify, render_template
from flask_socketio import SocketIO
import serial.tools.list_ports
from flask_cors import CORS
import threading
import serial
from dotenv import load_dotenv
import os
import sys

# Initialize Flask application and configure SocketIO and CORS
app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")
CORS(app)

load_dotenv()

COM_PORT = os.getenv("COM_PORT")

if not COM_PORT:
    print("COM_PORT not configured in .env")
    sys.exit()

# Global variables for serial communication and threads
ser = None
id_thread = None
book_thread = None
stop_threads = threading.Event()

MIFARE_ERRORS_LIST = [
    "PCD_Authenticate() failed: Error in communication.", 
    "PCD_Authenticate() failed: Timeout in communication.",
    "MIFARE_Read() failed: Error in communication.", 
    "MIFARE_Read() failed: Collision detected.", 
    "MIFARE_Read() failed: The CRC_A does not match."
]

SYSTEM_OUTPUT_LIST = [
    "No Response...", 
    "Unknown command."
]

# Variables to store the latest data read from each Arduino
id_data = ""
book_data = ""

# Define commands to communicate with the serial device
id_read_command = "I"
book_read_command = "B"
write_command = ";"
stop_write_command = ":"

# Function to send commands to the serial port
def Serial_write(command):
    global ser
    if ser:
        ser.write(command.encode())
        print(f"Command sent to serial: {command}")

# Function to notify the web clients about ID data changes
def notify_id_change(new_id):
    global id_data
    id_data = new_id
    socketio.emit('id_changed', {'new_id': new_id})

# Function to notify the web clients about book data changes
def notify_book_change(new_book):
    global book_data
    book_data = new_book
    socketio.emit('book_changed', {'new_book': new_book})

# Function to read ID data from the serial port
def get_id_read_data():
    global id_data, ser, MIFARE_ERRORS_LIST, SYSTEM_OUTPUT_LIST
    
    # send id read command to read id
    Serial_write(id_read_command)
    while not stop_threads.is_set():
        
        if ser.in_waiting > 0:
            id_data = ser.readline().strip().decode('utf-8')
            try:
                # Handle various error messages from the device
                if id_data in MIFARE_ERRORS_LIST:
                    print(f"Encountered error: {id_data}")

                elif id_data in SYSTEM_OUTPUT_LIST:
                    print(f"System output found: '{id_data}', skipping updation...")
                    
                else:
                    notify_id_change(id_data)
                    print(f"ID data updated, ID: {id_data}")
            
            except serial.SerialException as e:
                print(f"Error reading the data from serial: {e}")
                break

            finally:
                # send id read command to read id again
                Serial_write(id_read_command)
        

# Function to read book data from the serial port
def get_book_read_data():
    global book_data, ser

    # send book read command to read book
    Serial_write(book_read_command)
    while not stop_threads.is_set():
        if ser.in_waiting > 0:
            book_data = ser.readline().strip().decode('utf-8')
            try:
                # Handle various error messages from the device
                if book_data in MIFARE_ERRORS_LIST:
                    print(f"Encountered error: {book_data}")

                elif book_data in SYSTEM_OUTPUT_LIST:
                    print(f"System output found: '{book_data}', skipping updation...")
                
                else:
                    notify_book_change(book_data)
                    print(f"Book data updated, Book: {book_data}")
            
            except serial.SerialException as e:
                print(f"Error reading the data from serial: {e}")
                break

            finally:
                # send book read command to read book again
                Serial_write(book_read_command)

# connect to com port
def kisokConnect():
    global ser
    try:
        # Establish connection to the serial port
        ser = serial.Serial(COM_PORT, baudrate=9600, timeout=1)
        print(f"Connected to {COM_PORT}")
    except serial.SerialException as e:
        print(f"Failed to connect to {COM_PORT}")

##
## ENDPOINTS
##

# Define endpoint for the main page
@app.route('/')
def index():
    return render_template('library.html')

# Endpoint to list available serial ports
@app.route('/ports', methods=['GET'])
def get_ports():
    ports = [port.device for port in serial.tools.list_ports.comports()]
    return jsonify({'ports': ports})

# Endpoint to connect to the selected serial port
@app.route('/libraryPortConnect', methods=['POST'])
def connect_ports():
    global ser
    data = request.json
    port = data.get('selectedPort')

    try:
        # Establish connection to the serial port
        ser = serial.Serial(port, baudrate=9600, timeout=1)
        return jsonify({'success': True})
    except serial.SerialException as e:
        return jsonify({'success': False, 'error': str(e)})
    
# Endpoint to start/stop ID reading
@app.route('/IDreadingStatus/<int:state>', methods=['POST'])
def id_status(state):
    global stop_threads, id_thread, book_thread

    if state == 1:  # Start ID reading
        # Stop any running book reading thread before starting the ID thread
        if book_thread is not None and book_thread.is_alive():
            stop_threads.set()
            book_thread.join()
            print("Book reader thread stopped for ID reader to start...")
        
        # Stop any running ID thread before starting a new one
        if id_thread is not None and id_thread.is_alive():
            stop_threads.set()
            id_thread.join()
            print("ID reader thread stopped...")

        stop_threads.clear()
        id_thread = threading.Thread(target=get_id_read_data, daemon= True)
        id_thread.start()
        print("ID reader thread started...")

        return {"status": "ID reading started"}, 200
    
    elif state == 0:  # Stop ID reading
        stop_threads.set()
        if id_thread is not None and id_thread.is_alive():
            id_thread.join()
            print("ID reader thread stopped...")
        return jsonify({'status': 'ID read thread terminated'}), 200
    
    else:
        return jsonify({'status': "invalid state"}), 400
    
# Endpoint to start/stop book reading
@app.route('/BookreadingStatus/<int:state>', methods=['POST'])
def book_status(state):
    global stop_threads, id_thread, book_thread

    if state == 1:  # Start book reading
        # Stop any running ID reading thread before starting the book thread
        if id_thread is not None and id_thread.is_alive():
            stop_threads.set()
            id_thread.join()
            print("ID reader thread stopped for Book reader to start...")
        
        # Stop any running book thread before starting a new one
        if book_thread is not None and book_thread.is_alive():
            stop_threads.set()
            book_thread.join()
            print("Book reader thread stopped...")

        stop_threads.clear()
        book_thread = threading.Thread(target=get_book_read_data, daemon=True)
        book_thread.start()
        print("Book reader thread started...")

        return {"status": "Book reading started"}, 200
    
    elif state == 0:  # Stop book reading
        stop_threads.set()
        if book_thread is not None:
            book_thread.join()
            print("Book reader thread stopped...")
        return jsonify({'status': 'Book read thread terminated'}), 200
    
    else:
        return jsonify({'status': "invalid state"}), 400

# Endpoint to disconnect from the serial port
@app.route('/breakConnection', methods=['POST'])
def break_connection():
    global ser, stop_threads

    stop_threads.set()
    if ser.is_open:
        ser.close()

    return jsonify({'status': 'Connections terminated'})

# Endpoint to get the latest book data
@app.route('/getBookData', methods=['GET'])
def get_book_data():
    return jsonify({
        'book_data': book_data
    })

# Endpoint to get the latest ID data
@app.route('/getIDData', methods=['GET'])
def get_id_data():
    return jsonify({
        'id_data': id_data
    })

# Endpoint to handle posting of data
@app.route('/postData', methods=['POST'])
def postData():
    response = request.get_json()
    if 'id' in response:
        print(f"Received data: {response['id']}")
        return jsonify({'received data': response['id']})
    else:
        print("post failed")
        return jsonify({'status': "failed"})

# Run the Flask app with SocketIO
if __name__ == '__main__':
    kisokConnect()
    socketio.run(app, host='0.0.0.0', port=80)