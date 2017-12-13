# MyFS
MyFS implementation using libfuse

# ToDo:
- [ ] (Bug)Beim erstellen einer neuen Datei wird ein Ordner erstellt. Keine normale Datei
- [ ] (Bug)Berechnung der Positionen der START/END Blöcke
- [ ] SuperBlock füllen und Rechte aus den SuperBlock nehmen um diese für FuseGetAttr() bei "/" zurück zugeben
- [ ] SuperBlock Automatisch beim Hinzufügen/Löschen/Ändern von Daten aktualisieren
- [ ] Überprüfen der Rechte, um so für Aufg 1 READ-Only Datein zu haben, die auch nicht gelöscht werden können
- [ ] (Frage)Wollen wir den Container umschreiben, zum verwalten der freien Blöcke, oder lassen wir es so wie es ist. Nachteil wäre das wir alle Blöcke durchgehen zum finden von freien Blöcken.
- [ ] Dafür sorgen das der Container groß genug ist. Max?
- [ ] FuseWrite, zum (über-)schreiben in Datein. 
- [ ] Sinnvolle ERROR Returns
- [ ] Buffer Struktur für das Lesen und Schreiben von FUSE

# TestIdeen:
Aufgabe 1:
- Auch versuchen große Datein zu kopieren und zu lesen
- Vll. sogar so groß, das sie (theoretisch) nicht in das FS passen
- Datein ohne Inhalt kopieren / öffnen
- Sämtliche Dateinamen versuchen
- Löschen / Ändern testen (sobald implementet)


# Use
- English as language
- Todo style: TODO Name: Comment
- 4 Spaces instead of tabs (https://stackoverflow.com/a/408449/4300087)
- Doxygen documentation (Window > Preferences > Editor > Set: Documentation tool comments > Workspace default > Doxygen) (https://stackoverflow.com/a/7590019/4300087)
