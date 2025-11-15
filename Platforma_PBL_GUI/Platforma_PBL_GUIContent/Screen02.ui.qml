

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 1200
    height: 800


    Rectangle {
        id: rectangle
        x: 0
        y: 0
        width: 1200
        height: 800
        color: "#ffffff"
    }
    Image {
        id: stop_button
        x: 8
        y: -8
        source: "images/Stop_button.png"
        fillMode: Image.PreserveAspectFit
    }
}
