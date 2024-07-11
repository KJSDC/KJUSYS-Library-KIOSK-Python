import mysql.connector
import mysql
from flask import Flask, render_template, jsonify, request, redirect, url_for
import serial
import time
import threading
import pyautogui

app = Flask(__name__)

############################################################
######## START FUNCTION FOR AUTOMATICALLY FETCHING UID######
############################################################
try:
    ser = serial.Serial('COM5', 115200, timeout=1)
except serial.SerialException as e:
    ser = None
    print(f"Failed to connect to serial port: {e}")

# Global variable to store the last read UID
auth = ""
last_uid = ""
read_active = True
write_active = False

# DB creds and initialization
mydb = mysql.connector.connect(
  host="localhost",
  user="root",
  password="tiger",
  database ="lms"
)
mycursor = mydb.cursor()

# Decimal to endian to standard conversion
def comCardDecoder(decimal_number):
    hex_string = format(decimal_number, '08x')

    endian = [hex_string[i:i+2] for i in range(0, len(hex_string), 2)]
    endian.reverse()
    endian = ''.join(endian)

    standard = [endian[i:i+2] for i in range(0, len(endian), 2)]
    standard_format = ' '.join(standard).upper()

    print(f"\nTo Standard Format: {standard_format}\n")
    return standard_format

def insertdata(uid, rollno):
    try:
        while True:
            # Read RFID
            uid = read_rfid(ser)
            if uid:
                # Once a valid UID is read, prompt for additional input
                no = input("Enter data: ")
                para = (uid, no)

                # SQL query for inserting data into table
                sqlinput = "INSERT INTO details (uid, no) VALUES (%s, %s)"
                try:
                    # Inserting data
                    mycursor.execute(sqlinput, para)
                    mydb.commit()
                    print(f"{mycursor.rowcount} record(s) inserted.")
                except mysql.connector.Error as err:
                    print(f"Error: {err}")
                # Adding a small delay to avoid rapid continuous polling
                time.sleep(1)
    finally:
        # Close cursor and connection
        mycursor.close()
        mydb.close()
        # Close the serial port
        ser.close()

def read_rfid():
    global last_uid
    if ser is None:
        print("Serial port not available.")
        return
    
    flag = {
        "ets Jul 29 2019 12:21:46", "rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)", 
        "configsip: 0, SPIWP:0xee", "clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00", 
        "mode:DIO, clock div:1", "load:0x3fff0030,len:1184", "load:0x40078000,len:13260", 
        "load:0x40080400,len:3028", "entry 0x400805e4"
    }

    while read_active:
        if ser.in_waiting > 0:
            try:
                rfid_tag = ser.readline().strip().decode('utf-8')  # Read and decode the RFID tag
                if any(flag_str in rfid_tag for flag_str in flag):
                    continue
                else:
                    last_uid = rfid_tag.strip()
                    print("\nRead Successfully: " + last_uid)
            except UnicodeDecodeError:
                print("Failed to decode RFID tag")

# Start a background thread to read RFID data if the serial port is available
if ser is not None:
    thread = threading.Thread(target=read_rfid)
    thread.daemon = True
    thread.start()

############################################################
######## END FUNCTION FOR AUTOMATICALLY FETCHING UID########
############################################################

############################################################
######## SEARCH AND KEYSTROKE ENTRY FUNCTION ###############
############################################################

cursor = mydb.cursor()

# Function for passing the query
def query_database(uid, cursor):
    cursor.execute("SELECT no FROM details WHERE uid = %s", (uid,))
    return cursor.fetchone()


############################################################
######## SEARCH AND KEYSTROKE ENTRY FUNCTION END ###########
############################################################

# AUTHENTICATION

@app.route('/auth', methods=['POST'])
def auth():
    global auth
    auth = request.form[auth]
    if auth:
        return jsonify({'status': 'success'})
    else:
        return jsonify({'status': 'failed'})

# AUTHENTICATION END

# HOMEPAGE

@app.route('/')
def index():
    return render_template('index.html')

# HOMEPAGE END

# MAPPING PAGE

@app.route('/map')
def map():
    return render_template('map.html')

# MAPPING PAGE END

# CRUD PAGE AND LOAD DATA

@app.route('/crud')
def crud():
    mycursor = mydb.cursor()
    try:
        mycursor.execute('SELECT * FROM details')
        users = mycursor.fetchall()
        for user in users:
            print(f"User UID: {user[0]}, User No: {user[1]}")  # Debug output
    except Exception as e:
        print(f"Error: {e}")
        return "An error occurred"
    finally:
        mycursor.close()
    return render_template('crud.html', users=users)

# CRUD PAGE AND LOAD DATA END

# CRUD PAGE UPDATE FUNCTION

@app.route('/update_user/<uid>', methods=['POST'])
def update_user(uid):
    name = request.form['name']
    mycursor = mydb.cursor()
    try:
        mycursor.execute('UPDATE details SET no=%s WHERE uid=%s', (name, uid))
        mydb.commit()
    except Exception as e:
        print(f"Error: {e}")
        return "An error occurred"
    finally:
        mycursor.close()
    return redirect(url_for('crud'))

# CRUD PAGE UPDATE FUNCTION END

# CRUD PAGE DELETE FUNCTION

@app.route('/delete_user/<uid>')
def delete_user(uid):
    mycursor = mydb.cursor()
    try:
        mycursor.execute('DELETE FROM details WHERE uid=%s', (uid,))
        mydb.commit()
    except Exception as e:
        print(f"Error: {e}")
        return "An error occurred"
    finally:
        mycursor.close()
    return redirect(url_for('crud'))

# CRUD PAGE DELETE FUNCTION END

#######################################################
######## END POINT FOR AUTOMATICALLY FETCHING UID######
#######################################################

@app.route('/get_uid')
def get_uid():
    global last_uid
    if last_uid:
        return jsonify({'uid': last_uid})
    else:
        return jsonify({'uid': ''})

#############################################################
######## CLOSE END POINT FOR AUTOMATICALLY FETCHING UID######
#############################################################


############################################################
######## PUSHING THE DATA TO THE DATABASE ##################
############################################################

@app.route('/postdata', methods=['POST'])
def postdata():
    data = request.get_json()
    rollno = data.get('rollno', '')
    uid = data.get('uid', '')
    print(f"Received rollno: {rollno}\nReceived UID: {uid}")

    try:
        para = (uid, rollno)
        query = "INSERT INTO details (uid, no) VALUES (%s, %s)"

        mycursor.execute(query, para)
        mydb.commit()
        print(f"{mycursor.rowcount} record(s) inserted.")
        return jsonify({'status': 'success', 'received_rollno': rollno, 'received_uid':uid})
    
    except mysql.connector.Error as err:
        print(f"Error: {err}")
        return jsonify({'status': 'failed', 'received_rollno': rollno, 'received_uid':uid})

############################################################
######## CLOSE PUSHING THE DATA TO THE DATABASE ############
############################################################

############################################################
######## START SEARCH AND KEYSTROKE FUNCTION ###############
############################################################

@app.route('/keystrokeData', methods=['POST'])
def keystrokeData():
    global read_active
    global write_active

    data = request.get_json()
    counter = data.get('ctr', '')

    if counter == 0:
        read_active = True
        write_active = False

        print("Write Stopped")
        print(f"write_active: {write_active}, read_active: {read_active}")

        thread = threading.Thread(target=read_rfid)
        thread.daemon = True
        thread.start()

        return jsonify({'status': 'stopped'})
    else:
        read_active = False
        write_active = True

    print(f"Counter: {counter}")

    try:
        while write_active:
            if ser.in_waiting > 0:
                uid = ser.readline().decode('utf-8').strip()
                print(f"Received UID: {uid}")
                
                result = query_database(uid, cursor)
                if result:
                    roll_number = str(result[0])
                    print(f"Roll number: {roll_number}")

                    time.sleep(0.5)

                    pyautogui.typewrite(roll_number)
                    pyautogui.press('enter')
                    if counter == 1:
                        return keystrokeData()
                    else:
                        write_active = False
                        return jsonify({'status': 'stopped'})
                else:
                    print("UID not found in the database.")
                    if counter == 1:
                        return keystrokeData()
                    else:
                        write_active = False
                        return jsonify({'status': 'stopped'})
        
        return jsonify({'status': 'stopped'})

    except Exception as e:
        print(f"Error: {e}")
        write_active = False
        return jsonify({'status': 'failed', 'error': str(e)})

############################################################
######## START SEARCH AND KEYSTROKE FUNCTION ###############
############################################################

if __name__ == '__main__':
    app.run()

