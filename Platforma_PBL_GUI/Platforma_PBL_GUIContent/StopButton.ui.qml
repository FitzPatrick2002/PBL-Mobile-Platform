

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Timeline 1.0

Button {
    id: control
    width: 300
    height: 100

    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    flat: true

    Image {
        id: stop_button
        x: -28
        y: -97
        width: 355
        height: 293
        source: "images/Stop_button.png"
        fillMode: Image.Stretch
    }

    states: [
        State {
            name: "normal"
            when: !control.down
        },
        State {
            name: "down"
            when: control.down
        }
    ]
}
