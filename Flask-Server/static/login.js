import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import {
  getAuth,
  signInWithEmailAndPassword,
} from "https://www.gstatic.com/firebasejs/10.8.0/firebase-auth.js";

/*FIREBASE AUTH*/

const firebaseConfig = {
  apiKey: "AIzaSyClFl9Z83m1_cKfjOBWmgPASfh6JxBEbt4",
  authDomain: "lumosai-8c4e8.firebaseapp.com",
  databaseURL: "https://lumosai-8c4e8-default-rtdb.firebaseio.com",
  projectId: "lumosai-8c4e8",
  storageBucket: "lumosai-8c4e8.appspot.com",
  messagingSenderId: "1027651970462",
  appId: "1:1027651970462:web:1a48cb44d763c32d62e28f",
};

const app = initializeApp(firebaseConfig);
const auth = getAuth(app);

const loginbtn = document.getElementById("login-submit");
const username = document.getElementById("user-name");
const password = document.getElementById("user-password");
const user = "";

// Login function

// Assuming the crud page and mapping page are hidden
loginbtn.addEventListener("click", function () {
  var mail = username.value;
  var pass = password.value;
  signInWithEmailAndPassword(auth, mail, pass)
    .then((userCredential) => {
      user = userCredential.user;
      sendauth(user);
      console.log("Signed in");
      window.alert("Signed in");
      // The options like crud page and mapping page must only show up after loggin in
      //window.location.href = "";
    })
    .catch((error) => {
      const errorCode = error.code;
      const errorMessage = error.message;
      window.alert("Invalid Credentials");
      console.log(errorMessage);
    });
});

async function sendauth(username) {
  try {
    const response = await fetch("http://127.0.0.1:5000/auth", {
      method: "POST",
      headers: {
        Accept: "application/json",
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ auth: username }),
    });

    const jsonResponse = await response.json();
    console.log(JSON.stringify(jsonResponse));

    if (jsonResponse.status) {
      if (jsonResponse.status == "success") {
        console.log("auth received");
      } else {
        console.log("authentication failed");
      }
    }
  } catch (error) {
    console.error(error);
  }
}
