param([switch]$Stop)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$composeFile = Join-Path $root 'deployments\agent_platform\searxng\compose.yml'

if ($Stop) {
    & docker compose -f $composeFile -p ai-agent-platform-searxng down
    if ($LASTEXITCODE -ne 0) { throw 'SearXNG konnte nicht beendet werden.' }
    Write-Host 'Lokaler SearXNG-Dienst beendet.'
    exit 0
}

$isRunning = (& docker inspect -f '{{.State.Running}}' ai-agent-platform-searxng 2>$null) -eq 'true'
if (!$isRunning) {
    # The service has no persistent volume. A fresh secret is intentionally created
    # for each local start and remains only in the child Docker Compose environment.
    $env:SEARXNG_SECRET = [Guid]::NewGuid().ToString('N') + [Guid]::NewGuid().ToString('N')
    & docker compose -f $composeFile -p ai-agent-platform-searxng up -d
    if ($LASTEXITCODE -ne 0) { throw 'SearXNG konnte nicht gestartet werden.' }
}

$healthUrl = 'http://127.0.0.1:8206/search?q=healthcheck&format=json'
for ($attempt = 1; $attempt -le 15; $attempt++) {
    try {
        $response = Invoke-RestMethod $healthUrl -TimeoutSec 5
        if ($null -ne $response.results) {
            $state = if ($isRunning) { 'bereits bereit' } else { 'bereit' }
            Write-Host "Lokaler SearXNG-Dienst ${state}: http://127.0.0.1:8206"
            exit 0
        }
    } catch {
        Start-Sleep -Seconds 2
    }
}
throw 'SearXNG ist gestartet, beantwortet aber keinen JSON-Suchtest. Pruefe: docker compose -f deployments\agent_platform\searxng\compose.yml -p ai-agent-platform-searxng logs'
