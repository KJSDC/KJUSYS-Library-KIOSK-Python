from flask import Flask, request, jsonify, render_template
import serial
import threading
import serial.tools.list_ports
import pyautogui
import time

app = Flask(__name__)

# Global variables for serial ports and threads
id_serial_port = None
book_serial_port = None
id_thread = None
book_thread = None
stop_threads = False

# Global variables to store the latest data from each Arduino
id_data = ""
book_data = ""

# Commands
read_command = "-"
stop_read_command = "_"
write_command = ";"
stop_write_command = ":"

# Function to send commands to ID serial
def ID_serial_write(command):
    if id_serial_port:
        id_serial_port.write(command.encode())
        print(f"Command sent to serial: {command}")

# Function to send commands to Book serial
def Book_serial_write(command):
    if book_serial_port:
        book_serial_port.write(command.encode())
        print(f"Command sent to serial {command}")


# Function to handle reading data from the ID card serial port
def read_id_data():
    global stop_threads, id_serial_port, id_data
    while not stop_threads:
        try:
            # if id_serial_port and id_serial_port.is_open:
            #     data = id_serial_port.readline().decode('utf-8').strip()
            #     print("ID Data read:", data)
            #     id_data = data  # Update the latest ID card data
            if id_serial_port.in_waiting > 0:
                id_data = id_serial_port.readline().strip().decode('utf-8')
                
                if id_data == "PCD_Authenticate() failed: Error in communication." or id_data == "PCD_Authenticate() failed: Timeout in communication." or id_data == "MIFARE_Read() failed: The CRC_A does not match." :
                    print(f"Encountered error: {id_data}")
                else:
                    time.sleep(0.5)
                    pyautogui.typewrite(id_data)
                    pyautogui.press('enter')
                    print(f"Read and Typed data: {id_data}")

        except serial.SerialException as e:
            print("Error reading from ID serial port:", e)
            break

# Function to handle reading data from the Book Reader serial port
def read_book_data():
    global stop_threads, book_serial_port, book_data
    while not stop_threads:
        try:
            # if book_serial_port and book_serial_port.is_open:
            #     data = book_serial_port.readline().decode('utf-8').strip()
            #     print("Book Data read:", data)
            #     book_data = data  # Update the latest Book Reader data
            if book_serial_port.in_waiting > 0:
                book_data = book_serial_port.readline().strip().decode('utf-8')

                if book_data == "PCD_Authenticate() failed: Error in communication." or book_data == "PCD_Authenticate() failed: Timeout in communication." or book_data == "MIFARE_Read() failed: The CRC_A does not match." :
                    print(f"Encountered error: {id_data}")
                else:
                    time.sleep(0.5)
                    pyautogui.typewrite(book_data)
                    pyautogui.press('enter')
                    print(f"Read and Typed data: {book_data}")

        except serial.SerialException as e:
            print("Error reading from Book serial port:", e)
            break

# Main page
@app.route('/')
def index():
    return render_template('library.html')

@app.route('/ports', methods=['GET'])
def get_ports():
    ports = [port.device for port in serial.tools.list_ports.comports()]
    return jsonify({'ports': ports})

@app.route('/libraryPortConnect', methods=['POST'])
def connect_ports():
    global id_serial_port, book_serial_port, id_thread, book_thread
    data = request.json

    id_port_name = data.get('IdPport')
    book_port_name = data.get('BookPort')

    try:
        # Establish connections to both serial ports
        id_serial_port = serial.Serial(id_port_name, baudrate=9600, timeout=1)
        book_serial_port = serial.Serial(book_port_name, baudrate=9600, timeout=1)

        # Start threads to handle data reading
        stop_threads = False

        return jsonify({'success': True})
    except serial.SerialException as e:
        return jsonify({'success': False, 'error': str(e)})

@app.route('/startLibraryKeystroke/<int:state>', methods=['POST'])
def start_keystroke(state):
    global stop_threads, id_serial_port, book_serial_port, id_thread, book_thread

    if state == 1:  # Start keystroke
        # Update the counter
        stop_threads = False

        # Start the threads
        id_thread = threading.Thread(target=read_id_data)
        book_thread = threading.Thread(target=read_book_data)

        id_thread.start()
        book_thread.start()

        # Send commands to start the read function
        ID_serial_write(read_command)
        time.sleep(0.5)
        Book_serial_write(read_command)    
        return jsonify({'status': 'Keystroke threads running'})
    
    elif state == 0:  # Stop keystroke
        # Update the counter
        stop_threads = True

        # Send commands to stop the read function
        ID_serial_write(stop_read_command)
        time.sleep(0.5)
        Book_serial_write(stop_read_command)   

        # Stop the threads
        if id_thread is not None:
            id_thread.join()
        if book_thread is not None:
            book_thread.join()
        return jsonify({'status': 'Keystroke threads stopped'})
    
    else:
        return jsonify({'status': 'Invalid state'})

@app.route('/breakConnection', methods=['POST'])
def break_connection():
    global id_serial_port, book_serial_port, stop_threads

    stop_threads = True
    if id_serial_port and id_serial_port.is_open:
        id_serial_port.close()
    if book_serial_port and book_serial_port.is_open:
        book_serial_port.close()

    return jsonify({'status': 'Connections terminated'})

@app.route('/getReadData', methods=['GET'])
def get_read_data():
    # Return the latest data from both Arduinos
    return jsonify({
        'id_data': id_data,
        'book_data': book_data
    })

if __name__ == '__main__':
    app.run()
