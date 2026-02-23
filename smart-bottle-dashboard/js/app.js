// FIREBASE CONFIG

const firebaseConfig = {
  databaseURL:
    "https://smart-bottle-iot-default-rtdb.asia-southeast1.firebasedatabase.app/",
};

firebase.initializeApp(firebaseConfig);

const db = firebase.database();

// LIVE DATA LISTENER

db.ref("bottle/telemetry").on("value", function (snapshot) {
  const data = snapshot.val();

  if (!data) return;

  document.getElementById("temp").innerText = data.temperature.toFixed(1);

  document.getElementById("setpoint").innerText = data.setpoint.toFixed(1);

  document.getElementById("mode").innerText = data.mode;

  document.getElementById("heater").innerText = data.heater ? "ON 🔥" : "OFF";

  document.getElementById("cooler").innerText = data.cooler ? "ON ❄" : "OFF";
});

// SLIDER CONTROL

const slider = document.getElementById("slider");

const sliderValue = document.getElementById("sliderValue");

slider.oninput = function () {
  sliderValue.innerText = this.value;
};

slider.onchange = function () {
  db.ref("bottle/control").update({
    setpoint: Number(this.value),
  });
};
