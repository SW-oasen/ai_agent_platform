# API- und Event-Contracts

## Versionierung

Die aktuelle Contract-Version steht in `contracts/VERSION`. API- und
Event-Änderungen werden dort gemeinsam versioniert. Additive, optionale Felder
sind Minor-Änderungen; entfernte oder neue Pflichtfelder sind Major-Änderungen.

## Core Agent

`agent-api.openapi.yaml` beschreibt den lokalen HTTP-/SSE-Rand des Core Agent.
Ein Run benötigt stets `user_id`, `session_id`, `turn_id` und
`correlation_id`; damit bleiben Kanal, Orchestrator, Memory und Logs eindeutig
zuordenbar. Streaming-Payloads entsprechen `agent-events.schema.json`.

Wichtige Endpunkte:

- `GET /v1/health`
- `GET /v1/tools`
- `POST /v1/chat`
- `POST /v1/chat/stream`

## Voice Runtime und Orchestrator

`voice-ws.asyncapi.yaml` dokumentiert die lokale WebSocket-Verbindung.
Voice Runtime sendet `turn.submitted` und `run.cancel_requested`; der
Orchestrator liefert Agent-Events zurück. Die Nachrichtenform ist in
`voice-orchestrator.schema.json` beziehungsweise `agent-events.schema.json`
definiert.

`orchestrator-api.openapi.yaml` beschreibt den lokalen HTTP-Rand für
kanalneutrale Text- und Voice-Turns. Der Orchestrator validiert die
Plattform-Identitäten, erweitert Context über den Memory-/Context-Rand und
leitet den Run über die öffentliche Core-Agent-API weiter.

`memory-context-api.openapi.yaml` beschreibt den getrennten, user-skoped
Memory-/Context-Dienst. Er hält Gesprächs- und explizite Memory-Daten, aber
keine RAG-Dokumentbibliothek oder Voice-spezifischen Zustand.

## Konfiguration

`platform-settings.schema.json` definiert die nicht-geheime Plattform-
Konfiguration. Das optionale Control-Plane-API ist in
`configuration-api.openapi.yaml` beschrieben. Geheimnisse gehören weder in
Settings-Dateien noch in Contract-Payloads.

## RAG und Document Service

`ai_rag_local` wird später über einen eigenen, versionierten Retrieval-Contract
als read-only Tool angebunden. Der unabhängige Kundenprototyp
`ai_document_service` behält seinen separaten Vertrag unter
`contracts/document_api/openapi.yaml` und ist kein Teil des Agent-Core-Contracts.
