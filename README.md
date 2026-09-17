# AI Agent Platform

Eine lokal betreibbare Agentenplattform für textbasierte Assistenz, Recherche,
RAG und lokale Terminverwaltung. Sie verbindet eine Browser-Oberfläche mit
einem lokalen LLM und klar abgegrenzten Diensten. Externe Websuche ist ein
kontrolliertes Werkzeug; Dokumente, Historie, Erinnerungen und Kalender bleiben
lokal. Voice ist vorbereitet, aber noch nicht Teil des normalen Chat-Starts.

Diese erweiterte README ist der Portfolio- und Projekteinstieg. Für den
konkreten Betrieb siehe [lokaler Text-Chat](docs/local_text_chat.md), für
Konfiguration und Verträge [Plattformgrundlagen](docs/platform_foundation.md).

## Portfolio-Überblick

Das Repository demonstriert eine serviceorientierte, lokale Agentenarchitektur
statt eines monolithischen Chatbots. Web UI, Orchestrierung, Agent Core,
Memory, RAG und Voice sind fachlich getrennt und über HTTP-Verträge integriert.
Der aktuelle Text-Chat zeigt die vollständige Kette von Browser-Eingabe über
Kontextaufbereitung und Tool-Auswahl bis zur lokalen LLM-Antwort.

### Was das Projekt demonstriert

- **Agent Engineering:** LLM-gesteuerte Auswahl kontrollierter Web-, Wetter-,
  Kalender- und RAG-Werkzeuge.
- **Local-first AI:** lokales, Ollama-kompatibles LLM sowie lokale Memory-,
  RAG- und Kalenderdatenhaltung.
- **Serviceorientierte Architektur:** unabhängige Komponenten mit
  HTTP-Verträgen statt direkter Python-Kopplung.
- **Security by Design:** Prompt-Injection-Abwehr für externe Inhalte,
  SSRF-/Redirect-/Größen-Grenzen beim Webzugriff und sichere HTML-Darstellung.
- **Robuste Ausführung:** kooperative Cancellation, Diagnose-IDs und
  Persistierung ausschließlich abgeschlossener Turns.
- **Human in the Loop:** schreibende Kalenderaktionen verlangen eine sichtbare
  Bestätigung in der UI.
- **Erweiterbarkeit:** Die Voice Runtime kann später denselben
  kanalneutralen Orchestrator verwenden.

> Für ein öffentliches Portfolio sollten ausschließlich Demo-Daten verwendet
> werden. Persönliche Sitzungen, Erinnerungen oder Kalenderdaten gehören nicht
> in Screenshots oder eingecheckte Datenbestände. Ein Screenshot wird erst
> referenziert, wenn die Bilddatei als bereinigtes Demo-Asset vorliegt.

## Ziel und aktueller Stand

Die Plattform stellt eine nachvollziehbare lokale Eingabe–LLM–Ausgabe-Kette
bereit und wird schrittweise um Tools und weitere Kanäle erweitert. Aktuell
testbar sind:

- Browser-Chat mit Sitzungen, Historie, Einstellungen, Fortschrittsanzeige,
  Abbruch und sicher gerenderten Antworten;
- lokales, Ollama-kompatibles LLM über den Core Agent;
- sitzungsbezogene Turns und explizite Langzeitinformationen in SQLite;
- RAG-Suche, Websuche über lokales SearXNG, begrenzter Webseitenabruf,
  Wetterabfrage sowie ein lokaler SQLite-Kalender mit Erinnerungen;
- Quellen- und Aktualitätsabsicherung für zeitkritische oder offizielle Angaben;
- Diagnose-IDs für Fehlerfälle und kooperative Cancellation.

Authentifizierung, Mehrbenutzerbetrieb, Backup/Restore und die vollständige
Voice-Anbindung sind noch offene Betriebs- beziehungsweise Ausbauschritte.

## Architektur

```text
Browser
  |
  v
Web UI :8200  -- Same-Origin-Proxy --> Conversation Orchestrator :8202
                                           |
                                           +--> Memory & Context :8205 (SQLite)
                                           |
                                           v
                                     Core Agent :8201 --> lokales LLM
                                           |
                                           +--> RAG-Local-Adapter (in-process)
                                           +--> SearXNG :8206 --> Suchmaschinen
                                           +--> Web-Scraper / Wetter / Kalender

AI Document Service :8090 <--> Document Demo Client :8181
Voice Runtime :8204 (optional, noch nicht in der Chat-Kette)
```

Alle Plattformdienste binden standardmäßig an `127.0.0.1`. Die zentralen,
nicht geheimen Laufzeiteinstellungen liegen lokal in
`config/platform.settings.json`. RAG Local besitzt zwar eine eigenständige
Anwendung, wird vom aktuellen Chat-Stack aber über einen Adapter im Core
eingebunden; Port 8203 ist deshalb in diesem Workflow nicht gestartet.

## Komponenten

| Bereich | Aufgabe | Dokumentation |
| --- | --- | --- |
| `apps/ai_web_ui_local` | Browser-Chat, Historie, Einstellungen, Quellen und Fortschritt | [README](apps/ai_web_ui_local/README.md) |
| `services/ai_conversation_orchestrator` | Kanalneutraler Turn-Einstieg, Context, Cancellation, Weitergabe | [README](services/ai_conversation_orchestrator/README.md) |
| `apps/ai_agent_core` | LLM-Steuerung, Tools, Freshness- und Quellen-Policy | [README](apps/ai_agent_core/README.md) |
| `services/ai_memory_context_service` | Nutzerbezogene Sessions, Turns und Langzeitinformationen | [README](services/ai_memory_context_service/README.md) |
| `services/ai_rag_local` | Lokale Dokumentindizierung und semantische Suche | [README](services/ai_rag_local/README.md) |
| `services/ai_document_service` | Unabhängige Dokument- und Barcode-Extraktion | [README](services/ai_document_service/README.md) |
| `apps/ai_document_demo_client` | Testclient für den Document Service | [README](apps/ai_document_demo_client/README.md) |
| `apps/ai_voice_agent_local` | Optionaler lokaler Voice-Prototyp | [README](apps/ai_voice_agent_local/README.md) |

## Zusammenspiel und Workflows

### Text-Chat

1. Die Web UI sendet Nachricht, Benutzer- und Sitzungs-ID an den Orchestrator.
2. Der Orchestrator baut beim Memory-Service passenden Kontext und ruft den
   Core über dessen HTTP-Vertrag auf.
3. Der Core verwendet das lokale LLM und nur bei Bedarf erlaubte Tools.
4. Der Orchestrator speichert nur erfolgreich abgeschlossene Turns. Ein
   abgebrochener Run erzeugt keinen Memory-Eintrag.
5. Die UI zeigt Antwort, Tool-Hinweise und Quellen sicher als HTML an.

Details: [lokaler Text-Chat](docs/local_text_chat.md),
[Cancellation](docs/cancellation.md) und [Diagnose-IDs](docs/diagnostic_ids.md).

### Web-Recherche und Quellen

Aktuelle, zeitkritische oder offizielle Angaben lösen verpflichtende Recherche
aus. `web_search` verwendet lokales SearXNG, `web_scrape` ruft anschließend
eine konkrete öffentliche URL mit SSRF-, Größen- und Redirect-Grenzen ab.
Web-Inhalte sind für das LLM Daten, niemals Anweisungen.

Details: [lokale Web-Recherche](docs/local_web_research.md),
[Prompt-Injection-Abwehr](docs/prompt_injection_defence.md) und
[API- und Event-Contracts](docs/api_contracts.md).

### Kalender und Erinnerungen

Der Kalender ist lokal und SQLite-basiert. Ein vorgeschlagener Termin wird erst
nach sichtbarer Bestätigung erstellt. Erinnerungen lassen sich schließen oder
um fünf Minuten verschieben.

Details: [Terminbestätigung](docs/calendar_confirmation.md) und
[Erinnerungen](docs/calendar_reminders.md).

### Dokumente

Der Document Service ist nicht Teil des normalen Agentenpfads. Er extrahiert
Text, Chunks und 2D-Barcodes. Hochgeladene Original-PDFs werden nach der
Extraktion nicht als Produktivdaten aufbewahrt; extrahierte Inhalte liegen in
der Datenhaltung des Services. Testdateien unter
`services/ai_document_service/data/scan` bleiben davon ausgenommen.

## Designentscheidungen

### Local-first statt Cloud-Abhängigkeit

Der zentrale Agentenpfad kann vollständig lokal betrieben werden. Das hält
sensible Kontextdaten unter eigener Kontrolle und reduziert die Abhängigkeit
von externen KI-APIs. Webzugriff bleibt ein explizites, kontrolliertes Tool.

### Orchestrator statt direkter UI-Agent-Kopplung

Die Web UI spricht nicht direkt mit dem Core. Der Orchestrator reichert Turns
mit Kontext an, koordiniert Cancellation und kapselt die Kommunikation. So
kann später auch Voice denselben Agentenpfad wiederverwenden.

### Memory und RAG als getrennte Verantwortlichkeiten

Der Memory-Service verwaltet Konversations- und Langzeitkontext. RAG Local ist
für Dokumentindizierung und semantische Suche zuständig. Die Trennung macht
Tests, Weiterentwicklung und spätere Austauschbarkeit klarer.

### Explizite Tool-Grenzen

Zeitkritische oder externe Informationen werden nicht als implizites
LLM-Wissen behandelt. Die Policy des Core entscheidet, wann Recherche nötig
ist und welche Werkzeuge zulässig sind.

## Bedienung: lokaler Chat

Voraussetzungen sind die Python-Umgebungen der beteiligten Projekte, Docker
Desktop für SearXNG, eine vorhandene `config/platform.settings.json` und ein
dort eingetragener erreichbarer Ollama-kompatibler LLM-Endpunkt.

```powershell
powershell -ExecutionPolicy Bypass -File deployments/agent_platform/windows/local-chat.ps1
```

Danach `http://127.0.0.1:8200` öffnen.

- **Chat:** neue Unterhaltung starten, Sitzungen wählen, Nachricht senden oder
  laufende Verarbeitung abbrechen.
- **Historie:** Sitzungen umbenennen oder löschen sowie einzelne Turns und
  gespeicherte Informationen verwalten.
- **Einstellungen:** lokale Darstellung und Antwortumfang anpassen.

Beenden:

```powershell
powershell -ExecutionPolicy Bypass -File deployments/agent_platform/windows/local-chat.ps1 -Stop
```

SearXNG kann bei Bedarf separat mit
`deployments/agent_platform/windows/local-searxng.ps1` betrieben werden.

## Roadmap

- Voice Runtime an den Conversation Orchestrator anbinden;
- Streaming-STT/TTS und Barge-in in den gemeinsamen Turn-Lifecycle integrieren;
- Authentifizierung und Mehrbenutzerbetrieb ergänzen;
- Backup-/Restore-Strategie für lokale persistente Daten definieren;
- reproduzierbare Deployment- und Offline-Installationspfade ausbauen;
- End-to-End- und Failure-Mode-Tests über mehrere Services erweitern.

## Dokumentationskarte

- [Plattformgrundlagen und Konfiguration](docs/platform_foundation.md)
- [Lokaler Text-Chat](docs/local_text_chat.md)
- [Web-Recherche und Scraper-Grenzen](docs/local_web_research.md)
- [API- und Event-Contracts](docs/api_contracts.md)
- [Cancellation](docs/cancellation.md)
- [Diagnose-IDs](docs/diagnostic_ids.md)
- [Prompt-Injection-Abwehr](docs/prompt_injection_defence.md)
- [Terminbestätigung](docs/calendar_confirmation.md) und [Erinnerungen](docs/calendar_reminders.md)
- [Architektur- und Entwicklungspläne](docs/plans/)
