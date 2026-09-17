# Lokaler Kalender: Vorschlag und Bestaetigung

> Teil der [AI Agent Platform](../README.md). Die UI-Bedienung steht im
> [Text-Chat-Workflow](local_text_chat.md).

`calendar_create` schreibt ausschliesslich in die lokale SQLite-Kalenderdatei.
Ein Agent-Aufruf kann einen Termin nur vorschlagen; er darf ihn nicht direkt
erstellen.

1. Bitte im Chat beispielsweise: `Lege morgen von 10:00 bis 11:00 einen Termin
   Projektplanung an.`
2. Der Agent zeigt die konkreten ISO-Zeitwerte und den Button **Termin
   erstellen**.
3. Erst dieser bewusst geklickte Button sendet die unveraenderten Daten ueber
   Web-UI, Orchestrator und Core an `calendar_create`.

Der Core akzeptiert die Ausfuehrung nur am separaten lokalen Endpoint
`POST /v1/tools/calendar_create/confirm`; ein normaler Tool-Aufruf bleibt
gesperrt. Damit kann weder ein Prompt noch Inhalt einer Webseite einen Termin
ohne Interaktion erstellen.

Aktuell ist der Kalender lokal und nicht benutzergetrennt, weil die Plattform
im Einzelbenutzerbetrieb laeuft. Vor einer Mehrbenutzerfreigabe muessen
Authentisierung und ein user-scoped Kalenderpfad ergaenzt werden.

## Voice TODO

Bei der spaeteren Voice-Integration muessen klare Ja-/Nein-Aussagen auf einen
aktiven, unveraenderten Terminvorschlag abgebildet werden. Ohne aktiven
Vorschlag darf eine solche Aussage keine Kalenderaenderung ausloesen.
