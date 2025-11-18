/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick
import QtQuick.Controls

Button {
    id: control

    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    hoverEnabled: false
    flat: true

    Image {
        id: stop_button
        x: -280
        y: -159
        width: 1200
        height: 798
        source: "images/Stop_button.png"
        fillMode: Image.PreserveAspectFit
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
