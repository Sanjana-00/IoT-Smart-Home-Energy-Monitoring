#  IoT Based Smart Home Energy Monitoring and Control System

### Smart Energy Monitoring | Real-Time Dashboard | Energy Prediction | Overload Protection

## 📌 Project Overview

The **IoT Based Smart Home Energy Monitoring and Control System** is designed to monitor and manage household electricity consumption using the ESP32 microcontroller.

The system collects voltage and current readings, calculates power and energy consumption, and sends the data to Firebase Realtime Database. A web-based dashboard displays real-time energy parameters and allows users to remotely control electrical appliances.

The project also incorporates Linear Regression to forecast future energy consumption and an overload protection mechanism to improve electrical load management.

## 🎯 Objectives

- Monitor real-time electricity consumption.
- Measure voltage, current, power and energy.
- Enable remote appliance control through a web dashboard.
- Predict future energy consumption using Linear Regression.
- Implement an overload protection mechanism.
- Provide a user-friendly energy monitoring interface.

## ✨ Key Features

- **Real-Time Monitoring:** Displays voltage, current, power and energy consumption.
- **Web Dashboard:** Interactive dashboard with live readings and graphical representations.
- **Remote Appliance Control:** Controls electrical appliances through a relay.
- **Energy Prediction:** Uses Linear Regression to forecast 30-day energy consumption.
- **Overload Protection:** Provides a 10-second protection mechanism to switch OFF the connected load and restore it automatically.
- **Cloud Integration:** Uses Firebase Realtime Database for data communication.

## 🛠️ Technologies Used

| Category | Technologies |
|---|---|
| Microcontroller | ESP32 |
| Programming | Embedded C/C++ |
| Sensors | SCT-013, ZMPT101B |
| Frontend | HTML, CSS, JavaScript |
| Charts | Chart.js |
| Cloud Database | Firebase Realtime Database |
| Machine Learning | Linear Regression |
| Simulation | Wokwi |
| Development Tool | Arduino IDE |

## 🔌 Hardware Components

- ESP32 Microcontroller
- SCT-013 Current Sensor
- ZMPT101B Voltage Sensor
- Relay Module
- 100W Bulb (Electrical Load)
- Connecting Wires and Power Supply

## 🏗️ System Architecture

1. Voltage and current sensors collect electrical parameters.
2. ESP32 processes the sensor readings and calculates power and energy consumption.
3. The readings are transmitted to Firebase Realtime Database.
4. The web dashboard retrieves and displays real-time information.
5. Linear Regression analyzes historical power readings to estimate future energy consumption.
6. Users can control the connected appliance through the dashboard using the relay control mechanism.

## 📊 Dashboard Features

The dashboard provides:

- Live voltage monitoring.
- Current measurement.
- Power consumption monitoring.
- Energy consumption in Wh and kWh.
- Live voltage and current graphs.
- Energy consumption graphs.
- 30-day energy forecasting.
- Relay and overload protection controls.

## 🤖 Energy Prediction Using Linear Regression

Linear Regression is implemented using the least-squares method.

The model uses historical power readings to calculate a regression line and estimate future power consumption.

The predicted hourly power values are accumulated to generate a 30-day energy forecast.

## 🧪 Wokwi Simulation

The ESP32 firmware and circuit behavior are demonstrated using Wokwi simulation.

**Simulation Link:**
https://wokwi.com/projects/472359856443161601

Note: Simulated sensor inputs are used for the Wokwi demonstration. Physical sensor measurements and simulation inputs should be distinguished.

## 📂 Project Structure

```text
IoT-Smart-Home-Energy-Monitoring/
│
├── Arduino/
│   └── Smart_Energy_Monitor.ino
│
├── Dashboard/
│   └── Smart_Energy_Dashboard.html
│
├── Simulation/
│   └── Wokwi_Link.txt
│
├── Images/
│   ├── Dashboard.png
│   └── Wokwi_Circuit.png
│
├── Documentation/
│   └── Project_Report.pdf
│
├── README.md
└── .gitignore
```

## 🚀 Future Enhancements

- Integration of mobile application support.
- Detailed historical energy analytics.
- Electricity bill estimation.
- Support for multiple household appliances.
- Improved prediction using advanced machine learning algorithms.

## 🎓 Project Information

**Project Type:** Academic / IoT Project

**Domain:** Internet of Things (IoT), Smart Home Automation and Machine Learning

**Project Status:** Prototype

---

⭐ If you find this project interesting, feel free to explore the source code and Wokwi simulation!
