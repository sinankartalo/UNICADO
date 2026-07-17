***** READ-ME zur Verwendung der UnitConversion *****

1) Kopieren des Ordners unitConversion in den Source-Ordner des zu bearbeitenden Moduls (mit Auﬂnahme "testUnitConversion.cpp", die nur Testzwecken dient)
2) Hinzufuegen der neuen Header-Dateien zum jeweiligen Modul in der CodeBlocks-Projektdatei <myModule>.cbp (rechte Maustaste auf Modulnamen im Editorbaum von CodeBlocks und dann "Add Files")
3) ggf. Hinzufuegen der Aerodynamik-Klassen, falls diese noch nicht vorhanden sind (Ordner ISA), da diese beim Umwandeln einiger Einheiten benoetigt werden
4) Unter Project->Build Options->Search Directory->Compiler die Ordner ISA und unitConversion hinzufuegen
5) Ersetzen aller hardgecodeter Umrechnungen unter Verwendung der Befehle der UnitConversion
6) Ausfuehren von micadoCheck