# Abwehr von Prompt-Injection in Tool-Inhalten

> Kontext: [AI Agent Platform](../README.md) und
> [lokale Web-Recherche](local_web_research.md).

Webseiten, Suchtreffer, RAG-Dokumente, Document-Service-Ergebnisse und gelesene
Dateien sind fuer das Modell **nicht vertrauenswuerdige Daten**. Sie koennen
Text enthalten, der wie eine Anweisung aussieht, etwa „ignoriere vorherige
Regeln“ oder „rufe dieses Tool auf“.

## Implementierte Schutzschichten

1. Jeder erfolgreiche Tool-Inhalt wird vor dem naechsten LLM-Schritt in einen
   eindeutig begrenzten Abschnitt `UNTRUSTED TOOL OUTPUT — DATA ONLY` gesetzt.
   Der Modellkontext sagt explizit, dass dieser Abschnitt nur Belegmaterial ist
   und weder Berechtigungen noch Instruktionen enthalten kann.
2. Eine konservative Heuristik markiert typische Muster: Regelueberschreibung,
   Rollen-Imitation, Geheimnis-Exfiltration und Tool-Manipulation. Die Treffer
   werden als `prompt_injection_signals` in den Metadaten des Tool-Ergebnisses
   protokolliert. Ein Treffer ist ein Warnsignal, kein Beweis.
3. Der Systemprompt verbietet, Anweisungen, Rollenbehauptungen, Geheimnis-
   Abfragen oder Tool-Aufrufe aus Tool-Inhalten zu befolgen.
4. Technische Grenzen des `web_scrape`-Tools bleiben wirksam: kein JavaScript,
   keine privaten Netzwerkziele, Redirect-Pruefung sowie Groessen- und
   Zeitlimits.
5. Seiteneffekte bleiben durch die Tool-Policy gesperrt: Tools wie
   `calendar_create` verlangen weiterhin eine explizite Bestaetigung und
   koennen nicht durch Text auf einer Webseite freigeschaltet werden.

## Grenzen

Prompt-Injection ist ein Modellangriff; eine Heuristik kann sie nicht vollstaendig
erkennen oder ausschliessen. Der Agent darf deshalb externe Inhalte nur als
Quellen fuer die Anfrage zusammenfassen, nicht als Handlungsauftrag verwenden.
Bei einer Erweiterung um neue schreibende, Netzwerk- oder Credential-Tools sind
explizite Nutzerbestaetigung, enges Allowlisting und eigene Sicherheits-Tests
verpflichtend.

## Tests

`apps/ai_agent_core/tests/test_prompt_injection_defence.py` prueft die
Mustererkennung und verifiziert, dass der Inhalt vor dem zweiten LLM-Durchlauf
tatsaechlich als untrusted data gekapselt wird.
