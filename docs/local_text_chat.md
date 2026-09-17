# Lokaler Text-Chat: testbarer Vertikalschnitt

> Teil der [AI Agent Platform](../README.md). Architektur, Komponenten und
> weitere Workflows stehen im zentralen Projektueberblick.

## Aktueller Betriebsstand

`local-chat.ps1` startet neben Core, Orchestrator, Memory-Service und Web-UI
bei Bedarf auch den lokalen SearXNG-Dienst. Der Browser bzw. VS Code startet
den Chat nicht selbst; die Hintergrundprozesse laufen bis zum Stop-Skript.

Die UI bietet einen Abbruchbutton waehrend eines Runs, sichere
Markdown-Darstellung, Quellen-Chips, Kalender-Bestaetigung und lokale
Erinnerungen. Die feste linke Navigation zeigt den aktiven lokalen Benutzer,
`Chat`, `Historie` und persoenliche Einstellungen; sie wird auf schmalen
Bildschirmen zu einer Icon-Leiste. In der Historie lassen sich ganze Sessions,
vollstaendige Frage-Antwort-Austausche und explizit gespeicherte Informationen
kompakt anzeigen und loeschen. Fehler zeigen eine Diagnose-ID. Details stehen in
`local_web_research.md`, `calendar_confirmation.md`,
`calendar_reminders.md` und `diagnostic_ids.md`.

Der Text-Chat läuft ohne Voice über diese Kette:

```text
Browser (8200) -> Web-UI-Proxy -> Conversation Orchestrator (8202) -> Core Agent (8201) -> lokales LLM
```

Der Orchestrator übergibt die Plattform-Identitäten (`user_id`, `session_id`,
`turn_id`, `correlation_id`) an den Core Agent. Voice kann später denselben
Orchestrator-Endpunkt verwenden, ist aber für diesen Pfad nicht gestartet.

## Start und Test

`config/platform.settings.json` muss vorhanden sein und der darin konfigurierte
Ollama-kompatible LLM-Endpunkt erreichbar sein.

```powershell
powershell -ExecutionPolicy Bypass -File deployments/agent_platform/windows/local-chat.ps1
powershell -ExecutionPolicy Bypass -File scripts/smoke-text-chat.ps1
```

Danach im Browser `http://127.0.0.1:8200` öffnen. Der sichtbare Chat ruft
`POST /v1/chat` der Web-UI auf; diese leitet ausschließlich an
`POST /v1/turns` des Orchestrators weiter.

Beenden:

```powershell
powershell -ExecutionPolicy Bypass -File deployments/agent_platform/windows/local-chat.ps1 -Stop
```
