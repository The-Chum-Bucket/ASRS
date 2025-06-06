# import smtplib
# import os
# from email.message import EmailMessage

# # Configuration: load from environment variables set in .env
# SMTP_SERVER = os.getenv("EMAIL_SMTP_SERVER", "smtp.office365.com")
# SMTP_PORT = int(os.getenv("EMAIL_SMTP_PORT", 587))
# EMAIL_ADDRESS = os.getenv("EMAIL_USERNAME")
# EMAIL_PASSWORD = os.getenv("EMAIL_PASSWORD")
# DEFAULT_FROM = EMAIL_ADDRESS
# EMAIL_RECIPIENTS = os.getenv("EMAIL_RECIPIENTS", "").split(",")
# EMAIL_RECIPIENTS = [email.strip() for email in EMAIL_RECIPIENTS if email.strip()]

# def send_email(subject, body, to_addrs=None):
#     """Sends an email using environment-configured SMTP credentials."""
#     if not EMAIL_ADDRESS or not EMAIL_PASSWORD:
#         raise ValueError("EMAIL_USERNAME and EMAIL_PASSWORD must be set in environment variables. Check README for environment variable setup.")

#     if to_addrs is None:
#         to_addrs = EMAIL_RECIPIENTS

#     msg = EmailMessage()
#     msg["Subject"] = subject
#     msg["From"] = DEFAULT_FROM
#     msg["To"] = ", ".join(to_addrs) if isinstance(to_addrs, list) else to_addrs
#     msg.set_content(body)

#     with smtplib.SMTP(SMTP_SERVER, SMTP_PORT) as server:
#         server.starttls()
#         server.login(EMAIL_ADDRESS, EMAIL_PASSWORD)
#         server.send_message(msg)


import smtplib
from email.message import EmailMessage

def send_email(subject, body, to_addrs=None):
    """Sends an email using Gmail's SMTP server with STARTTLS."""

    if not EMAIL_ADDRESS or not EMAIL_PASSWORD:
        raise ValueError("Email credentials not set.")

    if to_addrs is None:
        to_addrs = EMAIL_RECIPIENTS

    msg = EmailMessage()
    msg["Subject"] = subject
    msg["From"] = DEFAULT_FROM
    msg["To"] = ", ".join(to_addrs) if isinstance(to_addrs, list) else to_addrs
    msg.set_content(body)

    with smtplib.SMTP(SMTP_SERVER, SMTP_PORT) as server:
        server.ehlo()
        server.starttls()
        server.ehlo()
        server.login(EMAIL_ADDRESS, EMAIL_PASSWORD)
        server.send_message(msg)
