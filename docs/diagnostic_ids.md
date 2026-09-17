# Diagnose-IDs bei Chat-Fehlern

> Kontext: [AI Agent Platform](../README.md) und
> [lokaler Text-Chat](local_text_chat.md).

Die Diagnose-ID ist nur bei einem Fehler sichtbar. Die Run-ID bleibt eine
interne technische Identitaet fuer Cancellation und Korrelation und muss im
normalen Betrieb nicht vom Benutzer kopiert werden.

Bei einem Fehler der Chat-Anfrage zeigt die UI unter der Fehlermeldung eine
`Diagnose-ID`. Sie entspricht der Korrelations-ID des fehlgeschlagenen Turns
und wird nicht bei erfolgreichen Antworten eingeblendet.

Für eine Fehleranalyse genuegen Diagnose-ID, ungefaehre Uhrzeit und die
eingegebene Nachricht. Die ID ist keine Berechtigung und enthaelt keine
Nutzdaten.
