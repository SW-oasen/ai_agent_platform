# Lokale Kalender-Erinnerungen

> Teil der [AI Agent Platform](../README.md). Siehe auch
> [Terminbestaetigung](calendar_confirmation.md).

**Schliessen** markiert eine faellige Erinnerung dauerhaft als erledigt.
**In 5 Minuten erneut erinnern** verschiebt sie um fuenf Minuten; sie wird
nicht automatisch geschlossen.

Termine koennen mit `reminder_minutes_before` erstellt werden. Solange die Chat-UI geoeffnet ist, fragt sie alle 30 Sekunden faellige lokale Erinnerungen ab und zeigt sie als Karte mit **Schließen** oder **In 5 Minuten**.

Der Core fuegt jedem LLM-Turn die aktuelle lokale Zeit hinzu. Die Anwendungszeit
wird hoechstens alle fuenf Minuten per NTP von `time.cloudflare.com` bzw.
`time.google.com` abgeglichen; ist beides nicht erreichbar, verwendet sie
kontrolliert die Windows-Systemzeit. Die Windows-Uhr selbst wird nicht veraendert.

Es gibt bewusst noch keine Hintergrund- oder Windows-Benachrichtigung. Faellige Erinnerungen bleiben in SQLite offen und erscheinen beim naechsten UI-Aufruf.

## TODO: Abnahmetests

- Termin und Erinnerung erst nach Bestaetigung speichern.
- Faellige Erinnerung genau einmal anzeigen.
- Schließen und Snooze dauerhaft pruefen.
- Offene Erinnerung nach UI-/Service-Neustart nachholen.
- Termin ohne Erinnerung ausschliessen.
