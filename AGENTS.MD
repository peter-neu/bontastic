## BLE connection Proof-of-concept


Goal: Connect to a Bluetooth device with a bluetooth pairing code, using the ESP32 Nimble library by h2zero.

Caveats:
Do not confuse the syntax! Check the repository with tool-calls if necessary [Nimble Library](https://github.com/h2zero/NimBLE-Arduino)


Style-Guide:
Since we want to create an POC and evaluate and learn the code, do NOT:

- Create "fluff" like error handling, unnecessary case matching etc.
- Do not write verbose code
- Do not put comments in the code 

Instead:
- be concise
- keep it simple!
- minimum neccessary code


## Tasks


### Printer Control via Bluetooth

We implement a GATT Server that controls the printer settings. We have a static webpage using Tailwind and vue3 via CDN. On the webpage we utilize Web-Bluetooth APIs to communicate with our GATT Server.

- Implement GATT Server on Device, that controls printer settings. We have to make sure that on-change the printer settings get updated
- GATT and Printer logic should reside in a separate file
- Create Static webpage
- Styling of the Webpage: Hacker looking, "dithering"-style, dark and green as color.
- Modern printer-page
- as a working reference webpage, see "reference.html" & "reference.ino"
- the server should be open to anyone - no need for extra security here

Futher instructions:

- Especially for the server side BLE connection, follow the reference implementation AS CLOSELY AS POSSIBLE
- Architectural clarification: Our device acts as CLIENT and SERVER in the BLE context. So we need to use the same NIMBLE instance.