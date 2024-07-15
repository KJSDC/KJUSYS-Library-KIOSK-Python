from flask import Flask, render_template, jsonify, request
import serial
import threading

app = Flask(__name__)

read_command = "-"
stop_read_command = "_"
write_command = ";"
stop_write_command = ":"

# Global variables to store the last read UID and data
last_read_data = ""
last_read_uid = ""

try:
    ser = serial.Serial('COM5', 115200, timeout=1)
    print("Serial port connected.")
except serial.SerialException as e:
    ser = None
    print(f"Failed to connect to serial port: {e}")

# Serial write function to send commands to the serial port
def serial_write(command):
    if ser:
        ser.write(command.encode())
        print(f"Command sent to serial: {command}")

# Function to read data from the serial port
def read_from_serial():
    global last_read_data, last_read_uid

    if ser is None:
        print("Serial port not available.")
        return
    
    flag = {
        "ets Jul 29 2019 12:21:46", "rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)", 
        "configsip: 0, SPIWP:0xee", "clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00", 
        "mode:DIO, clock div:1", "load:0x3fff0030,len:1184", "load:0x40078000,len:13260", 
        "load:0x40080400,len:3028", "entry 0x400805e4"
    }
    
    while True:
        if ser.in_waiting > 0:
            try:
                # Read UID
                UID = ser.readline().strip().decode('utf-8')
                if any(flag_str in UID for flag_str in flag):
                    continue
                else:
                    last_read_uid = UID
                    print(f"Read UID: {last_read_uid}")
                
                # Read Data
                Data = ser.readline().strip().decode('utf-8')
                if any(flag_str in Data for flag_str in flag):
                    continue
                else:
                    last_read_data = Data
                    print(f"Read Data: {last_read_data}")

            except UnicodeDecodeError:
                print("Failed to decode RFID tag")

# Start a background thread to read RFID data if the serial port is available
if ser is not None:
    thread = threading.Thread(target=read_from_serial)
    thread.daemon = True
    thread.start()

# Main page
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/startRead', methods=['POST'])
def startRead():
    try:
        serial_write(read_command)
        return jsonify({'status': 'read started'})
    except Exception as e:
        print(f"Exception occurred while starting read: {e}")
        return jsonify({'status': 'failed', 'error': str(e)}), 500

@app.route('/stopRead', methods=['POST'])
def stopRead():
    try:
        serial_write(stop_read_command)
        return jsonify({'status': 'read stopped'})
    except Exception as e:
        print(f"Exception occurred while stopping read: {e}")
        return jsonify({'status': 'failed', 'error': str(e)}), 500

@app.route('/startWrite', methods=['POST'])
def startWrite():
    try:
        serial_write(write_command)
        return jsonify({'status': 'write started'})
    except Exception as e:
        print(f"Exception occurred while starting write: {e}")
        return jsonify({'status': 'failed', 'error': str(e)}), 500

@app.route('/stopWrite', methods=['POST'])
def stopWrite():
    try:
        serial_write(stop_write_command)
        return jsonify({'status': 'write stopped'})
    except Exception as e:
        print(f"Exception occurred while stopping write: {e}")
        return jsonify({'status': 'failed', 'error': str(e)}), 500

# Endpoint to send data to be written to the EEPROM
@app.route('/newWrite/<data>', methods=['POST'])
def newWrite(data):
    if data:
        serial_write(data + "\n")
        return jsonify({'status': 'success'})
    else:
        return jsonify({'status': 'failed', 'error': 'No data provided'}), 400

# Endpoint to get the last read data
@app.route('/getReadData')
def getReadData():
    return jsonify({'uid': last_read_uid, 'data': last_read_data})

if __name__ == '__main__':
    app.run()
