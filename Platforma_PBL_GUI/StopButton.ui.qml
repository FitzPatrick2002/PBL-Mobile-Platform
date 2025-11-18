

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
    display: AbstractButton.IconOnly

    Image {
        id: stop_button
        x: -281
        y: -171
        width: 649
        height: 393
        source: "Platforma_PBL_GUIContent/images/Stop_button.png"
        autoTransform: true
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
