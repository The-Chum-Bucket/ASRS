# NORA - Nearshore Ocean Retrieval Apparatus
Contributors: Jack Anderson, Emma Lucke, Danny Gutierrez, Deeba Khosravi, and Jorge Ramirez
Codespace for the Arduino/C++ code running on the P1AM-100 PLC inside of NORA, as well as the corresponding Python script on the topside computer.

##  Setting Up `.env` Configuration

This project requires environment variables to send emails securely without hardcoding sensitive information into this public repository.

### Step-by-Step Instructions

1. **Create a `.env` file** in the same folder as the Python script in this project. The file will include the email username, password, SMTP server and port, and the email recipients (seperated by commas)

2. **Follow the Template Below to Create the `.env` file:**

   ```env

   EMAIL_USERNAME=mmustang@calpoly.edu
   EMAIL_PASSWORD=gomustangs
   EMAIL_SMTP_SERVER=smtp.calpoly.edu
   EMAIL_SMTP_PORT=1901
   EMAIL_RECIPIENTS=jmustang@calpoly.edu,kmustang@calpoly.edu

