const express = require("express");
const bodyParser = require("body-parser");
const app = express();
const path = require("path");

app.use(bodyParser.json());
const FilePath = path.join("D:", "--PROJECTS2024--", "LumosAI_", "index.html");

app.get("/", function (req, res) {
  res.send("GET request to homepage");
});

// Test endpoint
app.get("/test", function (req, res) {
  res.send("communication established");
});

// POST endpoint to handle the data from the board
app.post("/api/endpoint", function (req, res) {
  const { uid, rollno } = req.body;
  console.log("UID:", uid);
  console.log("RollNo:", rollno);

  // Send a response back to the board
  res.send("Data received successfully");
});

const port = 3000;
app.listen(port, '0.0.0.0', () => {
  console.log(`Server is running on http://localhost:${port}`);
});
