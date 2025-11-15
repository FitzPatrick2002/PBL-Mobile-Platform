import sys

from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtCore import QObject, Slot, Signal

from autogen.settings import setup_qt_environment
from widgets import AboutUsOpening, Navigation





def main():
    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()

    setup_qt_environment(engine)


###########         Nazwy klas w Pythonie: AboutUsOpening (PascalCase)                            #####################
###########         Nazwy instancji / obiektów w QML: aboutUsOpening (camelCase, mała litera)     #####################


    about_us_opening = AboutUsOpening()
    engine.rootContext().setContextProperty("aboutUsOpening", about_us_opening)

    nav = Navigation()
    engine.rootContext().setContextProperty("navigation", nav)




    if not engine.rootObjects():
        sys.exit(-1)

    ex = app.exec()
    del engine
    return ex


if __name__ == "__main__":
    sys.exit(main())
