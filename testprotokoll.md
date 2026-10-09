# Testprotokoll

Fyll i datum och faktiskt resultat när testerna körs. Markera inte ett test som godkänt innan det har verifierats.

## End-to-end: giltig mätning
curl -i http://localhost:8080/api/readings/latest

* Starta broker med TLS på port `8883` | Brokerlogg visar att tjänsten lyssnar | Lyckad |
* Starta C++-subscriber | TLS-anslutning lyckas och subscriber prenumererar på `sensor/distance` | Lyckad |
* Starta ESP32 med fysisk avståndssensor | Subscriber loggar JSON-mätningar | Lyckad |
* Anropa `GET /api/readings/latest` | HTTP `200 OK` och senaste giltiga JSON-mätning | Lyckad |

## Feltest 1: Ogiltig JSON

1. Notera den senaste giltiga mätningen via HTTP-API:t.
2. Ändra tillfälligt publisherpayloaden så `unit` saknar citattecken, exempelvis `"unit":cm`.
3. Kontrollera att subscribern loggar ett JSON-/valideringsfel.
4. Kontrollera att HTTP-API:t fortfarande returnerar den tidigare giltiga mätningen.
5. Återställ korrekt JSON-format och kontrollera att nya värden accepteras.

**Resultat:** Subscribern loggar: Rejected MQTT message: invalid fields or values, inget nytt värde kommer när jag anropar via HTTP-APIet, den returnerar den senaste.
Efter att korrekt JSON format är återställt så fungerar allt som det ska

## Feltest 2: Fel topic

1. Kontrollera att subscribern prenumererar på `sensor/distance`.
2. Ändra tillfälligt publisherns topic till `sensor/distanc`.
3. Kontrollera att nya mätningar inte når subscribern och att HTTP-värdet inte uppdateras.
4. Återställ publisherns topic till `sensor/distance`.
5. Kontrollera att subscriber tar emot mätningar igen och att API-värdet uppdateras.

**Resultat:** När jag ändrade topic så fortsatte publishern att publicera, men payloaden kom endast fram till brokern där den loggades, men kom inte vidare till subscribern, när jag ändrade till rätt topic
så kom payloaden rätt igen.

