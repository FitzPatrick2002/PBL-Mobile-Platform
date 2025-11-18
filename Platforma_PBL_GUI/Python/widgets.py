
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
    def __init__(self, engine):
        super().__init__()
        self.engine = engine

    @Slot(str)
    def openScreen(self, name: str):
        root = self.engine.rootObjects()[0]  # to jest Window z App.qml
        root.openScreen(name)
        print(f"Przełączono na ekran: {name}")