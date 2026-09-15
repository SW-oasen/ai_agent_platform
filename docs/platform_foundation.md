# Platform Foundation

The platform repository contains contracts, integration configuration and diagnostics; it does not contain component application code. `apps/` and `services/` are local workspaces for independent repositories and remain ignored by the platform Git repository.

## First local setup

1. Copy `.env.example` to `.env` and keep it limited to bootstrap values.
2. Copy `config/platform.settings.example.json` to `config/platform.settings.json`; this is the single source of truth for non-secret runtime settings.
3. Ensure the three directories in `workspace.lock.yaml` exist.
4. Run `powershell -ExecutionPolicy Bypass -File scripts/diagnose.ps1`.
5. After an integrated test run, replace the lock's `UNRESOLVED` refs with immutable commit SHAs and set `validated_at`.

Before starting a component in Platform mode, set the settings path once in that shell:

```powershell
$env:PLATFORM_SETTINGS_PATH = (Resolve-Path config/platform.settings.json)
```

## Contract ownership

- `agent-api.openapi.yaml`: HTTP and SSE entry point of the Core Agent.
- `agent-events.schema.json`: data payload for every Core SSE event.
- `voice-orchestrator.schema.json`: messages emitted by Voice Runtime to the future Conversation Orchestrator.
- `platform-settings.schema.json`: single non-secret runtime settings document.
- `configuration-api.openapi.yaml`: Web-UI control-plane API for settings reads, versioned writes and apply status.
- `document_api/`: independent AI Document Service contract; it is not an Agent-Core contract.

Contract versioning is defined by `contracts/VERSION`. Additive optional fields are minor changes; removing, renaming or changing required fields is a major change.

The version 0.1.0 contracts are the target boundary. Existing Core and Voice prototypes do not yet implement every required identity field, event name or cancellation endpoint. That migration belongs to the next implementation increment.

## Local port allocation

The platform reserves the contiguous block `8200–8204`; each service must bind to `127.0.0.1` by default.

| Port | Owner | Protocol / purpose |
| --- | --- | --- |
| 8200 | Web UI | HTTP |
| 8201 | Core Agent | HTTP and SSE |
| 8202 | Conversation Orchestrator | HTTP and SSE |
| 8203 | RAG Service | HTTP |
| 8204 | Voice Runtime | WebSocket |

Known external local allocations are deliberately excluded: `8000` MinerU API, `8080` AI Job Application Assistant, `8090` AI Document Service HTTP default, `8181` AI Document Demo UI, `8443` AI Document Service TLS deployment, `10091` its WildFly management port, and `11434` Ollama. Do not assign platform services to these ports.

## Settings ownership and safe updates

`config/platform.settings.json` is the sole source of truth for non-secret settings shared across the Web UI, Core Agent, Voice Runtime and RAG. It is deliberately local and ignored by Git; its example is versioned. Platform mode is explicitly enabled only by `PLATFORM_SETTINGS_PATH`; components do not discover this file implicitly. Component `.env` files keep hardware bootstrap values.

The future Configuration API is served by the Orchestrator, not by the LLM-powered Core Agent. A UI reads a snapshot and writes a full validated replacement with `If-Match: <ETag>`. A stale revision returns `409 Conflict`; it must be reloaded rather than silently overwritten. Every service reports its applied revision as `applied`, `pending_restart` or `failed`. In Platform mode, local overrides for platform-owned agent and voice fields are ignored; hardware settings remain local. Ports, bind hosts, paths and all secrets remain deployment-only and cannot be changed through this API.
