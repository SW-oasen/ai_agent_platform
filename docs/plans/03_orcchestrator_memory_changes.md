# Orchestrung und Memory Architekturplanung

## Ziel:
Conversation Memory wird aus der Voice Runtime herausgelöst und als
plattformweiter, kanal- und agentenunabhängiger Service geführt.

## Zielarchitektur:

Voice / Chat / zukünftige Channels
             |
             v
Conversation- und Session-Orchestrator
        |                         |
        v                         v
Memory- und Context-Service      Core Agent
        |                         |
        +---- Context ----------> ModelProvider
                                   |- Ollama
                                   |- OpenAI-kompatibel
                                   `- weitere Provider

## Verantwortlichkeiten:

- Voice Runtime:
  Audioaufnahme, VAD, Turn Detection, STT, TTS, Barge-in, kanalbezogene Events.
  Kein persistentes User-Memory, keine Kontextkonstruktion, keine direkten
  Ollama-/OpenAI-Clients.

- Conversation-/Session-Orchestrator:
  Authentifizierte User- und Session-Zuordnung, Turn-Lifecycle, Context-Anfrage,
  Weiterleitung des Antwortstreams an den jeweiligen Kanal.

- Memory-/Context-Service:
  Session-/Working-Memory, episodisches Memory, semantisches Langzeit-Memory,
  Retrieval, Kontextaufbau, asynchrone Kandidatenextraktion, Datenschutz- und
  Löschoperationen.

- Core Agent:
  Task-Ausführung, Tool-Nutzung, Antwortstream und Auswahl des Modellproviders.
  Kein globales User-Memory und keine Voice-Wiedergabe.

- ModelProvider:
  Einheitliche Schnittstelle für Streaming, Cancellation, Timeouts,
  Modellwahl und strukturierte Fehler. Implementierungen: Ollama,
  OpenAI-kompatibel, später weitere lokale/remote Provider.

## Erforderliche Contracts:

- Jede Anfrage enthält authenticated user_id, session_id, turn_id und
  correlation_id.
- Memory-/Context-Service:
  build_context(...)
  record_turn(...)
  list_memory(...)
  delete_memory(...)
  clear_memory(...)
- Core Agent:
  stream_response(context, cancellation)

## Ablauf pro Turn:

1. Kanal erzeugt einen User-Turn.
2. Orchestrator validiert User und Session.
3. Orchestrator fordert relevanten Context beim Memory-Service an.
4. Orchestrator ruft Core Agent mit diesem Context auf.
5. Core Agent streamt Antwort zurück.
6. Orchestrator routet den Stream an Voice/Chat.
7. Memory-Kandidaten werden nach/parallel zur Antwort asynchron extrahiert und
   gespeichert; sie dürfen First-Token bzw. First-Audio nicht verzögern.

## Memory-Regeln:

- Session: aktuelle Turns, Task-State, Topic, temporäre Fakten, offene Fragen,
  Rolling Summary.
- Episodisch: zeitgebundene Zusammenfassungen früherer Sessions und Outcomes.
- Semantisch: stabile Fakten/Präferenzen mit hoher Konfidenz.
- RAG-/Wissensdaten bleiben von persönlichem Conversation-Memory getrennt.
- Neue strukturierte Fakten superseden alte Werte; alte Werte behalten
  Validität/Provenance, werden aber nicht mehr als aktuell abgerufen.
- Deterministisches strukturiertes Retrieval zuerst; keine Vektordatenbank ohne
  nachgewiesenen Bedarf.

## Migration aus ai_voice_agent_local:

1. Contracts und Identity-Modell in agent_platform definieren.
2. voice_runtime.memory und voice_runtime.conversation in den Plattformservice
   überführen bzw. als lokale Prototypen ablösen.
3. Voice Runtime nutzt anschließend nur einen Orchestrator-Client.
4. DummyAgent bleibt ausschließlich für isolierte Voice-/Audio-Tests.
5. Ollama- und OpenAI-kompatible Adapter werden im Core Agent implementiert.
6. Contract- und Migrations-Tests für Voice → Orchestrator → Core Agent ergänzen.

## Datenschutz:

- Memory immer strikt user-scoped.
- APIs für Anzeigen, Einzel-Löschen, Session-Löschen und vollständiges Löschen.
- Löschvorgänge auditierbar, ohne gelöschten Inhalt weiter vorzuhalten.
- Retention, Export und Berechtigungen zentral in der Plattform definieren.