# Lokale Web-Recherche

> Teil der [AI Agent Platform](../README.md). Fuer den Chat-Gesamtworkflow
> siehe auch [lokaler Text-Chat](local_text_chat.md).

Die Plattform trennt Suche und Seitenabruf bewusst in zwei read-only Tools:

- `web_search` fragt die lokale SearXNG-Instanz ab und liefert eine kurze Liste mit Quelle, URL und Trefferzusammenfassung.
- `web_scrape` liest anschliessend eine konkrete, oeffentliche URL und extrahiert lesbaren HTML-, Text- oder PDF-Inhalt. Es fuehrt kein JavaScript aus.

## SearXNG starten

Der normale Weg ist `local-chat.ps1`: Es stellt SearXNG automatisch bereit und
laesst eine bereits laufende Instanz unveraendert. `local-searxng.ps1` bleibt
fuer einen eigenstaendigen Start oder Stop verfuegbar.

Docker Desktop muss laufen. Der Dienst wird ausschliesslich auf dem lokalen Loopback-Port `8206` veroeffentlicht:

```powershell
.\deployments\agent_platform\windows\local-searxng.ps1
```

Der Start erzeugt einen neuen Laufzeit-Schluessel nur im Prozessumfeld von Docker Compose. Die Compose-Datei bindet weder ein Daten- noch ein Cache-Volume ein; Suchanfragen, Treffer und Dokumente werden dadurch nicht dauerhaft lokal abgelegt. Beenden:

```powershell
.\deployments\agent_platform\windows\local-searxng.ps1 -Stop
```

Danach den Text-Chat-Stack neu starten, damit der Core `SEARXNG_URL` erhaelt:

```powershell
.\deployments\agent_platform\windows\local-chat.ps1 -Stop
.\deployments\agent_platform\windows\local-chat.ps1
```

SearXNG ist eine Meta-Suche, kein LLM und kein Web-Scraper. Die verwendbaren Suchmaschinen und deren Ergebnisqualitaet richten sich nach deren jeweiligen Regeln; einzelne Engines koennen zeitweise keine Treffer liefern.

## Sicherheitsgrenzen des Scrapers

`web_scrape` akzeptiert nur absolute `http`-/`https`-URLs ohne eingebettete Zugangsdaten. Vor Abruf und bei jedem Redirect prueft es die DNS-Adresse und blockiert Loopback-, private, Link-Local- und reservierte Netze. Die Antwort ist auf 5 MB und die Tool-Ausgabe auf 12.000 Zeichen begrenzt. Damit kann das Tool nicht als Zugriffspfad auf lokale Dienste verwendet werden.

Fuer rein clientseitig gerenderte Seiten ist bewusst kein Browser-Bypass enthalten. Eine spaetere Playwright/Edge-Renderer-Erweiterung muss separat aktiviert, mit eigenen Zeit- und Sicherheitsgrenzen versehen und gegen die Webseitenbedingungen getestet werden.

Inhalte externer Seiten sind zudem fuer das LLM nicht vertrauenswuerdig. Die
Prompt-Injection-Abwehr und ihre Grenzen stehen in
[prompt_injection_defence.md](prompt_injection_defence.md).
# Aktualitaet und Quellenabsicherung

Der Core wendet vor dem LLM eine deterministische Freshness-Policy an. Fragen
mit aktuellem Bezug sowie Jahreszahlen nach `agent.knowledge_cutoff` loesen
zwingend `web_search` aus. Fuer Wahltermine, Gesetze und Wetterwarnungen sind
zusaetzlich Quellenfamilien definiert. Eine Antwort mit einer offiziellen
Behauptung wird blockiert, wenn sie keine zulaessige Primaerquelle verlinkt.

Bei verpflichtender Recherche wird der vorherige Chatverlauf nicht als
Faktenbasis an das Modell uebergeben. Das verhindert, dass fruehere falsche
Assistentenantworten aktuelle Angaben fortschreiben. Umfragen bleiben
zeitgebundene Momentaufnahmen und muessen mit Quelle, Institut und Feldzeit
dargestellt werden.

Die vertrauenswuerdige Zeit kommt vom lokalen Clock-Service mit NTP-Abgleich.
Bei Fragen nach dem naechsten Termin prueft der Core die fertige Antwort
zusaetzlich: Vergangene explizite Daten oder ein fehlendes zukuenftiges Datum
werden nicht als naechster Termin ausgegeben. Eine belegte Aussage, dass noch
kein fester Termin existiert, bleibt zulaessig.
