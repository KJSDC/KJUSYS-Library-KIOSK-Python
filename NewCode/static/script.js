document.addEventListener("DOMContentLoaded", async function () {
  async function getReadData() {
    let uidField = document.getElementById("cardUID");
    let dataField = document.getElementById("cardData");
  
    try {
      const response = await fetch('http://127.0.0.1:5000/getReadData');
      const result = await response.json();
  
      if (!result.uid || !result.data) {
        console.log("Error in fetching data");
      } else {
        console.log("Data received, uid: " + result.uid + " data: " + result.data);
        uidField.value = result.uid;
        dataField.value = result.data;
      }
    } catch (error) {
      console.error(error);
    }
  }

  setInterval(getReadData, 500);

  try {
    response = await fetch("http://127.0.0.1:5000/startRead");
    result = await response.json();
    console.log(result.status);
  }
  catch (e) {
    console.log("Exception has occured: "&e)
  }
});

async function newWriteFunction(event) {
  event.preventDefault(); // Prevent default form submission

  let data = document.getElementById("newData").value;
  let url = "http://127.0.0.1:5000/newWrite/" + data;

  try {
    const response = await fetch(url, {
      method: 'POST', // Ensure it is a POST request
      headers: {
        Accept: "application/json",
        "Content-Type": "application/json"
      }
    });
    const result = await response.json();

    if (result.status == 'success') {
      console.log("New data has been written");
      window.alert("Data Updated Successfully");
    } else {
      console.log("New data was not written due to an error, check server logs");
      window.alert("Data Updation Failed");
    }
  } catch (error) {
    console.error(error);
  }
}

// Animation and read write functions

const container = document.getElementById("container");
const Read = document.getElementById("startRead");
const Write = document.getElementById("startWrite");
const writeData = document.getElementById("writeData");
// const startReading = document.getElementById("startReading");
// const stopReading = document.getElementById("stopReading");


Read.addEventListener("click", async () => {
  try {
    // Stop reading
    let stopResponse = await fetch("http://127.0.0.1:5000/stopRead");
    let stopResult = await stopResponse.json();
    console.log(stopResult.status);

    // Start writing
    let startResponse = await fetch("http://127.0.0.1:5000/startWrite");
    let startResult = await startResponse.json();
    console.log(startResult.status);
  } catch (e) {
    console.log("Exception has occurred: " + e);
  }

  container.classList.add("active");
});


Write.addEventListener("click", async () => {
  try {
    // Stop writing
    let stopResponse = await fetch("http://127.0.0.1:5000/stopWrite");
    let stopResult = await stopResponse.json();
    console.log(stopResult.status);

    // Start reading
    let startResponse = await fetch("http://127.0.0.1:5000/startRead");
    let startResult = await startResponse.json();
    console.log(startResult.status);    
  } catch (e) {
    console.log("Exception has occurred: " + e);
  }

  container.classList.remove("active");
});


writeData.addEventListener("click", newWriteFunction);
