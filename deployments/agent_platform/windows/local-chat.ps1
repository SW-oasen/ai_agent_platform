param([switch]$Stop)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$settings = Join-Path $root 'config\platform.settings.json'
$processFile = Join-Path $root 'run\agent-platform-chat.pids.json'

function Stop-PlatformChat {
    if (!(Test-Path $processFile)) { Write-Host 'Kein lokaler Chat-Stack registriert.'; return }
    $pids = Get-Content $processFile -Raw | ConvertFrom-Json
    foreach ($processId in @($pids.core, $pids.orchestrator, $pids.memory_context, $pids.web_ui)) {
        if ($processId) {
            $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
            if ($process) { Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue }
        }
    }
    Remove-Item -LiteralPath $processFile -Force
    Write-Host 'Lokaler Chat-Stack beendet.'
}

if ($Stop) { Stop-PlatformChat; exit 0 }
if (!(Test-Path $settings)) { throw "Fehlt: $settings. Kopiere zuerst config\\platform.settings.example.json." }
if (Test-Path $processFile) { throw 'Chat-Stack ist bereits registriert. Mit -Stop zuerst beenden.' }

$corePython = Join-Path $root 'apps\ai_agent_core\.venv\Scripts\python.exe'
if (!(Test-Path $corePython)) { throw "Fehlt: $corePython" }

# Search is an independent local service, but the chat stack ensures it is ready
# before Core starts. local-searxng.ps1 leaves an already healthy instance intact.
$searxngStarter = Join-Path $root 'deployments\agent_platform\windows\local-searxng.ps1'
& powershell -NoProfile -ExecutionPolicy Bypass -File $searxngStarter
if ($LASTEXITCODE -ne 0) { throw 'SearXNG konnte nicht fuer den Chat-Stack vorbereitet werden.' }

$env:PLATFORM_SETTINGS_PATH = $settings
# The local platform uses direct network access by default.  A stale proxy
# inherited from a developer shell (for example 127.0.0.1:9) prevents Python
# tools from reaching public sites even when the browser works normally.
foreach ($proxyVariable in 'HTTP_PROXY', 'HTTPS_PROXY', 'ALL_PROXY', 'http_proxy', 'https_proxy', 'all_proxy') {
    Remove-Item "Env:$proxyVariable" -ErrorAction SilentlyContinue
}
# The Core uses the local RAG adapter during this development slice.  Make the
# component path explicit so a legacy user-level LOCAL_AGENT_RAG_PROJECT_PATH
# cannot point it at an unrelated checkout.
$env:LOCAL_AGENT_RAG_PROJECT_PATH = Join-Path $root 'services\ai_rag_local'
# web_search uses the local SearXNG service. Start it once with local-searxng.ps1.
$env:SEARXNG_URL = 'http://127.0.0.1:8206'
$core = Start-Process -FilePath $corePython -ArgumentList '-m', 'uvicorn', 'local_agent.api:app', '--app-dir', 'src', '--host', '127.0.0.1', '--port', '8201' -WorkingDirectory (Join-Path $root 'apps\ai_agent_core') -WindowStyle Hidden -PassThru
$memoryContext = Start-Process -FilePath $corePython -ArgumentList '-m', 'uvicorn', 'memory_context.api:app', '--app-dir', 'src', '--host', '127.0.0.1', '--port', '8205' -WorkingDirectory (Join-Path $root 'services\ai_memory_context_service') -WindowStyle Hidden -PassThru
$orchestrator = Start-Process -FilePath $corePython -ArgumentList '-m', 'uvicorn', 'conversation_orchestrator.api:app', '--app-dir', 'src', '--host', '127.0.0.1', '--port', '8202' -WorkingDirectory (Join-Path $root 'services\ai_conversation_orchestrator') -WindowStyle Hidden -PassThru
$webUi = Start-Process -FilePath $corePython -ArgumentList '-m', 'uvicorn', 'ai_web_ui_local.api:app', '--app-dir', 'src', '--host', '127.0.0.1', '--port', '8200' -WorkingDirectory (Join-Path $root 'apps\ai_web_ui_local') -WindowStyle Hidden -PassThru

New-Item -ItemType Directory -Force -Path (Split-Path $processFile) | Out-Null
@{ core = $core.Id; memory_context = $memoryContext.Id; orchestrator = $orchestrator.Id; web_ui = $webUi.Id } | ConvertTo-Json | Set-Content -LiteralPath $processFile -Encoding utf8
Start-Sleep -Seconds 2
foreach ($url in 'http://127.0.0.1:8201/v1/health', 'http://127.0.0.1:8205/v1/health', 'http://127.0.0.1:8202/v1/health', 'http://127.0.0.1:8200/health') {
    try { Invoke-RestMethod $url -TimeoutSec 3 | Out-Null } catch { Stop-PlatformChat; throw "Start fehlgeschlagen bei ${url}: $($_.Exception.Message)" }
}
Write-Host 'Chat-Stack bereit: http://127.0.0.1:8200'
