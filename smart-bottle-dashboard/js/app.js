// --- Firebase config & init ---
const firebaseConfig = {
  databaseURL:
    "https://smart-bottle-iot-default-rtdb.asia-southeast1.firebasedatabase.app/",
};

// initialize Firebase after SDK loaded
if (typeof firebase === "undefined") {
  console.error("Firebase SDK not loaded!");
} else {
  firebase.initializeApp(firebaseConfig);
}

const db = firebase.database ? firebase.database() : null;

// UI elements
const elTemp = document.getElementById("temp");
const elSet = document.getElementById("setpoint");
const elMode = document.getElementById("mode");
const elHeater = document.getElementById("heater");
const elCooler = document.getElementById("cooler");
const elStatus = document.getElementById("status");
const slider = document.getElementById("slider");
const sliderValue = document.getElementById("sliderValue");

// show connection status
function setStatus(s) {
  if (elStatus) elStatus.innerText = s;
}

// Listen telemetry (defensive)
if (db) {
  setStatus("Connected to Firebase");
  db.ref("bottle/telemetry").on(
    "value",
    (snapshot) => {
      const data = snapshot.val();
      if (!data) {
        // device offline -> show placeholders
        elTemp.innerText = "--";
        elSet.innerText = "--";
        elMode.innerText = "--";
        elHeater.innerText = "--";
        elCooler.innerText = "--";
        return;
      }

      if (data.temperature !== undefined && !Number.isNaN(data.temperature)) {
        elTemp.innerText = Number(data.temperature).toFixed(1);
      } else elTemp.innerText = "--";

      if (data.setpoint !== undefined && !Number.isNaN(data.setpoint)) {
        elSet.innerText = Number(data.setpoint).toFixed(1);
        // keep slider in sync if a remote set changed it
        slider.value = Number(data.setpoint);
        sliderValue.innerText = Number(data.setpoint);
      } else elSet.innerText = "--";

      elMode.innerText = data.mode || "--";
      elHeater.innerText = data.heater ? "ON 🔥" : "OFF";
      elCooler.innerText = data.cooler ? "ON ❄" : "OFF";
    },
    (err) => {
      console.error("Telemetry listener error:", err);
      setStatus("Telemetry error");
    }
  );
} else {
  setStatus("Firebase not initialized");
}

// Slider: update text on drag
if (slider && sliderValue) {
  slider.oninput = function () {
    sliderValue.innerText = this.value;
  };

  slider.onchange = function () {
    if (!db) return;
    const v = Number(this.value);
    // write only setpoint path under control
    db.ref("bottle/control")
      .update({ setpoint: v })
      .then(() => console.log("Wrote new setpoint:", v))
      .catch((e) => console.error("Failed write setpoint:", e));
  };
}
