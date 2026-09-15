[CmdletBinding()]
param(
    [switch]$CheckEndpoints
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
$requiredPaths = @('apps/ai_agent_core', 'apps/ai_voice_agent_local', 'services/ai_conversation_orchestrator', 'services/ai_memory_context_service', 'services/ai_rag_local', 'contracts')
$failed = $false

Write-Host 'AI Agent Platform diagnostics'
foreach ($relativePath in $requiredPaths) {
    $path = Join-Path $workspace $relativePath
    $exists = Test-Path -LiteralPath $path
    $label = if ($exists) { 'OK' } else { 'MISSING' }
    Write-Host ("[{0}] {1}" -f $label, $relativePath)
    if (-not $exists) { $failed = $true }
}

foreach ($contract in @('contracts/VERSION', 'contracts/agent-api.openapi.yaml', 'contracts/agent-events.schema.json', 'contracts/orchestrator-api.openapi.yaml', 'contracts/memory-context-api.openapi.yaml', 'contracts/voice-orchestrator.schema.json', 'contracts/voice-ws.asyncapi.yaml', 'contracts/platform-settings.schema.json', 'contracts/configuration-api.openapi.yaml', 'docs/api_contracts.md', 'config/platform.settings.example.json', 'config/platform.settings.json', 'workspace.lock.yaml', '.env.example')) {
    $exists = Test-Path -LiteralPath (Join-Path $workspace $contract)
    Write-Host ("[{0}] {1}" -f $(if ($exists) {'OK'} else {'MISSING'}), $contract)
    if (-not $exists) { $failed = $true }
}

if ($CheckEndpoints) {
    foreach ($endpoint in @('http://127.0.0.1:8201/v1/health', 'http://127.0.0.1:8202/health', 'http://127.0.0.1:8203/health')) {
        try {
            $response = Invoke-WebRequest -Uri $endpoint -TimeoutSec 2 -UseBasicParsing
            Write-Host ("[OK] {0} ({1})" -f $endpoint, $response.StatusCode)
        } catch {
            Write-Host ("[UNAVAILABLE] {0}" -f $endpoint)
        }
    }
}

if ($failed) { exit 1 }
