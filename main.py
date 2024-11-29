import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import argparse
def get_password(file_path):
    try:
        with open(file_path, 'r') as file:
            return file.read().strip()
    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found.")
        exit(1)

def send_email(receiver_email, message_body):
    sender_email = "mmaks9852@gmail.com"
    password_file = "password.txt"
    password = get_password(password_file)

    message = MIMEMultipart()
    message['From'] = sender_email
    message['To'] = receiver_email
    message['Subject'] = 'Your auth code: '

    message.attach(MIMEText(message_body, 'plain'))

    try:
        server = smtplib.SMTP('smtp.gmail.com', 587)
        server.starttls() 
        server.login(sender_email, password)  

        text = message.as_string()
        server.sendmail(sender_email, receiver_email, text)
        print("Email sent successfully!")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        server.quit()  


def main():
    parser = argparse.ArgumentParser(description='Send an email via SMTP.')
    parser.add_argument('receiver', help='The email address of the receiver')
    parser.add_argument('message', help='The body of the email message')

    args = parser.parse_args()

    send_email(args.receiver, args.message)


if __name__ == "__main__":
    main()
