# AI Agent Platform – Architekturplanung

**Status:** in Umsetzung – Platform Foundation und Ziel-Contracts sind begonnen; die Komponentenmigration folgt.  
**Bezug:** `01_platform_foundation_and_versioning.md`  
**Ziel:** Eine lokale, modulare AI-Agent-Plattform entwickeln, die Text- und Spracheingabe über denselben Agent Core verarbeitet und später kontrolliert um RAG, MCP-Tools, LangGraph-Workflows und spezialisierte Agenten erweitert werden kann.

---

## 1. Leitidee

Die Plattform soll **nicht möglichst viele AI-Frameworks demonstrieren**, sondern eine nachvollziehbare Applied-AI-Architektur mit klaren Verantwortlichkeiten bilden.

Grundprinzipien:

1. **Core Agent zuerst selbst implementieren.**
   - Tool Registry
   - Tool Selection / Tool Calling
   - strukturierte Tool-Argumente
   - Fehlerbehandlung und Retries
   - Session-/Conversation-State
   - Streaming
   - Observability und Evaluation

2. **Frameworks erst dort integrieren, wo sie echten Mehrwert liefern.**
   - LlamaIndex optional innerhalb des RAG-Service
   - LangGraph später für zustandsbehaftete Workflows und Supervisor-/Subagent-Orchestrierung
   - kein zusätzliches Multi-Agent-Framework wie CrewAI, solange LangGraph den tatsächlichen Bedarf abdeckt

3. **Voice ist ein optionaler Ein-/Ausgabekanal, kein eigener Agent.**
   - Text und Voice verwenden denselben Core Agent und dieselbe Session.
   - STT erzeugt Texteingabe für den Core.
   - Core-Ausgaben werden optional über TTS gesprochen.

4. **Backend-Komponenten kommunizieren ausschließlich über definierte Verträge.**
   - keine direkten Python-Imports zwischen Repositories
   - HTTP/SSE/WebSocket/MCP je nach Kommunikationsart
   - Contracts werden im Platform-Repository versioniert

5. **Local first.**
   - lokale LLMs über Ollama bzw. austauschbare Model Adapter
   - lokale STT-/TTS-/RAG-Komponenten
   - Cloud-Komponenten können später als alternative Adapter ergänzt werden

---

## 2. Plattformgrenzen und Repository-Struktur

Ausgangsstruktur:

```text
ai_agent_platform/
├─ apps/
│  ├─ ai_agent_local/        # Core Agent
│  ├─ ai_voice_agent_local/  # Voice Runtime
│  └─ ai_web_ui_local/       # Web UI
│
├─ services/
│  └─ ai_rag_local/          # RAG Service
│
├─ contracts/
├─ docs/
├─ scripts/
├─ tests/
├─ compose.yaml              # später / optional
├─ workspace.lock.yaml
└─ .env.example
```

Alle vier Komponenten bleiben eigenständige Git-Repositories. Das übergeordnete `ai_agent_platform`-Repository enthält ausschließlich Integrationslogik, Verträge, Dokumentation, Start-/Diagnosekonfiguration und Integrations-/E2E-Tests.

### Optionale spätere semantische Bereinigung

Langfristig wäre folgende Benennung fachlich noch klarer:

```text
apps/
└─ ai_web_ui_local/

services/
├─ ai_agent_local/
├─ ai_voice_agent_local/
└─ ai_rag_local/
```

Begründung: Web UI ist die eigentliche User Application; Core Agent, Voice Runtime und RAG sind Backend-Services.

**Diese Verschiebung ist nicht zwingend.** Solange die vorhandene Struktur bereits verwendet wird, soll sie nicht nur aus kosmetischen Gründen geändert werden. Wichtig sind die logischen Grenzen, nicht der Verzeichnisname.

---

## 3. Zielarchitektur

```text
                           USER
                            │
                    ┌───────▼────────┐
                    │     Web UI     │
                    │ Text + Audio   │
                    └───────┬────────┘
                            │
             ┌──────────────┴──────────────┐
             │                             │
          Text/SSE                     Audio/WS
             │                             │
             │                    ┌────────▼────────┐
             │                    │ Voice Runtime  │
             │                    │ VAD / STT / EOT│
             │                    │ TTS / Barge-in │
             │                    └────────┬────────┘
             │                             │ text
             └──────────────┬──────────────┘
                            │
                    ┌───────▼────────┐
                    │   Core Agent   │
                    │                │
                    │ Session/State  │
                    │ Agent Loop     │
                    │ Tool Registry  │
                    │ Validation     │
                    │ Streaming      │
                    └───────┬────────┘
                            │
              ┌─────────────┼──────────────────┐
              │             │                  │
              ▼             ▼                  ▼
         MCP Tools       RAG Service      Workflows
                           │                  │
                           │              LangGraph
                           │                  │
                           ▼            Supervisor /
                     LlamaIndex?         Subagents
                     Vector DB
                     Documents
```

### Zentrale Architekturregel

**Nur der Core Agent entscheidet über fachliche Agent-/Tool-Ausführung.**

Die Web UI ruft deshalb nicht direkt RAG, Forecasting oder andere MCP-Tools auf. Die UI darf deren Status und Resultate darstellen, aber die Orchestrierung bleibt im Core Agent.

Dadurch gibt es genau einen Ort für:

- Tool Selection
- Authorization / Bestätigung
- Session Context
- Fehlerbehandlung
- Agent Evaluation
- Logging / Tracing
- spätere Supervisor-Entscheidungen

---

## 4. Verantwortlichkeiten der Komponenten

## 4.1 Web UI

### Verantwortlich für

- Chat-Oberfläche
- Texteingabe
- optionale Audioaufnahme
- Anzeige gestreamter Agent-Antworten
- Anzeige von Tool- und Workflow-Status
- Wiedergabe von TTS-Audio
- Session-Auswahl bzw. Session-Neuanlage
- Stop/Cancel
- Benutzerbestätigung bei Side Effects

### Nicht verantwortlich für

- Tool Selection
- Prompt-/Agent-Logik
- Retrieval
- direkte Modellaufrufe
- fachliche Workflow-Steuerung
- Speicherung des kanonischen Agent-State

### Grundsatz

Text und Audio sind zwei Eingabemodalitäten derselben Unterhaltung:

```text
Typed Text ──────────────┐
                         ├──> Core Agent Session
Audio -> STT -> Text ────┘
```

Die UI muss daher keine getrennten "Text-Agent"- und "Voice-Agent"-Modi kennen.

---

## 4.2 Voice Runtime

Die Voice Runtime ist eine **Media-/Realtime-Schicht**, kein fachlicher Agent.

### Verantwortlich für

- Audio Input
- VAD
- Streaming STT
- End-of-Turn Detection
- Übergabe finaler/geeigneter Transkripte an den Core
- Streaming TTS
- Audio Output
- Barge-in
- Abbruch laufender TTS-Ausgabe
- Audio-/Voice-Latenzmetriken

### Nicht verantwortlich für

- Tool Selection
- RAG
- fachliche Agent Memory
- Multi-Agent Routing
- Business Workflows

### Wichtige Trennung

```text
Voice Runtime = Wie kommuniziert der Benutzer?
Core Agent    = Was soll das System tun?
```

Diese Trennung muss auch dann bestehen bleiben, wenn beide Komponenten lokal auf demselben Rechner laufen.

---

## 4.3 Core Agent

Der Core Agent ist das **fachliche Zentrum der Plattform**.

### Phase-1-Verantwortlichkeiten

- LLM-/Model-Adapter
- System Prompt / Agent Policy
- Conversation Session
- Message History
- Streaming Response
- Tool Registry
- strukturierte Tool-Schemas
- Tool Selection
- Tool Execution
- Tool Result Normalization
- Timeout
- Retry
- Failure Mapping
- Cancellation
- einfache Guardrails
- Confirmation Gate für Side Effects
- Telemetrie / Evaluation Events

### Bewusste Nicht-Ziele der ersten Version

- LangGraph als Voraussetzung für den Agent Loop
- Multi-Agent-System
- komplexe Planning-Hierarchie
- autonomes Langzeit-Task-Management
- versteckte automatische Side Effects

Der erste Core soll klein genug bleiben, dass sein Verhalten verstanden, getestet und gemessen werden kann.

---

## 4.4 RAG Service

Der RAG-Service kapselt Retrieval vollständig vom Core Agent ab.

```text
Core Agent
    │
    │ retrieve(query, filters, ...)
    ▼
RAG Service
    │
    ├── Ingestion
    ├── Chunking
    ├── Embeddings
    ├── Vector Search
    ├── optional Hybrid Search
    ├── optional Reranking
    └── Sources / Citations
```

### LlamaIndex

LlamaIndex kann später **innerhalb dieses Services** eingesetzt werden, wenn es gegenüber der vorhandenen Implementierung Vorteile bringt, beispielsweise für:

- Daten-Connectoren
- Index-Abstraktion
- Retrieval Pipelines
- Hybrid Retrieval
- Reranking
- Source Nodes / Metadaten

Der Core Agent darf nicht davon abhängig sein, ob intern LlamaIndex, eine eigene Retrieval-Pipeline oder eine andere Bibliothek verwendet wird.

Das öffentliche RAG-Interface bleibt stabil.

---

## 5. Tool- und MCP-Architektur

Der Core Agent erhält eine zentrale Tool Registry.

```text
Tool Registry
├─ local/internal tools
├─ MCP tools
├─ RAG tool
├─ data/ML tools
└─ action tools
```

Jedes Tool benötigt mindestens:

- eindeutigen Namen
- Beschreibung
- Input Schema
- Output Schema bzw. normalisiertes Resultat
- Timeout Policy
- Error Mapping
- Side-Effect-Klassifikation
- optional Permission-/Confirmation Policy

Beispielhafte Klassifikation:

```text
READ_ONLY
├─ weather.search
├─ rag.search
└─ forecast.get

SIDE_EFFECT
├─ calendar.create_event
├─ email.send
└─ filesystem.write
```

Für Side-Effect-Tools gilt grundsätzlich:

```text
Agent proposes action
        │
        ▼
User confirmation
        │
    ┌───┴───┐
   yes      no
    │        │
execute    cancel
```

---

## 6. Session- und State-Modell

Die `session_id` ist modalitätsunabhängig.

```text
session_id
├─ text messages
├─ STT transcripts
├─ assistant messages
├─ tool calls
├─ tool results
├─ workflow events
└─ confirmation events
```

### Wichtige Regel

Die Voice Runtime besitzt **keine zweite Conversation History**.

Sie darf technischen Voice-State besitzen, beispielsweise:

- aktueller Audio Stream
- VAD State
- STT Buffer
- TTS Queue
- interruption state

Der kanonische Conversation-/Agent-State gehört dagegen zum Core Agent.

---

## 7. Streaming- und Event-Modell

Für den Core Agent sollte früh ein einheitliches Event-Modell definiert werden.

Mögliche Events:

```text
session.started
user.message.received
agent.started
agent.token.delta
agent.message.completed

tool.requested
tool.started
tool.completed
tool.failed

confirmation.required
confirmation.accepted
confirmation.rejected

workflow.started
workflow.step.started
workflow.step.completed
workflow.completed
workflow.failed

agent.error
agent.cancelled
```

Voice-spezifische Events bleiben zusätzlich im Voice Contract:

```text
voice.connected
voice.speech.started
voice.speech.ended
voice.transcript.partial
voice.transcript.final
voice.tts.started
voice.tts.interrupted
voice.tts.completed
```

Die UI kann dadurch Tool- und Workflow-Aktivität darstellen, ohne interne Agent-Implementierung kennen zu müssen.

---

## 8. LangGraph – geplante Integrationsstufe

LangGraph wird **nicht als erstes Core-Fundament** eingeführt.

Zuerst wird der einfache Agent Loop selbst implementiert und evaluiert.

### LangGraph wird sinnvoll, sobald mindestens einer dieser Fälle real benötigt wird

- mehrere zustandsbehaftete Schritte
- explizite Workflow-Zustände
- Branching
- Retry/Fallback-Pfade
- Human-in-the-loop
- persistierbare/resumierbare Abläufe
- Parallelisierung
- spezialisierte Subagents
- Supervisor Routing

### Beispiel späterer Workflow

```text
START
  │
  ▼
understand_request
  │
  ▼
select_capability
  │
  ├── direct answer ───────────────┐
  │                                │
  ├── tool call -> validate ───────┤
  │                                │
  └── workflow -> execute ─────────┤
                                   │
                                   ▼
                              final_response
                                   │
                                   ▼
                                  END
```

---

## 9. Späteres Supervisor-/Multi-Agent-Modell

Kein Multi-Agent-System nur als Demonstration.

Spezialisierte Agenten werden erst eingeführt, wenn unterschiedliche Rollen tatsächlich unterschiedliche:

- Tools
- Kontexte
- System Prompts
- Datenquellen
- Berechtigungen
- Evaluationskriterien

benötigen.

Mögliches späteres Modell:

```text
                     Core / Supervisor
                            │
             ┌──────────────┼───────────────┐
             │              │               │
             ▼              ▼               ▼
       Research Agent   Data/ML Agent   Action Agent
             │              │               │
         Web + RAG      Python + ML      Calendar etc.
             │              │               │
             └──────────────┼───────────────┘
                            │
                            ▼
                      Supervisor result
```

### Supervisor-Verantwortung

- Delegationsentscheidung
- Kontextübergabe
- Aggregation der Ergebnisse
- Fehler-/Fallback-Entscheidung
- Bestätigungsanforderungen
- finaler Response

### Spezialagent-Verantwortung

- eng begrenzte Domänenaufgabe
- eigener kleiner Kontext
- nur notwendige Tools
- keine globale Plattformorchestrierung

---

## 10. Skills

Skills werden nicht global nach dem Prinzip "mehr ist besser" installiert.

### Core Agent

Nur generische Fähigkeiten, z. B.:

- Planning / Task Decomposition, falls wirklich benötigt
- Tool Usage Policy
- Context Management
- Error Recovery
- ggf. Delegation

### Research Agent

Später beispielsweise:

- Retrieval
- Web Research
- Citation / Grounding

### Data/ML Agent

Später beispielsweise:

- Data Analysis
- Python execution
- Forecast interpretation

### Action Agent

Später beispielsweise:

- Calendar
- Mail
- andere Side-Effect-Tools

### Voice Runtime

Keine fachlichen Agent Skills.

---

## 11. Observability und Evaluation

Evaluation ist ein Architekturteil und kein späteres Add-on.

Mindestens folgende Metriken vorsehen:

### Agent

- Tool Selection Accuracy
- Tool Argument Accuracy
- Hallucinated Tool Calls
- Tool Failure Rate
- Retry Rate
- Task Success Rate
- Average Agent Latency
- LLM Inference Latency

### RAG

- Retrieval Relevance
- Source Coverage
- Citation Correctness
- Empty Retrieval Rate
- Retrieval Latency

### Voice

- STT Latency
- TTS Time-to-First-Audio
- End-of-Turn Latency
- Barge-in Interruption Latency
- STT Error Samples / optional WER Testset

### Workflow

- Workflow Success Rate
- Step Failure Rate
- Retry Count
- Human Confirmation Rate
- End-to-End Latency

Die Event Contracts sollten genügend IDs enthalten, um einen Lauf Ende-zu-Ende zu korrelieren:

```text
session_id
run_id
message_id
tool_call_id
workflow_id / workflow_run_id
trace_id          # optional / später
```

---

## 12. Fehler- und Abbruchmodell

Fehler sollen nicht als beliebige Strings durch die Plattform wandern.

Vorgesehene Kategorien:

```text
MODEL_ERROR
TOOL_ERROR
TOOL_TIMEOUT
TOOL_VALIDATION_ERROR
RAG_ERROR
WORKFLOW_ERROR
VOICE_ERROR
USER_CANCELLED
CONFIRMATION_REJECTED
INTERNAL_ERROR
```

Jeder Fehler sollte mindestens besitzen:

```text
code
message
retryable
source
run_id
optional details
```

Interne Details, Stacktraces und Secrets werden nicht ungefiltert an die UI gesendet.

---

## 13. Cancellation als Querschnittsfunktion

Gerade wegen Voice/Barge-in sollte Cancellation früh berücksichtigt werden.

```text
User stop / barge-in
        │
        ▼
Cancel current output
        │
        ├── stop TTS immediately
        ├── stop streaming response when appropriate
        └── propagate cancellation to running operation where supported
```

Nicht jede Tool-Ausführung kann technisch sicher abgebrochen werden. Deshalb muss zwischen:

- UI-/Output-Cancellation
- Agent-Run-Cancellation
- Tool-Cancellation

unterschieden werden.

---

## 14. API-/Contract-Plan

Die bereits geplanten Contracts bleiben Grundlage:

```text
contracts/
├─ VERSION
├─ agent-api.openapi.yaml
├─ agent-events.schema.json
└─ voice-ws.asyncapi.yaml
```

Empfohlene Ergänzung, sobald MCP/Tools stabilisiert werden:

```text
contracts/
└─ examples/
   ├─ chat_request.json
   ├─ tool_started.json
   ├─ tool_completed.json
   ├─ confirmation_required.json
   └─ error.json
```

Contract-Beispiele sollten Bestandteil der Integrations-/Contract-Tests sein und nicht nur Dokumentation darstellen.

---

## 15. Security- und Permission-Grundmodell

Auch lokal soll die Architektur Side Effects explizit behandeln.

### Tool Metadata

```text
read_only: true/false
requires_confirmation: true/false
allowed_scopes: [...]
```

Später möglich:

- Tool Allowlist pro Agent
- Permission Scopes pro Session/User
- Sandbox für Code Execution
- Dateisystemzugriff nur auf explizite Pfade
- Netzwerkzugriff nur für freigegebene Tools

Keine dieser Erweiterungen muss die erste Core-Version blockieren; die Tool-Schnittstelle sollte sie jedoch ermöglichen.

---

## 16. Konfigurationsstrategie

Platform-Konfiguration beschreibt nur Integration und Discovery.

Beispiel:

```text
WEB_UI_URL=http://127.0.0.1:8200
AGENT_API_URL=http://127.0.0.1:8201
ORCHESTRATOR_URL=http://127.0.0.1:8202
RAG_API_URL=http://127.0.0.1:8203
VOICE_WS_URL=ws://127.0.0.1:8204
OLLAMA_BASE_URL=http://localhost:11434
```

Komponentenspezifische Modellparameter bleiben möglichst im jeweiligen Repository.

Keine Secrets im Platform-Repository.

---

## 17. Empfohlene Implementierungsreihenfolge

## Phase A – Platform Foundation

Bestehende Planung aus `01_platform_foundation_and_versioning.md` umsetzen:

- Platform Repository
- Workspace-Struktur
- `.gitignore`
- `workspace.lock.yaml`
- Konfiguration
- Diagnose-Skript
- Basis-Contracts

**Ergebnis:** reproduzierbarer Multi-Repo-Workspace.

---

## Phase B – Core Agent Foundation

Implementieren:

1. Model Adapter
2. einfache Chat Session
3. Streaming
4. Tool Interface + Registry
5. ein sehr einfaches Test-Tool
6. strukturierte Tool Calls
7. Tool Result Normalization
8. Timeout + Retry Policy
9. Error Model
10. Cancellation
11. Agent Events
12. Basis-Evaluation

**Noch kein LangGraph.**

**Akzeptanz:** Der Core kann deterministisch getestet ein oder mehrere registrierte Tools aufrufen, Fehler sichtbar behandeln und den Run vollständig streamen/protokollieren.

---

## Phase C – Web UI Integration

- textbasierter Chat gegen Core API
- SSE-Streaming
- Sessions
- Tool-Status
- Cancel
- Confirmation UI vorbereiten

**Akzeptanz:** Vollwertige Textinteraktion läuft ausschließlich über die öffentlichen Core Contracts.

---

## Phase D – Voice als optionale Modalität

- Browser Audio -> Voice Runtime
- VAD
- STT
- End-of-Turn
- Übergabe Transcript + `session_id` an Core
- Core Streaming Result
- TTS
- Barge-in

**Akzeptanz:** Derselbe Chat kann innerhalb derselben Session abwechselnd per Text und Sprache geführt werden.

---

## Phase E – RAG Service

- stabiles RAG API / MCP Interface
- Quellen und Metadaten im Resultat
- Integration als Tool des Core Agent
- Retrieval Evaluation

Danach prüfen, ob LlamaIndex gegenüber der vorhandenen Implementierung einen konkreten Nutzen bringt.

**Akzeptanz:** Der Core kennt nur den RAG Contract, nicht dessen interne Bibliotheken.

---

## Phase F – Reale MCP Tools

Schrittweise wenige sinnvolle Tools ergänzen, z. B.:

1. Web/Search bzw. aktueller Informationszugriff
2. Calendar
3. RAG
4. vorhandenes Electricity Forecasting als ML-Tool

Nicht die Anzahl der Tools optimieren, sondern deren zuverlässige Verwendung evaluieren.

**Akzeptanz:** Tool Selection, Argumente, Failures und Side Effects sind messbar und testbar.

---

## Phase G – LangGraph Workflow

Erst einen echten mehrstufigen Use Case auswählen.

Beispiel:

```text
request
  -> gather context
  -> retrieve information
  -> validate
  -> optional action proposal
  -> confirmation
  -> execute
  -> final answer
```

LangGraph für diesen Workflow integrieren, ohne den kompletten bestehenden Core unnötig umzubauen.

**Akzeptanz:** Der Workflow demonstriert gegenüber dem einfachen Agent Loop einen klaren funktionalen Vorteil.

---

## Phase H – LangGraph Supervisor / spezialisierte Agenten

Nur bei realem Bedarf.

Erster sinnvoller Kandidat:

```text
Supervisor
├─ Research Agent
└─ Data/ML Agent
```

Action Agent erst ergänzen, wenn genügend echte Side-Effect-Tools vorhanden sind.

**Akzeptanz:** Für ein definiertes Testset ist Delegation zuverlässiger oder kontextärmer als dieselbe Aufgabe mit einem einzigen universellen Agenten.

---

## 18. Teststrategie

### Unit Tests je Repository

Komponenten testen ihre interne Logik selbst.

### Contract Tests im Platform-Repository

Prüfen:

- Requests gegen OpenAPI
- Events gegen Schema
- Voice Messages gegen AsyncAPI/Schema
- Error Payloads
- additive Contract-Kompatibilität

### Integration Tests

Mit Fake-Komponenten testen:

```text
Fake UI -> Core -> Fake Tool
Fake Voice -> Core -> Fake TTS
Core -> Fake RAG
```

### End-to-End Tests

Später:

```text
Web UI
 -> Voice/STT optional
 -> Core
 -> Tool/RAG/Workflow
 -> Core response
 -> TTS optional
```

Die Tests sollten ohne echtes großes LLM ausführbar sein können, wo deterministische Fake-Modelle sinnvoll sind.

---

## 19. Architekturentscheidungen, die ausdrücklich vermieden werden sollen

- kein Framework nur für Portfolio-Buzzwords
- kein Multi-Agent-System ohne fachliche Trennung
- kein direkter UI-Zugriff auf RAG/ML/MCP-Services
- keine Business-/Agent-Logik in der Voice Runtime
- keine zweite Session-History für Voice
- keine direkten Python-Imports zwischen Repositories
- keine Framework-spezifischen Datentypen in öffentlichen Contracts
- keine global installierten Skills für jeden Agenten
- keine automatische Ausführung kritischer Side Effects ohne Policy/Bestätigung
- kein Big-Bang-Umbau des Core beim späteren LangGraph-Einsatz

---

## 20. Wichtigste Architekturverbesserungen gegenüber dem bisherigen Entwurf

### 1. Web UI als einziger Benutzer-Entry-Point, nicht als direkter Service-Hub

Die UI bietet Zugriff auf die Fähigkeiten der gesamten Plattform, aber **über den Core Agent**. Dadurch bleiben RAG, Tools und Spezialagenten austauschbar.

### 2. Voice explizit als Transport-/Media-Layer definieren

Audio ist optionale Modalität innerhalb derselben Session und kein zweites Agent-System.

### 3. Core Agent besitzt den kanonischen Conversation-State

Das verhindert divergierende Text-/Voice-Historien.

### 4. Cancellation früh als Plattformfunktion aufnehmen

Für Voice/Barge-in ist saubere Cancellation architektonisch wichtiger als viele zusätzliche Tools.

### 5. Side-Effect-Policy bereits im Tool Contract vorsehen

Dadurch können Calendar/Mail später sauber ergänzt werden, ohne das Tool-Modell neu zu entwerfen.

### 6. LangGraph als gezielte Workflow-/Supervisor-Schicht statt als Grundvoraussetzung

So bleibt nachvollziehbar, welche Agent-Funktionalität selbst implementiert wurde und welchen zusätzlichen Nutzen das Framework später liefert.

### 7. LlamaIndex hinter dem RAG Contract kapseln

Damit bleibt Retrieval-Technologie austauschbar.

### 8. Evaluation IDs und Events von Beginn an einplanen

Das ermöglicht später objektive Aussagen über Tool Selection, Latency, RAG und Barge-in statt nur Demo-Screenshots.

---

## 21. Zielbild für das Portfolio

Die technische Geschichte des Projekts soll am Ende nicht lauten:

> "Mehrere AI-Frameworks wurden miteinander verbunden."

Sondern:

> Eine modulare lokale AI-Agent-Plattform wurde von einem selbst implementierten Agent Core aus aufgebaut. Text und Voice verwenden dieselbe Session und dieselbe Tool-/Workflow-Orchestrierung. Externe Fähigkeiten werden über stabile Contracts und MCP angebunden. RAG ist als eigenständiger Service gekapselt. Komplexe zustandsbehaftete Abläufe und spezialisierte Agenten werden nur dort mit LangGraph ergänzt, wo sie gegenüber dem einfachen Agent Loop einen messbaren Vorteil bieten.

Das Projekt soll damit insbesondere folgende Engineering-Kompetenzen zeigen:

- Agent Architecture
- Local LLM Integration
- Tool Calling
- MCP
- RAG
- Streaming
- Realtime Voice
- Barge-in / Cancellation
- API-/Event-Design
- Multi-Repo Integration
- Contract Testing
- Agent Evaluation
- Workflow Orchestration
- später kontrolliertes Multi-Agent Routing

---

## 22. Nächster konkreter Umsetzungsschritt für Codex

Nach Fertigstellung der Platform-Foundation soll **nicht sofort LangGraph integriert** werden.

Codex soll zunächst im Core-Agent-Repository prüfen bzw. implementieren:

1. vorhandene Agent-Loop-Struktur dokumentieren
2. Model Adapter klar vom Agent Loop trennen
3. ein einheitliches `Tool`-/`ToolSpec`-Interface definieren
4. Tool Registry implementieren
5. strukturierten Tool Call + Argumentvalidierung implementieren
6. ein deterministisches Dummy/Test-Tool hinzufügen
7. Tool Execution Result vereinheitlichen
8. Fehler-/Timeout-Modell implementieren
9. Run-/Tool-Events erzeugen
10. `session_id`, `run_id`, `message_id`, `tool_call_id` konsequent propagieren
11. Cancellation-Schnittstelle vorsehen
12. Unit Tests und erste Agent-Evaluation für Tool Selection/Arguments ergänzen

Dabei öffentliche Interfaces klein halten und **keine LangChain-/LangGraph-/LlamaIndex-spezifischen Typen in die Platform Contracts übernehmen**.
