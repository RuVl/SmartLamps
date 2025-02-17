#!/usr/bin/env bash

echo "This script creates a mqtt server."

# Ask for username
read -p "Username: " -r username

# Ask for password
while : ; do
  read -p "New password: " -r -s psw1
  echo
  read -p "Retype new password: " -r -s psw2
  echo

  if [ "$psw1" == "$psw2" ]; then
    break
  else
    echo "Sorry, passwords do not match."
  fi
done

# Update package list
sudo apt update

# Install MQTT broker
sudo apt install mosquitto -y

# Open 1883 port for internet
sudo ufw allow 1883/tcp

# Adding password and give rights
sudo mosquitto_passwd -c -b /etc/mosquitto/passwd "$username" "$psw1"
sudo chmod 640 /etc/mosquitto/passwd

# Add config for internet connections with password
echo "
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
" >> /etc/mosquitto/mosquitto.conf

# Restart service for applying changes
sudo systemctl restart mosquitto

# Useful commands:
# Subscribe mqtt topic
mosquitto_sub -h YOUR_SERVER_IP -p 1883 -u "your_user" -P "your_password" -t "topic_name"

# Publish message to topic
mosquitto_pub -h YOUR_SERVER_IP -p 1883 -u "your_user" -P "your_password" -t "topic_name" -m "Message"
