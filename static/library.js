document.addEventListener("DOMContentLoaded", function () {
  const startKeystrokeBtn = document.getElementById("startKeystroke");
  const stopKeystrokeBtn = document.getElementById("stopKeystroke");
  const connectCOM = document.getElementById("connectCOM");
  const disconnectCOM = document.getElementById("disconnectCOM");
  const IDcard = document.getElementById("idCardData");
  const Book = document.getElementById("bookData");

  ////////////////////////////
  // FUNCTION DEFINITIONS
  ////////////////////////////

  // Universal function to fetch endpoints
  async function sendCommand(url) {
    try {
      const response = await fetch(url, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
      });
      const result = await response.json();
      console.log(result.status);
    } catch (error) {
      console.error("Error:", error);
    }
  }

  // Update read data function call
  async function getReadData() {
    try {
      const response = await fetch("/getReadData");
      const result = await response.json();
      // if (result.data == "PCD_Authenticate() failed: Error in communication." || result.data == "PCD_Authenticate() failed: Timeout in communication." || result.data == "MIFARE_Read() failed: The CRC_A does not match.") {
      //   document.getElementById("cardUID").value = "Try again";
      //   await sendCommand("/startLibraryKeystroke/1");
      // }
      // else {
      //   // document.getElementById("cardUID").value = result.uid;
      //   document.getElementById("cardData").value = result.data;
      // }
      IDcard.value = result.id_data;
      Book.value = result.book_data;      

    } catch (error) {
      console.error("Error fetching read data:", error);
    }
  }

  // Load the ports into both the drop down menus
  function loadPorts() {
    fetch('/ports')
      .then(response => response.json())
      .then(data => {
        const idport = document.getElementById('IDport');
        idport.innerHTML = '';

        const bookport = document.getElementById('BookPort');
        bookport.innerHTML = '';

        data.ports.forEach(port => {
          const option = document.createElement('option');
          option.value = port;
          option.text = port;
          idport.appendChild(option);
        });

        data.ports.forEach(port => {
          const option = document.createElement('option');
          option.value = port;
          option.text = port;
          bookport.appendChild(option);
        });
      })
      .catch(error => {
        console.error('Error fetching ports:', error);
      });
  }

  // Upon selecting the ports and pressing connect, send the serialports and start the listener
  function connectSerialPort() {
    const IDcomPort = document.getElementById('IDport').value;
    const BookcomPort = document.getElementById('BookPort').value;
    fetch('/libraryPortConnect', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ IdPport: IDcomPort, BookPort: BookcomPort }),
    })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          // enable the buttons once the listener is established
          startKeystrokeBtn.disabled = false;
          stopKeystrokeBtn.disabled = false;

          connectCOM.classList.add("active");
          connectCOM.textContent = "Connection Established";

          alert('Connected to ' + IDcomPort + ' and ' + BookcomPort);
        } else {
          alert('Failed to connect: ' + data.error);
        }
      })
      .catch(error => {
        console.error('Error:', error);
      });
  }

  ////////////////////////////////
  // EVENT LISTENERS
  ////////////////////////////////

  // Sending connectSerialPort function call on pressing connect
  connectCOM.addEventListener("click", (event) => {
    event.preventDefault();
    connectSerialPort();
    connectCOM.disabled = true;
  });

  // Fetch endpoint to terminate both the threads that are running
  disconnectCOM.addEventListener("click", (event) => {
    event.preventDefault()
    sendCommand("/breakConnection")
    connectCOM.disabled = false;
    connectCOM.classList.remove("active");
    connectCOM.textContent = "Connect";
  });

  // Fetch endpoint to start the keystrokes thread
  startKeystrokeBtn.addEventListener("click", (event) => {
    event.preventDefault();
    sendCommand("/startLibraryKeystroke/1");

    startKeystrokeBtn.disabled = true;
    startKeystrokeBtn.textContent = "Keystrokes Running";
    startKeystrokeBtn.classList.add("active");
  });

  // Fetch endpoint to stop the keystrokes thread
  stopKeystrokeBtn.addEventListener("click", (event) => {
    event.preventDefault();
    sendCommand("/startLibraryKeystroke/0");

    startKeystrokeBtn.textContent = "Start Reading";
    startKeystrokeBtn.classList.remove("active");
    startKeystrokeBtn.disabled = false;
  });

  loadPorts();
  setInterval(getReadData, 500);
});
