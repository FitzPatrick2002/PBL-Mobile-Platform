from PySide6.QtGui import QPalette, QColor


def force_dark(app):
    p = QPalette()
    p.setColor(QPalette.Window, QColor(30, 30, 30))
    p.setColor(QPalette.WindowText, QColor(220, 220, 220))
    p.setColor(QPalette.Base, QColor(25, 25, 25))
    p.setColor(QPalette.AlternateBase, QColor(45, 45, 45))
    p.setColor(QPalette.Text, QColor(220, 220, 220))
    p.setColor(QPalette.Button, QColor(45, 45, 45))
    p.setColor(QPalette.ButtonText, QColor(220, 220, 220))
    p.setColor(QPalette.Highlight, QColor(42, 130, 218))
    p.setColor(QPalette.HighlightedText, QColor(0, 0, 0))
    app.setPalette(p)