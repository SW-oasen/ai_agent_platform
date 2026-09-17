# Cancellation-Regelung

> Kontext: [AI Agent Platform](../README.md), insbesondere der
> [Text-Chat-Workflow](local_text_chat.md).

## Ablauf

Jeder laufende Agent-Run besitzt eine clientseitig bekannte `run_id`. Ein Abbruch gilt genau fuer diese Run-ID und ist idempotent.

```text
Web-UI Cancel / Voice Barge-in
            |
            v
POST /v1/runs/{run_id}/cancel (Orchestrator, 8202)
            |
            v
POST /v1/runs/{run_id}/cancel (Core, 8201)
```

Web-UI und Voice Runtime erzeugen die Run-ID vor dem Start. Der Orchestrator gibt sie unveraendert an den Core weiter.

## Antwort und Ereignisse

Der Endpunkt antwortet immer mit HTTP 200 und `cancellation_requested` oder `already_finished`. Bei SSE-Runs emittiert der Core nach erfolgreichem Abbruch `run.cancelled`.

## Grenzen

- UI-Cancel sperrt die weitere Textausgabe und fordert den Remote-Abbruch an.
- Voice-Barge-in stoppt TTS und Playback sofort und fordert denselben Remote-Abbruch an.
- LLM-Streams werden kooperativ an Chunk- bzw. Runden-Grenzen beendet.
- Bereits blockierende I/O- oder Subprozess-Tools koennen nur ueber eine tool-spezifische Cancellation hart beendet werden. Jedes Tool benoetigt deshalb ein Timeout.

## Lokaler Test

1. `deployments/agent_platform/windows/local-chat.ps1` starten.
2. Eine laengere Anfrage senden und in der Web-UI **Abbrechen** waehlen.
3. Sicherstellen, dass ein nachfolgender Turn normal funktioniert.
4. Optional direkt pruefen:

```powershell
Invoke-RestMethod -Method Post http://127.0.0.1:8202/v1/runs/<run-id>/cancel
```
