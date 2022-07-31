# bmp2ps
Erzeugt aus einer BMP-Datei (alle Formate) ein monochromes PostScript-Image mit weißem Hintergrund.

Die Farbe des Eingangs-Kanals (rot, grün, blau, schwarz oder weiß) und der 1 bis 2 Ausgabekanäle (rot, grün, blau, gelb, magenta, cyan oder schwarz) können angegeben werden. Voreingestellt sind (schwarz, schwarz) bzw. (schwarz,X) oder (W,schwarz).

Ein Pixel auf dem Eingabekanal ist gesetzt, wenn der Wert auf dem Kanal ausreichend groß (> 170) und der Wert eines der beiden anderen Kanäle ausreichend klein (< 85) ist. Ein Pixel ist schwarz, wenn die Werte auf allen Kanälen ausreichend klein (< 85) sind. Ein Pixel ist weiß, wenn die Werte auf allen Kanälen ausreichend groß (> 170) sind.

Bei schwarz als Ausgabefarbe wird ein PostScript **Image** erstellt, bei allen anderen Ausgabefarben ein **Colorimage**. Der Speicherplatzbedarf für ein Colorimage ist aber nicht größer.
