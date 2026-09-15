# Plattform-Fundament und Versionierungsstrategie

**Status:** in Umsetzung – Foundation-Artefakte angelegt; Komponenten-Revisionen und integrierter Start stehen noch aus.  
**Ziel:** Die lokale AI-Agent-Plattform reproduzierbar integrieren, ohne die Eigenständigkeit der beteiligten Git-Repositories zu verlieren.

## Ausgangslage

Die Plattform bündelt mehrere eigenständige Komponenten in einem lokalen Arbeitsbereich:

```text
ai_agent_platform/
├─ apps/
│  ├─ ai_agent_local/        # Core Agent
│  ├─ ai_voice_agent_local/  # Voice Runtime
│  └─ ai_web_ui_local/       # künftig: Web UI
├─ services/
│  └─ ai_rag_local/          # RAG-Service bzw. RAG-Adapter-Quelle
├─ contracts/
└─ docs/
```

Core Agent, Voice Runtime, RAG und Web UI bleiben eigene Repositories. Der Platform-Ordner ist der Ort für ihre Zusammenschaltung, nicht für eine Kopie ihrer Anwendungslogik.

## Architekturentscheidung: eigenes Platform-Repository

`ai_agent_platform` wird selbst ein **eigenes Git-Repository**. Es versioniert ausschließlich:

- API- und Event-Verträge in `contracts/`
- Architektur-, Betriebs- und Entwicklungsdokumentation
- lokale Start- und Diagnose-Skripte
- `compose.yaml` oder spätere Deployment-Konfiguration
- `.env.example` ohne Geheimnisse
- Integrations- und Contract-Tests
- einen Commit-Lock der verwendeten Komponenten

Die Verzeichnisse `apps/` und `services/` bleiben lokale Arbeitskopien ihrer eigenen Repositories und werden im Platform-Repository ignoriert. Damit gibt es keine Git-Submodule und keine versehentlich in das Parent-Repository übernommenen Child-Dateien.

Geplante `.gitignore` des Platform-Repositorys:

```gitignore
apps/
services/
.env
data/
*.log
```

## Reproduzierbare Zusammenschaltung

Eine versionierte Datei `workspace.lock.yaml` hält fest, welche Revisionsstände gemeinsam getestet wurden:

```yaml
components:
  core_agent:
    path: apps/ai_agent_local
    remote: https://github.com/<organisation>/ai-agent-core.git
    ref: <commit-sha>
  voice_runtime:
    path: apps/ai_voice_agent_local
    remote: https://github.com/<organisation>/ai-voice-runtime.git
    ref: <commit-sha>
  rag:
    path: services/ai_rag_local
    remote: https://github.com/<organisation>/ai-rag.git
    ref: <commit-sha>
```

Der Lock ist kein Ersatz für Git. Er dokumentiert lediglich die Kombination von Commits, gegen die Integrations- und End-to-End-Tests erfolgreich waren. Bei jeder getesteten Plattformversion wird er aktualisiert und zusammen mit Contracts und Startkonfiguration committed.

## Versionierungsregeln

### Einzelprojekte

Jedes Projekt nutzt eigene SemVer-Tags, etwa `v0.2.0` für den Core Agent. Ein Projekt darf intern weiterentwickelt werden, solange sein veröffentlichter Vertrag kompatibel bleibt.

### Verträge

Die Vertragsversion ist unabhängig und wird in `contracts/VERSION` sowie in den Metadaten der Spezifikationen geführt.

- Patch: Dokumentationskorrektur oder optionale, rein additive Information.
- Minor: neue optionale Endpunkte, Events oder Felder.
- Major: entfernte/umbenannte Endpunkte, Events oder Pflichtfelder sowie geänderte Bedeutung bestehender Felder.

Clients müssen unbekannte optionale Felder und unbekannte additive Event-Daten ignorieren können. Neue Pflichtfelder oder ein verändertes Event-Format sind Breaking Changes.

### Plattform-Releases

Ein Platform-Tag, zum Beispiel `platform-v0.1.0`, markiert einen getesteten Satz aus:

- Platform-Commit
- Contract-Version
- `workspace.lock.yaml`
- Einzelprojekt-Commit-IDs

## Workflow

1. Eine Änderung beginnt beim API- oder Event-Vertrag, falls Komponenten miteinander kommunizieren.
2. Der Contract wird im Platform-Repository angepasst und versioniert.
3. Betroffene Einzelprojekte implementieren die Änderung in ihren eigenen Feature-Branches.
4. Contract-Tests und End-to-End-Tests laufen im Platform-Workspace.
5. Erfolgreich getestete Commits werden in `workspace.lock.yaml` eingetragen.
6. Der Platform-Commit wird getaggt, sobald die Kombination stabil ist.

## Umsetzungsphasen

### Phase 1 – Repository und Inventar

- Platform-Repository initialisieren und Remote bei GitHub anlegen.
- `apps/` und `services/` in `.gitignore` aufnehmen.
- Für jede Komponente Remote-URL, Branch und Commit erfassen.
- `.env.example` je Einzelprojekt prüfen; echte `.env`-Dateien niemals einchecken.
- Absolute Altpfade aus `D:\Projects\DataScience\Portfolio` inventarisieren und auf Platform-Pfade umstellen.

**Akzeptanz:** Das Platform-Repository enthält keine Dateien aus Child-Repositories und keinen geheimen Konfigurationswert.

### Phase 2 – Laufzeitkonfiguration

- Port- und URL-Konventionen in einer Platform-`.env.example` dokumentieren.
- RAG-Pfad des Core Agents explizit auf `services/ai_rag_local` konfigurieren.
- Memory- und Datenpfade unter `data/` zentral, aber nicht versioniert ablegen.
- Ein Diagnose-Skript erstellt einen lesbaren Status aller Komponenten.

**Akzeptanz:** Alle Komponenten starten am neuen Ort mit dokumentierter Konfiguration.

### Phase 3 – Contracts

- `contracts/agent-api.openapi.yaml` für Agent-HTTP-Endpunkte erstellen.
- `contracts/agent-events.schema.json` für SSE-Events erstellen.
- `contracts/voice-ws.asyncapi.yaml` für UI-Voice-WebSocket erstellen.
- Lesbare Beispiele in `docs/api_contracts.md` ergänzen.

**Akzeptanz:** Jeder interprojektliche Request und jedes Event ist maschinen- und menschenlesbar definiert.

### Phase 4 – Integration und Web UI

- Text-UI zuerst direkt gegen die Agent-SSE-API entwickeln.
- Voice Runtime mit derselben `session_id` an den Core Agent anbinden.
- STT → Agent → TTS mit Fake-Backends und später realen lokalen Backends testen.
- Einen gemeinsamen lokalen Startweg bereitstellen.

**Akzeptanz:** Text- und Sprachinteraktion verwenden dieselbe Agent-Session und der Tool-Status ist sichtbar.

## Nicht-Ziele

- Kein Monorepo der Anwendungsquellcodes.
- Keine direkten Python-Imports zwischen Core Agent, Voice Runtime und Web UI.
- Keine Secrets, Datenbanken, Modellgewichte oder virtuelle Umgebungen im Git.
- Noch keine verpflichtenden Git-Submodule; sie werden nur erwogen, falls ein fester Checkout-Snapshot später wichtiger als der einfache Multi-Repo-Workflow wird.
