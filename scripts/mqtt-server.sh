#!/usr/bin/env bash
# Sets up a Mosquitto broker for the lamps on Debian/Ubuntu. Safe to re-run:
# the config file is rewritten as a whole, the password file is updated in
# place, nothing is appended twice.
#
#   sudo scripts/mqtt-server.sh                      # asks for user and password
#   sudo scripts/mqtt-server.sh -u lamp -p secret    # non-interactive
#   sudo scripts/mqtt-server.sh -u lamp --tls mqtt.example.com
#
# Without --tls the broker listens on 1883 with password auth, which is what
# the firmware speaks today. --tls adds 8883 with a Let's Encrypt certificate
# (certbot standalone: port 80 must reach this host) and keeps 1883 for the
# lamps until the firmware grows TLS - see docs/backlog.md.

set -euo pipefail

CONF=/etc/mosquitto/conf.d/smartlamp.conf
PASSWD=/etc/mosquitto/passwd
CERTS=/etc/mosquitto/certs

user=""
pass=""
domain=""

usage() { sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        -u|--user) user="$2"; shift 2 ;;
        -p|--pass) pass="$2"; shift 2 ;;
        --tls) domain="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "unknown option: $1" >&2; usage ;;
    esac
done

[ "$(id -u)" -eq 0 ] || { echo "run as root (sudo)" >&2; exit 1; }
command -v apt-get >/dev/null || { echo "this script knows only apt (Debian/Ubuntu)" >&2; exit 1; }

if [ -z "$user" ]; then read -r -p "MQTT user: " user; fi
if [ -z "$pass" ]; then
    while :; do
        read -r -s -p "Password: " p1; echo
        read -r -s -p "Again: " p2; echo
        [ "$p1" = "$p2" ] && [ -n "$p1" ] && { pass="$p1"; break; }
        echo "passwords differ or empty, once more"
    done
fi

echo "== packages"
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq mosquitto mosquitto-clients >/dev/null
[ -n "$domain" ] && apt-get install -y -qq certbot >/dev/null

echo "== password file"
if [ -f "$PASSWD" ]; then
    mosquitto_passwd -b "$PASSWD" "$user" "$pass"  # add or update one user
else
    mosquitto_passwd -c -b "$PASSWD" "$user" "$pass"
fi
chown mosquitto:mosquitto "$PASSWD"
chmod 640 "$PASSWD"

if [ -n "$domain" ]; then
    echo "== certificate for $domain"
    if [ ! -d "/etc/letsencrypt/live/$domain" ]; then
        # Standalone needs port 80 free for the challenge.
        certbot certonly --standalone --non-interactive --agree-tos \
            --register-unsafely-without-email -d "$domain"
    fi
    # Mosquitto drops privileges and cannot read /etc/letsencrypt, so the
    # files are copied; the deploy hook repeats the copy on every renewal.
    HOOK=/etc/letsencrypt/renewal-hooks/deploy/mosquitto.sh
    mkdir -p "$(dirname "$HOOK")" "$CERTS"
    cat > "$HOOK" <<HOOKEOF
#!/usr/bin/env bash
set -e
cp /etc/letsencrypt/live/$domain/fullchain.pem $CERTS/fullchain.pem
cp /etc/letsencrypt/live/$domain/privkey.pem $CERTS/privkey.pem
chown mosquitto:mosquitto $CERTS/*.pem
chmod 600 $CERTS/privkey.pem
systemctl reload mosquitto
HOOKEOF
    chmod +x "$HOOK"
    "$HOOK" || true  # first copy; reload may fail before the config exists
fi

echo "== $CONF"
{
    echo "# Written by scripts/mqtt-server.sh; edits here are overwritten on re-run."
    echo "allow_anonymous false"
    echo "password_file $PASSWD"
    echo
    echo "listener 1883"
    if [ -n "$domain" ]; then
        echo
        echo "listener 8883"
        echo "certfile $CERTS/fullchain.pem"
        echo "keyfile $CERTS/privkey.pem"
    fi
} > "$CONF"

echo "== firewall"
if command -v ufw >/dev/null && ufw status | grep -q "^Status: active"; then
    ufw allow 1883/tcp >/dev/null
    [ -n "$domain" ] && ufw allow 8883/tcp >/dev/null
    echo "ufw: opened"
else
    echo "ufw not active, nothing to open"
fi

echo "== restart"
systemctl enable --now mosquitto >/dev/null
systemctl restart mosquitto
sleep 1

echo "== check"
if mosquitto_sub -h 127.0.0.1 -p 1883 -u "$user" -P "$pass" -t '$SYS/broker/version' -C 1 -W 5; then
    echo "broker answers with a password on 1883"
else
    echo "broker did not answer; see: journalctl -u mosquitto -n 50" >&2
    exit 1
fi

ip=$(hostname -I 2>/dev/null | awk '{print $1}')
cat <<EOT

In the lamp's panel, menu MQTT:
  Сервер:       ${domain:-$ip}
  Порт:         1883
  Пользователь: $user
  Пароль:       (the one you entered)

Watch the lamps from here:
  mosquitto_sub -h ${domain:-$ip} -u $user -P '...' -t 'lamp/#' -v
EOT
