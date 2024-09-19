// Wait for the DOM to fully load before executing the script
document.addEventListener("DOMContentLoaded", function () {
  // Get references to various buttons and input fields
  const startIDReadingBtn = document.getElementById("startIDReading");
  const startBookReadingBtn = document.getElementById("startBookReading");
  const stopKeystrokeBtn = document.getElementById("stopKeystroke");
  const connectCOM = document.getElementById("connectCOM");
  const disconnectCOM = document.getElementById("disconnectCOM");
  const IDcard = document.getElementById("idCardData");
  const Book = document.getElementById("bookData");

  // Initialize status flags for ID and book reading
  let idstatus = false;
  let bookstatus = false;

  // Initialize a Socket.IO connection
  const socket = io();

  ////////////////////////////
  // FUNCTION DEFINITIONS
  ////////////////////////////

  // Universal function to send a POST request to a specified URL
  async function sendCommand(url) {
    try {
      const response = await fetch(url, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
      });
      const result = await response.json();
      console.log(result.status); // Log the response status
    } catch (error) {
      console.error("Error:", error); // Log any errors
    }
  }

  // Function to fetch and update book data in the input field
  async function getBookData() {
    try {
      const response = await fetch("/getBookData");
      const result = await response.json();
      Book.value = result.book_data; // Update the book data input field
    } catch (error) {
      console.error("Error fetching read data:", error); // Log any errors
    }
  }

  // Function to fetch and update ID data in the input field
  async function getIDData() {
    try {
      const response = await fetch("/getIDData");
      const result = await response.json();
      IDcard.value = result.id_data; // Update the ID data input field
    } catch (error) {
      console.error("Error fetching read data:", error); // Log any errors
    }
  }

  // Function to load available serial ports into a dropdown menu
  function loadPorts() {
    fetch('/ports')
      .then(response => response.json())
      .then(data => {
        const portList = document.getElementById('portList');
        portList.innerHTML = ''; // Clear existing options

        // Populate the dropdown with available ports
        data.ports.forEach(port => {
          const option = document.createElement('option');
          option.value = port;
          option.text = port;
          portList.appendChild(option);
        });
      })
      .catch(error => {
        console.error('Error fetching ports:', error); // Log any errors
      });
  }

  // Function to connect to the selected serial port
  function connectSerialPort() {
    const selectedPort = document.getElementById('portList').value; // Get selected port
    fetch('/libraryPortConnect', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ selectedPort: selectedPort }), // Send the selected port
    })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          // Enable buttons if connection is successful
          startIDReadingBtn.disabled = false;
          startBookReadingBtn.disabled = false;
          stopKeystrokeBtn.disabled = false;

          connectCOM.classList.add("active"); // Update button appearance
          connectCOM.textContent = "Connection Established"; // Update button text

          alert('Connected to ' + selectedPort); // Notify user
        } else {
          alert('Failed to connect: ' + data.error); // Notify failure
        }
      })
      .catch(error => {
        console.error('Error:', error); // Log any errors
      });
  }

  ////////////////////////////////
  // EVENT LISTENERS
  ////////////////////////////////

  // Connect serial port when button is clicked
  connectCOM.addEventListener("click", (event) => {
    event.preventDefault();
    connectSerialPort();
    connectCOM.disabled = true; // Disable button after click
  });

  // Disconnect and terminate threads when button is clicked
  disconnectCOM.addEventListener("click", (event) => {
    event.preventDefault();
    sendCommand("/breakConnection");
    connectCOM.disabled = false; // Enable connect button
    connectCOM.classList.remove("active"); // Update button appearance
    connectCOM.textContent = "Connect"; // Reset button text
  });

  // Start ID reading when button is clicked
  startIDReadingBtn.addEventListener("click", async (event) => {
    event.preventDefault();

    idstatus = true; // Update status flag
    // Stop book reading if it's active
    if (bookstatus) {
      bookstatus = false;
      startBookReadingBtn.textContent = "Start Book Reading"; // Reset button text
      startBookReadingBtn.classList.remove("active"); // Remove active class
      startBookReadingBtn.disabled = false; // Enable button
      sendCommand("/BookreadingStatus/0"); // Stop book reading
      await new Promise((r) => setTimeout(r, 500)); // Delay to ensure command completion
    }
    
    sendCommand("/IDreadingStatus/1"); // Start ID reading
    startIDReadingBtn.disabled = true; // Disable button
    startIDReadingBtn.textContent = "Read Active"; // Update button text
    startIDReadingBtn.classList.add("active"); // Add active class
  });

  // Start book reading when button is clicked
  startBookReadingBtn.addEventListener("click", async (event) => {
    event.preventDefault();

    bookstatus = true; // Update status flag
    // Stop ID reading if it's active
    if (idstatus) {
      idstatus = false;
      startIDReadingBtn.textContent = "Start ID Reading"; // Reset button text
      startIDReadingBtn.classList.remove("active"); // Remove active class
      startIDReadingBtn.disabled = false; // Enable button
      sendCommand("/IDreadingStatus/0"); // Stop ID reading
      await new Promise((r) => setTimeout(r, 500)); // Delay to ensure command completion
    }
    sendCommand("/BookreadingStatus/1"); // Start book reading

    startBookReadingBtn.disabled = true; // Disable button
    startBookReadingBtn.textContent = "Read Active"; // Update button text
    startBookReadingBtn.classList.add("active"); // Add active class
  });

  // Stop both reading processes when button is clicked
  stopKeystrokeBtn.addEventListener("click", async (event) => {
    event.preventDefault();
    idstatus = false; // Update status flag
    bookstatus = false; // Update status flag
    sendCommand("/IDreadingStatus/0"); // Stop ID reading
    await new Promise((r) => setTimeout(r, 500)); // Delay to ensure command completion
    sendCommand("/BookreadingStatus/0"); // Stop book reading

    // Restore ID button appearance
    startIDReadingBtn.textContent = "Start ID Reading";
    startIDReadingBtn.classList.remove("active");
    startIDReadingBtn.disabled = false;

    // Restore Book button appearance
    startBookReadingBtn.textContent = "Start Book Reading";
    startBookReadingBtn.classList.remove("active");
    startBookReadingBtn.disabled = false;
  });

  // Update ID data dynamically via WebSocket
  socket.on('id_changed', function(data) {
    const updated_value = data.new_id; // Get new ID data
    IDcard.value = updated_value; // Update the ID data input field
  });

  // Update book data dynamically via WebSocket
  socket.on('book_changed', function(data) {
    const updated_value = data.new_book; // Get new book data
    Book.value = updated_value; // Update the book data input field
  });

  loadPorts(); // Load available ports on page load
  // setInterval(getBookData, 500); // (Commented out) Periodically fetch book data
  // setInterval(getIDData, 500); // (Commented out) Periodically fetch ID data
});
