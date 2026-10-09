# API-dokumentation

## Hämta senaste mätningen

| Egenskap | Värde |
|---|---|
| Metod | `GET` |
| Sökväg | `/api/readings/latest` |
| Lokal URL | `http://localhost:8080/api/readings/latest` |
| Parametrar | Inga |
| Format | JSON |

Exempel:

```bash
curl -i http://localhost:8080/api/readings/latest
```

Exempel på svar med status `200 OK`:

```json
{
  "sensorId": 1,
  "unit": "cm",
  "value": 24.00
}
```

API:t returnerar den senast mottagna och godkända MQTT-payloaden. Mätningen lagras endast i minnet.

## Felkoder

| HTTP-status | Betydelse | Exempelsvar |
|---|---|---|
| `200 OK` | En giltig mätning finns | Sensorobjektet som JSON |
| `503 Service Unavailable` | Ingen giltig mätning har tagits emot ännu | `{"error":"No sensor reading received yet"}` |
| `404 Not Found` | Sökvägen stöds inte | `{"error":"Endpoint not found"}` |

API:t använder för närvarande okrypterad HTTP och har ingen autentisering. Använd det bara i den avsedda lokala testmiljön.

