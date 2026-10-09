# Felsökning

Dokumentera varje feltest med symptom, identifiering, verktyg/loggar, orsak, åtgärd och verifiering. Följande fall bygger på det som observerats under utvecklingen. Uppgiften kräver att minst två fel införs avsiktligt; återupprepa därför fallen kontrollerat och fyll i datum/resultat i [testprotokollet](../testprotokoll.md) innan inlämning.

## Fall A: MQTT-port 8883 blockerad

- **Symptom:** MQTT-klienterna kunde inte ansluta till brokern över TLS. Ping ensamt kan inte visa om TCP-port `8883` är tillgänglig, eftersom ping använder ICMP.
- **Identifiering:** Jämförde anslutning från publisher/subscriber med brokerloggen och nätverkstester. Routerinställningen visade att port `8883` blockerades.
- **Verktyg/loggar:** ESP32-logg, Mosquitto-logg, ping och routerinställningar.
- **Orsak:** Routern blockerade TCP-port `8883` mellan enheterna.
- **Åtgärd:** Tillät den avsedda MQTT/TLS-trafiken på det lokala nätverket.
- **Verifiering:** ESP32 och subscriber anslöt igen; subscriber tog emot meddelanden på `sensor/distance`.
- **Avsiktlig repetition före inlämning:** Blockera porten kontrollerat, dokumentera felet, tillåt den igen och verifiera återanslutningen. Ändra inte andra inställningar under testet.

## Fall B: Ogiltig JSON-payload

- **Symptom:** HTTP-endpointen svarade `200 OK`, men payloaden innehöll exempelvis ett textvärde utan JSON-citattecken eller felaktigt formaterat `sensorId`.
- **Identifiering:** Jämförde rå MQTT-payload med JSON-kontraktet och validerade HTTP-svaret med `python3 -m json.tool`.
- **Verktyg/loggar:** Subscriberlogg och `curl -i` mot API:t. Den automatiska JSON-valideringen i subscribern ska visa att payloaden avvisas.
- **Orsak:** JSON-formatsträngen i ESP32-publishern saknade korrekt citering/struktur.
- **Åtgärd:** Rättade `snprintf` så att `sensorId`, `unit` och `value` produceras med rätt JSON-format.
- **Verifiering:** ESP32 publicerade giltig JSON; subscriber accepterade mätningen och API:t returnerade den.
- **Avsiktlig repetition före inlämning:** Skicka tillfälligt en payload med exempelvis `"unit":cm`. Subscriber ska logga valideringsfelet och behålla den senaste giltiga mätningen. Återställ sedan publishern.

## Fall C: Fel MQTT-topic (andra kontrollerade feltestet)

- **Planerat symptom:** Subscriber fortsätter prenumerera på `sensor/distance`, men publishern skickar till ett annat topic. Nya mätningar syns då inte hos subscribern och API:t fortsätter visa den senaste tidigare giltiga mätningen.
- **Så införs felet:** Ändra tillfälligt topicen i publishern, till exempel till `sensor/distanc`.
- **Identifiera med:** Kontrollera publisherns topic och broker-/subscriberloggar. Jämför med subscriberns prenumererade topic.
- **Orsak:** Publisherns topic matchar inte subscriberns prenumeration.
- **Åtgärd:** Återställ publisherns topic till `sensor/distance`.
- **Verifiering:** Subscriber loggar nya mätningar och API-svaret uppdateras.
- **Status:** Kör och fyll i resultat i testprotokollet innan detta fall beskrivs som genomfört.

