document.addEventListener("DOMContentLoaded", function () {
    const startIDReadingBtn = document.getElementById("startIDReading");
    const startBookReadingBtn = document.getElementById("startBookReading");
    const stopKeystrokeBtn = document.getElementById("stopKeystroke");
    const connectCOM = document.getElementById("connectCOM");
    const disconnectCOM = document.getElementById("disconnectCOM");
    const IDcard = document.getElementById("idCardData");
    const Book = document.getElementById("bookData");

    let idstatus = false;
    let bookstatus = false;
  
    const socket = io()
  
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
  
    // Update book data function call
    async function getBookData() {
      try {
        const response = await fetch("/getBookData");
        const result = await response.json();
        Book.value = result.book_data;      
  
      } catch (error) {
        console.error("Error fetching read data:", error);
      }
    }
  
    // Update id data function call
    async function getIDData() {
      try {
        const response = await fetch("/getIDData");
        const result = await response.json();
        IDcard.value = result.id_data;      
  
      } catch (error) {
        console.error("Error fetching read data:", error);
      }
    }
  
    // Load the ports into both the drop down menus
    function loadPorts() {
      fetch('/ports')
        .then(response => response.json())
        .then(data => {
          const portList = document.getElementById('portList');
          portList.innerHTML = '';
  
          data.ports.forEach(port => {
            const option = document.createElement('option');
            option.value = port;
            option.text = port;
            portList.appendChild(option);
          });
  
        })
        .catch(error => {
          console.error('Error fetching ports:', error);
        });
    }
  
    // Upon selecting the ports and pressing connect, send the serialports and start the listener
    function connectSerialPort() {
      const selectedPort = document.getElementById('portList').value;
      fetch('/libraryPortConnect', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ selectedPort: selectedPort}),
      })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            // enable the buttons once the listener is established
            startIDReadingBtn.disabled = false;
            startBookReadingBtn.disabled = false;
            stopKeystrokeBtn.disabled = false;
  
            connectCOM.classList.add("active");
            connectCOM.textContent = "Connection Established";
  
            alert('Connected to ' + selectedPort);
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
  
    // Fetch endpoint to start the ID thread
    startIDReadingBtn.addEventListener("click", async (event) => {
      event.preventDefault();

      idstatus = true;
      // Remove active class and stop book reading if book reading is active while clicking start id reading
      if (bookstatus) {
        bookstatus = false;
        // Restore Book button
        startBookReadingBtn.textContent = "Start Book Reading";
        startBookReadingBtn.classList.remove("active");
        startBookReadingBtn.disabled = false;

        sendCommand("/BookreadingStatus/0");
        await new Promise((r) => setTimeout(r, 500));
      }
      
      sendCommand("/IDreadingStatus/1");
      startIDReadingBtn.disabled = true;
      startIDReadingBtn.textContent = "Read Active";
      startIDReadingBtn.classList.add("active");
    });
  
    // Fetch endpoint to start the Book thread
    startBookReadingBtn.addEventListener("click", async (event) => {
      event.preventDefault();

      bookstatus = true;
      // Remove active class and stop id reading if id reading is active while clicking start book reading
      if (idstatus) {
        idstatus = false;
        // Restore ID button
        startIDReadingBtn.textContent = "Start ID Reading";
        startIDReadingBtn.classList.remove("active");
        startIDReadingBtn.disabled = false;

        sendCommand("/IDreadingStatus/0");
        await new Promise((r) => setTimeout(r, 500));
      }
      sendCommand("/BookreadingStatus/1");
  
      startBookReadingBtn.disabled = true;
      startBookReadingBtn.textContent = "Read Active";
      startBookReadingBtn.classList.add("active");
    });
  
    // Fetch endpoint to stop the keystrokes thread
    stopKeystrokeBtn.addEventListener("click", async (event) => {
      event.preventDefault();
      idstatus = false;
      bookstatus = false;
      sendCommand("/IDreadingStatus/0");
      // Delay to mitigate serial conflict
      await new Promise((r) => setTimeout(r, 500));
      sendCommand("/BookreadingStatus/0");
  
  
      // Restore ID button
      startIDReadingBtn.textContent = "Start ID Reading";
      startIDReadingBtn.classList.remove("active");
      startIDReadingBtn.disabled = false;
  
      // Restore Book button
      startBookReadingBtn.textContent = "Start Book Reading";
      startBookReadingBtn.classList.remove("active");
      startBookReadingBtn.disabled = false;
    });
  
    // Dynamic id data updation
    socket.on('id_changed', function(data){
      const updated_value = data.new_id;
      IDcard.value = updated_value
    })
  
    // Dynamic book data updation
    socket.on('book_changed', function(data){
      const updated_value = data.new_book;
      Book.value = updated_value
    })
  
    loadPorts();
    // setInterval(getBookData, 500);
    // setInterval(getIDData, 500);
  
  });
  