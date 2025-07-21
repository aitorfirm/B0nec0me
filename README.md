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
