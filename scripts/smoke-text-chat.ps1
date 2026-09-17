param(
    [string]$Message = 'Antworte bitte nur mit: CHAT_OK',
    [string]$UserId = 'smoke-user',
    [string]$SessionId = 'smoke-session'
)

$ErrorActionPreference = 'Stop'
$body = @{ message = $Message; user_id = $UserId; session_id = $SessionId } | ConvertTo-Json
try {
    $result = Invoke-RestMethod -Uri 'http://127.0.0.1:8200/v1/chat' -Method Post -ContentType 'application/json' -Body $body -TimeoutSec 150
} catch {
    throw "Text-Chat-Smoke-Test fehlgeschlagen: $($_.Exception.Message)"
}
if ([string]::IsNullOrWhiteSpace($result.message)) { throw 'Text-Chat-Smoke-Test lieferte keine LLM-Antwort.' }
Write-Host "Text-Chat erfolgreich. Run-ID: $($result.run_id)"
Write-Output $result
