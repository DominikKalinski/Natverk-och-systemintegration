# Säkerhet

## Genomförda åtgärder

### TLS för MQTT

ESP32-publishern och C++-subscribern ansluter till brokern via TLS på TCP-port `8883`. Subscribern använder `server.crt` för att verifiera broker-certifikatet. TLS skyddar MQTT-trafiken mot avlyssning och hjälper klienten att upptäcka en broker som inte kan verifieras.

### Validering av inkommande JSON

Subscribern tolkar varje MQTT-payload som JSON och kontrollerar att:

- payloaden är inom den konfigurerade storleksgränsen;
- JSON-objektet har de förväntade fälten och datatyperna;
- `unit` är `cm`;
- avståndet är numeriskt, ändligt och inom det intervall som konfigurerats för sensorn.

Ogiltiga meddelanden loggas och ersätter inte den senaste giltiga mätningen. Kontrollera att intervallet i koden följer den använda sensorns specifikation.

## Begränsningar och kvarvarande risker

- HTTP-API:t är okrypterat och saknar autentisering. En annan enhet på samma nätverk kan i nuläget läsa mätningen om port `8080` är åtkomlig.
- API:t bör därför endast köras på ett betrott lokalt nätverk. Öppna inte port `8080` mot internet; begränsa åtkomsten med brandväggsregler om den behövs på nätverket.
- Mätdata sparas bara i minnet och försvinner när subscribern avslutas.
- Broker-IP och topic är hårdkodade i källkoden; de bör flyttas till konfiguration om lösningen byggs vidare.
- `server.crt` är ett CA-certifikat, inte en privat nyckel. Lägg aldrig privata nycklar, lösenord eller andra hemligheter i versionshanteringen.

