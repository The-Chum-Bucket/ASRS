import os
import smtplib
from email.message import EmailMessage
from dotenv import load_dotenv


def send_email(subject, body, to_addrs=None):
    """Sends an email using environment-configured SMTP credentials."""

    # Re-load .env each time to get the latest values (e.g. updated recipients)
    #load_dotenv()

    # Configuration: load from environment variables set in .env
    #SMTP_SERVER = os.getenv("EMAIL_SMTP_SERVER", "smtp.office365.com")
    #SMTP_PORT = int(os.getenv("EMAIL_SMTP_PORT", 587))
    #EMAIL_ADDRESS = os.getenv("EMAIL_USERNAME")
    #EMAIL_PASSWORD = os.getenv("EMAIL_PASSWORD")
    
    #EMAIL_RECIPIENTS = os.getenv("EMAIL_RECIPIENTS", "").split(",")
    #EMAIL_RECIPIENTS = [email.strip() for email in EMAIL_RECIPIENTS if email.strip()]
    EMAIL_ADDRESS = "nora.pier.test@gmail.com"
    EMAIL_PASSWORD = "hixgxngnmbvjqmqw"
    EMAIL_RECIPIENTS = ["jande180@calpoly.edu"]
    DEFAULT_FROM = EMAIL_ADDRESS
    SMTP_SERVER = "smtp.gmail.com"
    SMTP_PORT = 465
   
    if not EMAIL_ADDRESS or not EMAIL_PASSWORD:
        raise ValueError("EMAIL_USERNAME and EMAIL_PASSWORD must be set in environment variables.")

    if to_addrs is None:
        to_addrs = EMAIL_RECIPIENTS

    msg = EmailMessage()
    msg["Subject"] = subject
    msg["From"] = DEFAULT_FROM
    msg["To"] = ", ".join(to_addrs) if isinstance(to_addrs, list) else to_addrs
    msg.set_content(body)

    with smtplib.SMTP(SMTP_SERVER, SMTP_PORT) as server:
        server.starttls()
        server.login(EMAIL_ADDRESS, EMAIL_PASSWORD)
        server.send_message(msg)