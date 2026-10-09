## Testprotokoll finns i docs

# TLS-skyddat MQTT-sensorsystem

Det här projektet består av tre program som bildar en datakedja:

1. **Publisher:** en ESP32-C6 läser av en ultraljudssensor och publicerar MQTT-data över TLS.
2. **Broker:** TLS-servern i `mqtt_cpp` tar emot MQTT-meddelanden och vidarebefordrar dem till subscribers.
3. **Subscriber:** ett C++-program i Linux prenumererar på `sensor/distance`, hanterar och validerar mätvärden samt tillhandahåller det senaste värdet via ett HTTP-gränssnitt.

MQTT-anslutningen använder TCP-port **8883**. MQTT-topicen är **`sensor/distance`**. Subscriberprogrammets HTTP-adress är **`http://localhost:8080/api/readings/latest`**.

## Mappstruktur

```text
mqtt-broker/mqtt_cpp/
  example/tls_server.cpp
  include/                    mqtt_cpp-bibliotekets headerfiler
  certs/                      skapas lokalt och checkas inte in
mqtt-publisher/
  main/                       ESP-IDF-källkod för firmware
mqtt-subscriber/
  main.cpp
  mqtt-subscriber.cpp
  mqtt-subscriber.h
  server.crt                  skapas/kopieras lokalt och checkas inte in
```

Projektet kompilerar endast exemplet `mqtt_cpp/example/tls_server.cpp` direkt. Övriga mqtt_cpp-exempel och dess ursprungliga bygg- och testsystem behövs inte för kommandot nedan.

## 1. Installera Linux-paketen

På Ubuntu eller Debian installerar du kompilatorn, TLS- och Boost-biblioteken, Mosquitto-klientbiblioteket, JSON-headerfilerna samt ESP-IDF:s förutsättningar:

```bash
sudo apt update
sudo apt install -y \
  build-essential git wget flex bison gperf \
  python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 \
  libboost-all-dev libmosquitto-dev nlohmann-json3-dev \
  mosquitto-clients openssl curl
```

`libboost-all-dev` installerar Boost-headerfilerna som mqtt_cpp använder. mqtt_cpp kräver C++14 och Boost 1.74 eller senare. `libssl-dev` installerar OpenSSL-utvecklingsfiler för TLS-brokern. `libmosquitto-dev` behövs för att kompilera Linux-subscribern. `nlohmann-json3-dev` installerar JSON-headern som används av subscriber/API-koden. `mosquitto-clients` är valfritt men praktiskt för tester från kommandoraden.

## 2. Ta reda på brokerns IP-adress

Brokern lyssnar på alla IPv4-nätverksgränssnitt på Linux-datorn. Publishern och subscribern måste ansluta till datorns lokala nätverksadress. Om de körs på andra enheter fungerar inte `127.0.0.1`.

Visa datorns IP-adresser med:

```bash
hostname -I
```

Välj adressen för det Wi-Fi- eller LAN-nätverk som ESP32:an använder, till exempel `192.168.1.115`. Använd samma adress i:

- certifikatets IP-adress i Subject Alternative Name (SAN),
- subscriberns värdnamn/IP i `mosquitto_connect()`,
- ESP32-publisherns broker-URI: `mqtts://<BROKER_IP>:8883`.

Om routern ändrar datorns adress behöver du uppdatera dessa inställningar och skapa ett nytt certifikat med den nya adressen. En DHCP-reservation i routern gör att adressen förblir densamma.

## 3. Skapa ett lokalt TLS-certifikat

Certifikat och privata nycklar ska inte checkas in i Git. Varje person som kör projektet skapar sina egna. Kommandot nedan skapar ett självsignerat testcertifikat och en privat nyckel som gäller i 365 dagar. Byt ut exempeladressen mot Linux-datorns faktiska lokala IP-adress innan du kör kommandot.

Kör från projektets rotmapp: (mqtt_cpp)

```bash
BROKER_IP="192.168.1.115"  # Byt till adressen från steg 2
mkdir certs

openssl req -x509 -newkey rsa:2048 -sha256 -days 365 -nodes \
  -keyout certs/server.key \
  -out certs/server.crt \
  -subj "/CN=${BROKER_IP}" \
  -addext "subjectAltName=IP:${BROKER_IP}" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,digitalSignature,keyEncipherment,keyCertSign" \
  -addext "extendedKeyUsage=serverAuth"

chmod 600 mqtt-broker/mqtt_cpp/certs/server.key
```

Kontrollera att certifikatet innehåller rätt IP-adress i SAN:

```bash
openssl x509 -in mqtt-broker/mqtt_cpp/certs/server.crt -noout -subject -ext subjectAltName
```

Kopiera det **publika certifikatet** till de två klienterna. Den privata nyckeln ska bara finnas hos brokern:

```bash
cp mqtt-broker/mqtt_cpp/certs/server.crt mqtt-publisher/main/server.crt
cp mqtt-broker/mqtt_cpp/certs/server.crt mqtt-subscriber/server.crt
```

Subscriberprogrammets nuvarande kod läser `server.crt` relativt den aktuella arbetsmappen. Därför ska du köra programmet från `mqtt-subscriber/`. Publisherns certifikatfil ligger i `mqtt-publisher/main/` och används av ESP-IDF-projektet. Bygg och flasha om publishern när du byter certifikat.

Checka aldrig in `server.key`, `server.crt` eller andra genererade certifikat- och nyckelfiler. Projektets `.gitignore` ignorerar filerna `*.crt`, `*.key` och `*.pem`.

## 4. Kompilera och starta TLS-brokern

Brokerns källkod är `mqtt-broker/mqtt_cpp/example/tls_server.cpp`. Kompilera den från mqtt_cpp-mappen:

```bash
cd mqtt-broker/mqtt_cpp

g++ -std=c++14 -DMQTT_USE_TLS -Iinclude \
  example/tls_server.cpp -o broker_tls \
  -pthread -lssl -lcrypto
```

Starta brokern på port 8883 och ange sökvägarna till certifikatet och den privata nyckeln:

```bash
./broker_tls 8883 certs/server.crt certs/server.key
```

Låt terminalen vara öppen medan du använder publishern och subscribern. Servern skriver information om anslutningar, prenumerationer och publicerade meddelanden i terminalen. Stoppa den med `Ctrl+C`.

Exemplet använder TLS för brokeranslutningen. Det ställer inte in MQTT-användarnamn eller lösenord och kräver inte klientcertifikat.

## 5. Kompilera och starta Linux-subscribern

Subscribern använder Mosquittos C-klientbibliotek. Det biblioteket installerades i steg 1.

```bash
cd mqtt-subscriber

g++ -std=c++17 -Wall -Wextra -Wpedantic \
  main.cpp mqtt-subscriber.cpp -pthread -lmosquitto \
  -o mqtt-subscriber
```

Starta programmet från samma mapp så att den relativa sökvägen till `server.crt` fungerar:

```bash
./mqtt-subscriber
```

Subscribern ansluter till brokern via TCP-port 8883, använder `server.crt` för att verifiera TLS-anslutningen och prenumererar på `sensor/distance` med QoS 1. Den skriver mottagen topic och payload i terminalen. IP-adressen i dess källkod måste stämma med adressen från steg 2.

### Anmärkning om subscriber-källkoden

Den tidigare delade versionen av `mqtt-subscriber.cpp` innehöll en oanvänd funktion `ping_computer()` med anrop som bara finns i ESP-IDF, till exempel `ESP_PING_DEFAULT_CONFIG()` och `vTaskDelay()`. Dessa funktioner finns inte i en vanlig Linux-kompilator. Om funktionen fortfarande finns kvar i din fil behöver du ta bort den från Linux-subscriberprogrammet. Den hör inte till MQTT-subscribern.

### HTTP-gränssnitt för senaste mätvärdet

När subscribern körs kan du hämta det senaste godkända mätvärdet med:

```bash
curl -i http://localhost:8080/api/readings/latest
```

Innan subscribern tagit emot ett giltigt mätvärde svarar adressen med **503 Service Unavailable**. Efter att ett giltigt MQTT-meddelande kommit fram returneras det senaste mätvärdet som JSON. Värdet lagras i minnet och försvinner när subscribern startas om.

Det avsedda formatet för ett sensormeddelande är:

```json
{"sensorId":1,"unit":"cm","value":26.0}
```

De förväntade fälten är ett heltal `sensorId`, en `unit` med värdet `cm` och ett numeriskt `value`. Avståndsgränserna ska stämma med ultraljudssensorns specifikation; intervallet som diskuterats för projektet är 2–400 cm. Publisherns meddelandeformat och subscriberns valideringsregler måste överensstämma.

## 6. Installera ESP-IDF för ESP32-C6-publishern

Publishern använder ESP-IDF 5.5.2. Om den versionen inte redan är installerad installerar du Ubuntu-/Debian-förutsättningarna i steg 1 och kör sedan:

```bash
mkdir -p "$HOME/esp/v5.5.2"
cd "$HOME/esp/v5.5.2"
git clone -b v5.5.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32c6
. ./export.sh
```

Punkten och mellanslaget före `./export.sh` är viktiga: skriptet sätter miljövariabler i den aktuella terminalen. I en ny terminal aktiverar du ESP-IDF igen:

```bash
. "$HOME/esp/v5.5.2/esp-idf/export.sh"
```

Om du redan har ESP-IDF 5.5.2 installerat på en annan plats använder du den installationens `export.sh`. ESP-IDF Component Manager hämtar komponenterna som deklareras i publisherns `idf_component.yml` när projektet konfigureras eller byggs.

Officiell installationsguide: [ESP-IDF 5.5 för ESP32-C6](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c6/get-started/linux-macos-setup.html).

## 7. Konfigurera, bygg och flasha ESP32-publishern

När ESP-IDF är aktiverat kör du följande från projektets rotmapp:

```bash
cd mqtt-publisher
idf.py set-target esp32c6
idf.py menuconfig
```

I projektets konfigurationsmeny anger du Wi-Fi-nätverkets SSID och lösenord samt brokerns lokala nätverksadress och port. Projektets egna inställningar finns i `main/Kconfig.projbuild`. Ange broker-URI:n som `mqtts://<BROKER_IP>:8883` och kontrollera att publishern använder servercertifikatet som kopierats till `main/server.crt`.

`idf.py set-target esp32c6` behövs vid den första byggningen eller när du byter målchip. Kommandot återställer den lokala byggkonfigurationen. Konfigurera och kompilera sedan:

```bash
idf.py reconfigure
idf.py build
```

Ta reda på ESP32-kortets serieport. Den heter ofta `/dev/ttyUSB0` eller `/dev/ttyACM0`. Flasha och följ programmets loggar med:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Byt `/dev/ttyUSB0` mot kortets faktiska serieport. Om Linux visar ett behörighetsfel lägger du till användaren i gruppen `dialout` och loggar ut och in igen:

```bash
sudo usermod -aG dialout "$USER"
```

Firmware ansluter till Wi-Fi, ansluter säkert till brokern med TLS och publicerar avståndsmätningar på `sensor/distance`.

## 8. Kör hela systemet

Använd tre terminaler på samma lokala nätverk:

1. **Broker:** kör `./broker_tls 8883 certs/server.crt certs/server.key` från `mqtt-broker/mqtt_cpp/`.
2. **Subscriber:** kör `./mqtt-subscriber` från `mqtt-subscriber/`.
3. **ESP-IDF:** aktivera ESP-IDF och kör `idf.py -p /dev/ttyUSB0 flash monitor` från `mqtt-publisher/`. Byt serieport vid behov.

När ESP32:an publicerar ska brokern logga topic och payload, subscribern ska skriva ut meddelandet och HTTP-adressen ska returnera det senaste giltiga mätvärdet.

## Felsökning

- **Subscribern hittar inte `server.crt`:** skapa certifikatet och kopiera det enligt steg 3. Starta sedan subscribern från `mqtt-subscriber/`.
- **TLS-verifieringen misslyckas:** IP-adressen i certifikatets SAN måste vara samma adress som klienterna ansluter till. Kopiera det nya `server.crt` till båda klientmapparna och bygg/flasha om publishern.
- **Anslutningen får timeout:** kontrollera att brokern körs, att alla enheter är på samma lokala nätverk och att TCP-port 8883 inte blockeras av datorns brandvägg eller routern/accesspunkten. ESP32:an och subscribern måste kunna nå brokern via det lokala nätverket.
- **Anslutningen nekas:** kontrollera att `broker_tls` körs på port 8883 och att brokerterminalen inte visar ett startfel.
- **API:t svarar 503:** det har ännu inte tagit emot något giltigt mätvärde. Starta publishern och kontrollera loggarna från brokern och subscribern.
- **Kommandot `idf.py` hittas inte:** aktivera ESP-IDF igen med `. "$HOME/esp/v5.5.2/esp-idf/export.sh"` eller använd sökvägen till din installation.

## Tredjepartsbibliotek

Brokern använder [mqtt_cpp](https://github.com/redboltz/mqtt_cpp), ett header-only MQTT-bibliotek. Licensen finns i `mqtt-broker/mqtt_cpp/LICENSE_1_0.txt`. Exemplet använder C++14, Boost 1.74 eller senare och OpenSSL.
