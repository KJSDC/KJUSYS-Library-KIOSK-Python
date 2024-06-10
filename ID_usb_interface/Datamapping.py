import mysql.connector
import serial
import serial.tools.list_ports

def find_serial_port():
    # List all available serial ports
    ports = serial.tools.list_ports.comports()
    for port in ports:
        try:
            # Try to open each port
            ser = serial.Serial(port.device, 115200, timeout=1)
            ser.close()
            return port.device
        except (OSError, serial.SerialException):
            continue
    return None

# Function to read RFID tag
def read_rfid(ser):
    while True:
        print("Place your RFID tag near the reader...")
        rfid_tag = ser.readline().strip().decode('utf-8')  # Read and decode the RFID tag
        if rfid_tag:
            print("Scanned successfully")
            return rfid_tag.strip()
       

# Find the correct serial port
port = find_serial_port()
if port is None:
    print("No serial ports found.")
    exit(1)

# Open the serial port
ser = serial.Serial(port, 115200, timeout=1)


#Connecting to database
mydb = mysql.connector.connect(
  host="localhost",
  user="root",
  password="tiger",
  database ="lms"
)
mycursor = mydb.cursor()

#Show all Tables
mycursor.execute("SHOW TABLES")
print("\nTable name in database lms")
for x in mycursor:
    print(x)

# Show column names in table details
print("\nColumn names in table details are")
mycursor.execute("SHOW COLUMNS FROM details")
for column in mycursor.fetchall():
    print(column)

#taking values 
try:
    while True:
        # Read RFID
        uid = read_rfid(ser)
        if uid:
            # Once a valid UID is read, prompt for additional input
            no = input("Enter no (integer): ")
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

            # SQL query for selecting data from table
            mycursor.execute("SELECT * FROM details")
            print("\nRows in 'details' table:")
            for row in mycursor.fetchall():
                print(row)
            
            # Prompt the user to continue or end the program
            continue_prompt = input("Do you want to scan another card? (y/n): ").strip().lower()
            if continue_prompt == 'n':
                print("Ending the program.")
                break
            # Adding a small delay to avoid rapid continuous polling
            time.sleep(1)
finally:
    # Close cursor and connection
    mycursor.close()
    mydb.close()
    # Close the serial port
    ser.close()