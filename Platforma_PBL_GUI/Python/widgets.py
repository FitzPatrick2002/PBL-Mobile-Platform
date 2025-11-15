
from PySide6.QtCore import QObject, Slot, Signal
import webbrowser, sys
from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickWindow

class AboutUsOpening(QObject):
    
    @Slot()
    def open_webbrowser(self):
        webbrowser.open_new('https://github.com/FitzPatrick2002/PBL-Mobile-Platform')
    

class Navigation(QObject):

    @Slot()
    def closeCurrentWindow(self):
        app = QGuiApplication.instance()
        if not app:
            print("Brak instancji aplikacji.")
            return

        for w in app.allWindows():
            # Szukamy widocznego QML-owego okna
            if isinstance(w, QQuickWindow) and w.isVisible():
                w.close()
                print("Zamknięto okno:", w)
                return

        print("Nie znaleziono widocznego okna do zamknięcia.")